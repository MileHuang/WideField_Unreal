#include "AssembleLevelManager.h"

#include "AssemblyPart.h"
#include "PartDatabase.h"
#include "PartInfo.h"

#include "Blueprint/UserWidget.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "AssembleLevelManager.h"
#include "AssemblySaveGame.h"
#include "SnapPointComponent.h"
#include "SlideConstraintComponent.h"
#include "PlaneConstraintComponent.h"
#include "DeleteAllConfirmWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
AAssembleLevelManager::AAssembleLevelManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAssembleLevelManager::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    EnableInput(PC);

    if (InputComponent)
    {
        FInputKeyBinding DeleteAllBinding(
            FInputChord(EKeys::Delete, true, false, false, false),
            IE_Pressed
        );

        DeleteAllBinding.KeyDelegate.GetDelegateForManualSet().BindUObject(
            this,
            &AAssembleLevelManager::ShowDeleteAllConfirm
        );

        DeleteAllBinding.bConsumeInput = true;

        InputComponent->KeyBindings.Add(DeleteAllBinding);
        InputComponent->BindKey(EKeys::Delete, IE_Pressed, this, &AAssembleLevelManager::DeleteSelected);

        InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &AAssembleLevelManager::TogglePauseMenu);
        InputComponent->BindKey(EKeys::M, IE_Pressed, this, &AAssembleLevelManager::TogglePauseMenu);

        InputComponent->BindKey(EKeys::L, IE_Pressed, this, &AAssembleLevelManager::ToggleLaser);
        InputComponent->BindKey(
            EKeys::S,
            IE_Pressed,
            this,
            &AAssembleLevelManager::SaveAssembly
        );

        InputComponent->BindKey(
            EKeys::O,
            IE_Pressed,
            this,
            &AAssembleLevelManager::LoadAssembly
        );
        FInputKeyBinding& LeftMouseBinding =
            InputComponent->BindKey(
                EKeys::LeftMouseButton,
                IE_Pressed,
                this,
                &AAssembleLevelManager::ShowPartInfo
            );

        LeftMouseBinding.bConsumeInput = false;
    }


    if (SpawnWidgetClass)
    {
        SpawnWidget =
            CreateWidget<UUserWidget>(
                GetWorld(),
                SpawnWidgetClass
            );

        if (SpawnWidget)
        {
            SpawnWidget->AddToViewport();
        }
    }


    if (TipsWidgetClass)
    {
        TipsWidget =
            CreateWidget<UUserWidget>(
                GetWorld(),
                TipsWidgetClass
            );

        if (TipsWidget)
        {
            TipsWidget->AddToViewport();
        }
    }

    GetWorldTimerManager().SetTimerForNextTick(
        this,
        &AAssembleLevelManager::HideAllLasers
    );
    LoadAssembly();
}
void AAssembleLevelManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TraceMouse();
}
FString AAssembleLevelManager::GetPartNameFromActor(AActor* Actor) const
{
    if (!Actor || !PartDatabase)
    {
        return TEXT("");
    }

    AAssemblyPart* Part = Cast<AAssemblyPart>(Actor);

    if (Part && !Part->SavePartName.IsEmpty())
    {
        return Part->SavePartName;
    }

    UStaticMeshComponent* MeshComp =
        Actor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp || !MeshComp->GetStaticMesh())
    {
        return TEXT("");
    }

    FString MeshName =
        MeshComp->GetStaticMesh()->GetName();

    FPartInfoData PartData;

    if (PartDatabase->FindPartInfo(MeshName, PartData))
    {
        return PartData.PartName;
    }

    return TEXT("");
}
void AAssembleLevelManager::TraceMouse()
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC) return;

    float MouseX = 0.f;
    float MouseY = 0.f;

    if (!PC->GetMousePosition(MouseX, MouseY))
    {
        HitActor = nullptr;
        return;
    }

    FVector WorldLocation;
    FVector WorldDirection;

    PC->DeprojectScreenPositionToWorld(
        MouseX,
        MouseY,
        WorldLocation,
        WorldDirection
    );

    FVector Start = WorldLocation;
    FVector End = Start + WorldDirection * TraceDistance;

    FHitResult Hit;

    bool bHit =
        GetWorld()->LineTraceSingleByChannel(
            Hit,
            Start,
            End,
            ECC_Visibility
        );

    AActor* NewHitActor =
        bHit ? Hit.GetActor() : nullptr;

    if (NewHitActor != LastHoverActor)
    {
        if (LastHoverActor)
        {
            UnhoverActor(LastHoverActor);
        }

        if (NewHitActor)
        {
            HoverActor(NewHitActor);
        }

        LastHoverActor = NewHitActor;
    }

    HitActor = NewHitActor;
}

void AAssembleLevelManager::HoverActor(AActor* NewActor)
{
    if (!NewActor || !HoverOverlayMaterial) return;

    UStaticMeshComponent* MeshComp =
        NewActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp) return;

    MeshComp->bDisallowNanite = true;
    MeshComp->MarkRenderStateDirty();
    MeshComp->SetOverlayMaterial(HoverOverlayMaterial);
}

void AAssembleLevelManager::UnhoverActor(AActor* OldActor)
{
    if (!OldActor) return;

    UStaticMeshComponent* MeshComp =
        OldActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp) return;

    MeshComp->SetOverlayMaterial(nullptr);
    MeshComp->bDisallowNanite = false;
    MeshComp->MarkRenderStateDirty();
}

void AAssembleLevelManager::DeleteSelected()
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (PC && PC->IsInputKeyDown(EKeys::LeftShift))
    {
        return;
    }

    if (!HitActor) return;

    if (AAssemblyPart* Part = Cast<AAssemblyPart>(HitActor))
    {
        Part->ClearAllSnapConnections();
    }

    HitActor->Destroy();

    HitActor = nullptr;
    LastHoverActor = nullptr;
}
void AAssembleLevelManager::TogglePauseMenu()
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC) return;

    if (!PauseWidget)
    {
        if (!PauseWidgetClass) return;

        PauseWidget =
            CreateWidget<UUserWidget>(
                GetWorld(),
                PauseWidgetClass
            );

        if (!PauseWidget) return;

        PauseWidget->AddToViewport();

        PC->SetShowMouseCursor(true);
        PC->SetInputMode(FInputModeUIOnly());
    }
    else
    {
        PauseWidget->RemoveFromParent();
        PauseWidget = nullptr;

        PC->SetShowMouseCursor(true);
        PC->SetInputMode(FInputModeGameOnly());
    }
}

void AAssembleLevelManager::ToggleSpawnWidget()
{
    if (!SpawnWidget)
    {
        if (!SpawnWidgetClass) return;

        SpawnWidget =
            CreateWidget<UUserWidget>(
                GetWorld(),
                SpawnWidgetClass
            );

        if (SpawnWidget)
        {
            SpawnWidget->AddToViewport();
        }
    }
    else
    {
        SpawnWidget->RemoveFromParent();
        SpawnWidget = nullptr;
    }
}

void AAssembleLevelManager::HideAllLasers()
{
    if (!LaserClass) return;

    TArray<AActor*> Lasers;

    UGameplayStatics::GetAllActorsOfClass(
        this,
        LaserClass,
        Lasers
    );

    for (AActor* Laser : Lasers)
    {
        if (Laser)
        {
            Laser->SetActorHiddenInGame(true);
        }
    }

    bLaserPressed = true;
}

void AAssembleLevelManager::ToggleLaser()
{
    if (!LaserClass) return;

    TArray<AActor*> Lasers;

    UGameplayStatics::GetAllActorsOfClass(
        this,
        LaserClass,
        Lasers
    );

    for (AActor* Laser : Lasers)
    {
        if (Laser)
        {
            Laser->SetActorHiddenInGame(!bLaserPressed);
        }
    }

    bLaserPressed = !bLaserPressed;
}

void AAssembleLevelManager::ClosePartInfo()
{
    if (CurrentPartInfoWidget)
    {
        CurrentPartInfoWidget->RemoveFromParent();
        CurrentPartInfoWidget = nullptr;
    }
}

void AAssembleLevelManager::ShowPartInfo()
{
    if (!HitActor)
    {
        ClosePartInfo();
        return;
    }

    if (!PartInfoWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("PartInfoWidgetClass is null"));
        return;
    }

    if (!PartDatabase)
    {
        UE_LOG(LogTemp, Warning, TEXT("PartDatabase is null"));
        return;
    }

    UStaticMeshComponent* MeshComp =
        HitActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp || !MeshComp->GetStaticMesh())
    {
        ClosePartInfo();
        return;
    }

    FString MeshName =
        MeshComp->GetStaticMesh()->GetName();

    FPartInfoData PartData;

    if (!PartDatabase->FindPartInfo(MeshName, PartData))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("No PartData found for %s"),
            *MeshName);

        ClosePartInfo();
        return;
    }

    ClosePartInfo();

    UPartInfo* Widget =
        CreateWidget<UPartInfo>(
            GetWorld(),
            PartInfoWidgetClass
        );

    if (!Widget)
    {
        UE_LOG(LogTemp, Warning, TEXT("CreateWidget failed"));
        return;
    }

    Widget->PartName = PartData.PartName;
    Widget->PartDescription = PartData.PartDescription;
    Widget->PartURL = PartData.PartURL;
    Widget->PartImage = PartData.PartImage;
    Widget->SetObject = HitActor;

    Widget->AddToViewport(20);

    CurrentPartInfoWidget = Widget;
}
void AAssembleLevelManager::SaveAssembly()
{
    if (!PartDatabase)
    {
        UE_LOG(LogTemp, Warning, TEXT("Save failed: PartDatabase is null"));
        return;
    }

    UAssemblySaveGame* SaveGame =
        Cast<UAssemblySaveGame>(
            UGameplayStatics::CreateSaveGameObject(
                UAssemblySaveGame::StaticClass()
            )
        );

    if (!SaveGame) return;

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AAssemblyPart::StaticClass(),
        Actors
    );

    TMap<AActor*, FString> ActorIDMap;
    int32 Index = 0;

    for (AActor* Actor : Actors)
    {
        if (!Actor) continue;
        if (Actor->IsChildActor()) continue;

        FString PartName = GetPartNameFromActor(Actor);

        if (PartName.IsEmpty())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("SKIP SAVE: %s no PartName"),
                *Actor->GetName());
            continue;
        }

        FString SaveID = FString::Printf(TEXT("Part_%d"), Index++);

        FAssemblyPartSaveData Data;
        Data.SaveID = SaveID;
        Data.PartName = PartName;
        Data.Transform = Actor->GetActorTransform();

        SaveGame->SavedParts.Add(Data);
        ActorIDMap.Add(Actor, SaveID);
    }

    for (AActor* Actor : Actors)
    {
        AAssemblyPart* Part = Cast<AAssemblyPart>(Actor);
        if (!Part) continue;
        if (!ActorIDMap.Contains(Actor)) continue;

        TArray<USnapPointComponent*> SnapPoints;
        Part->GetComponents<USnapPointComponent>(SnapPoints);

        for (USnapPointComponent* Point : SnapPoints)
        {
            if (!Point || !Point->bIsConnected || !Point->ConnectedSnapPoint)
            {
                continue;
            }

            AActor* OtherActor =
                Point->ConnectedSnapPoint->GetOwner();

            if (!OtherActor || !ActorIDMap.Contains(OtherActor))
            {
                continue;
            }

            FString ThisID = ActorIDMap[Actor];
            FString OtherID = ActorIDMap[OtherActor];

            if (ThisID > OtherID)
            {
                continue;
            }

            FAssemblySnapConnectionSaveData Conn;
            Conn.PartAID = ThisID;
            Conn.SnapAName = Point->GetFName();
            Conn.PartBID = OtherID;
            Conn.SnapBName = Point->ConnectedSnapPoint->GetFName();
            Conn.bIsSlideConnection = Point->bIsSlideConnection;

            SaveGame->SavedConnections.Add(Conn);
        }
    }

    bool bSuccess =
        UGameplayStatics::SaveGameToSlot(
            SaveGame,
            SaveSlotName,
            0
        );

    UE_LOG(LogTemp, Warning,
        TEXT("SaveAssembly %s, Parts=%d Connections=%d"),
        bSuccess ? TEXT("Success") : TEXT("Failed"),
        SaveGame->SavedParts.Num(),
        SaveGame->SavedConnections.Num());
}
void AAssembleLevelManager::LoadAssembly()
{
    if (!PartDatabase)
    {
        UE_LOG(LogTemp, Warning, TEXT("Load failed: PartDatabase is null"));
        return;
    }

    if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        UE_LOG(LogTemp, Warning, TEXT("No save file found"));
        return;
    }

    UAssemblySaveGame* SaveGame =
        Cast<UAssemblySaveGame>(
            UGameplayStatics::LoadGameFromSlot(
                SaveSlotName,
                0
            )
        );

    if (!SaveGame) return;

    ClosePartInfo();

    TArray<AActor*> CurrentParts;
    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AAssemblyPart::StaticClass(),
        CurrentParts
    );

    for (AActor* Actor : CurrentParts)
    {
        if (Actor)
        {
            Actor->Destroy();
        }
    }

    TMap<FString, AAssemblyPart*> LoadedPartMap;

    for (const FAssemblyPartSaveData& Data : SaveGame->SavedParts)
    {
        FPartInfoData PartData;

        if (!PartDatabase->FindPartByName(Data.PartName, PartData))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Part not found by PartName: %s"),
                *Data.PartName);
            continue;
        }

        if (!PartData.PartActorClass)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ActorClass missing: %s"),
                *Data.PartName);
            continue;
        }

        AAssemblyPart* NewPart =
            GetWorld()->SpawnActor<AAssemblyPart>(
                Cast<UClass>(PartData.PartActorClass),
                Data.Transform
            );

        if (!NewPart)
        {
            continue;
        }

        NewPart->bDisableAutoSnap = true;
        LoadedPartMap.Add(Data.SaveID, NewPart);
    }

    for (const FAssemblySnapConnectionSaveData& Conn : SaveGame->SavedConnections)
    {
        AAssemblyPart** PartAPtr = LoadedPartMap.Find(Conn.PartAID);
        AAssemblyPart** PartBPtr = LoadedPartMap.Find(Conn.PartBID);

        if (!PartAPtr || !PartBPtr) continue;

        AAssemblyPart* PartA = *PartAPtr;
        AAssemblyPart* PartB = *PartBPtr;

        if (!PartA || !PartB) continue;

        USnapPointComponent* SnapA =
            PartA->FindSnapPointByName(Conn.SnapAName);

        USnapPointComponent* SnapB =
            PartB->FindSnapPointByName(Conn.SnapBName);

        if (!SnapA || !SnapB)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Restore connection failed: missing snap point"));
            continue;
        }

        RestoreSnapConnection(
            PartA,
            SnapA,
            PartB,
            SnapB,
            Conn.bIsSlideConnection
        );
    }

    FTimerHandle TimerHandle;

    GetWorld()->GetTimerManager().SetTimer(
        TimerHandle,
        [LoadedPartMap]()
        {
            for (const TPair<FString, AAssemblyPart*>& Pair : LoadedPartMap)
            {
                if (IsValid(Pair.Value))
                {
                    Pair.Value->bDisableAutoSnap = false;
                }
            }
        },
        0.3f,
        false
    );

    UE_LOG(LogTemp, Warning,
        TEXT("LoadAssembly finished. Parts=%d Connections=%d"),
        SaveGame->SavedParts.Num(),
        SaveGame->SavedConnections.Num());
}
void AAssembleLevelManager::RestoreSnapConnection(
    AAssemblyPart* PartA,
    USnapPointComponent* SnapA,
    AAssemblyPart* PartB,
    USnapPointComponent* SnapB,
    bool bIsSlideConnection
)
{
    if (!PartA || !PartB || !SnapA || !SnapB)
    {
        return;
    }

    SnapA->bIsConnected = true;
    SnapB->bIsConnected = true;

    SnapA->bIsSlideConnection = bIsSlideConnection;
    SnapB->bIsSlideConnection = bIsSlideConnection;

    SnapA->ConnectedSnapPoint = SnapB;
    SnapB->ConnectedSnapPoint = SnapA;

    UPlaneConstraintComponent* PlaneA =
        PartA->FindComponentByClass<UPlaneConstraintComponent>();

    UPlaneConstraintComponent* PlaneB =
        PartB->FindComponentByClass<UPlaneConstraintComponent>();

    USlideConstraintComponent* SlideA =
        PartA->FindComponentByClass<USlideConstraintComponent>();

    USlideConstraintComponent* SlideB =
        PartB->FindComponentByClass<USlideConstraintComponent>();

    if (bIsSlideConnection)
    {
        if (PlaneA && SnapA->bUseSlideConstraint)
        {
            PlaneA->SetMovingActorWithSnapPoints(
                PartB,
                SnapB,
                SnapA
            );
            return;
        }

        if (PlaneB && SnapB->bUseSlideConstraint)
        {
            PlaneB->SetMovingActorWithSnapPoints(
                PartA,
                SnapA,
                SnapB
            );
            return;
        }

        if (SlideA && SnapA->bUseSlideConstraint)
        {
            SlideA->SetMovingActorWithSnapPoints(
                PartB,
                SnapB,
                SnapA
            );
            return;
        }

        if (SlideB && SnapB->bUseSlideConstraint)
        {
            SlideB->SetMovingActorWithSnapPoints(
                PartA,
                SnapA,
                SnapB
            );
            return;
        }

        return;
    }

    if (SnapA->SnapRole == ESnapRole::Parent &&
        SnapB->SnapRole == ESnapRole::Child)
    {
        PartB->AttachToActor(
            PartA,
            FAttachmentTransformRules::KeepWorldTransform
        );
    }
    else if (SnapA->SnapRole == ESnapRole::Child &&
        SnapB->SnapRole == ESnapRole::Parent)
    {
        PartA->AttachToActor(
            PartB,
            FAttachmentTransformRules::KeepWorldTransform
        );
    }
}

void AAssembleLevelManager::ShowDeleteAllConfirm()
{
    if (DeleteAllConfirmWidget)
    {
        return;
    }

    if (!DeleteAllConfirmWidgetClass)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("DeleteAllConfirmWidgetClass is null"));
        return;
    }

    DeleteAllConfirmWidget =
        CreateWidget<UDeleteAllConfirmWidget>(
            GetWorld(),
            DeleteAllConfirmWidgetClass
        );

    if (!DeleteAllConfirmWidget)
    {
        return;
    }

    DeleteAllConfirmWidget->AssembleManager = this;
    DeleteAllConfirmWidget->AddToViewport(100);

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (PC)
    {
        PC->SetShowMouseCursor(true);

        FInputModeUIOnly InputMode;
        InputMode.SetWidgetToFocus(
            DeleteAllConfirmWidget->TakeWidget()
        );

        PC->SetInputMode(InputMode);
    }
}
void AAssembleLevelManager::DeleteAllParts()
{
    ClosePartInfo();

    TArray<AActor*> Parts;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AAssemblyPart::StaticClass(),
        Parts
    );

    for (AActor* Actor : Parts)
    {
        AAssemblyPart* Part = Cast<AAssemblyPart>(Actor);

        if (Part)
        {
            Part->ClearAllSnapConnections();
        }
    }

    for (AActor* Actor : Parts)
    {
        if (IsValid(Actor))
        {
            Actor->Destroy();
        }
    }

    HitActor = nullptr;
    LastHoverActor = nullptr;

    CloseDeleteAllConfirm();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Deleted all assembly parts. Count=%d"),
        Parts.Num()
    );
}

void AAssembleLevelManager::CloseDeleteAllConfirm()
{
    if (DeleteAllConfirmWidget)
    {
        DeleteAllConfirmWidget->RemoveFromParent();
        DeleteAllConfirmWidget = nullptr;
    }

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (PC)
    {
        PC->SetShowMouseCursor(true);

        FInputModeGameAndUI InputMode;
        PC->SetInputMode(InputMode);
    }
}
bool AAssembleLevelManager::GetSpawnLocationUnderMouse(
    FVector& OutLocation
) const
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(GetWorld(), 0);

    if (!PC)
    {
        UE_LOG(LogTemp, Error, TEXT("GetSpawnLocation: PC null"));
        return false;
    }

    if (!GEngine || !GEngine->GameViewport)
    {
        UE_LOG(LogTemp, Error, TEXT("GetSpawnLocation: GameViewport null"));
        return false;
    }

    FVector2D MousePosition;

    if (!GEngine->GameViewport->GetMousePosition(MousePosition))
    {
        UE_LOG(LogTemp, Error, TEXT("GetSpawnLocation: viewport mouse position failed"));
        return false;
    }

    FVector WorldOrigin;
    FVector WorldDirection;

    if (!PC->DeprojectScreenPositionToWorld(
        MousePosition.X,
        MousePosition.Y,
        WorldOrigin,
        WorldDirection
    ))
    {
        UE_LOG(LogTemp, Error, TEXT("GetSpawnLocation: deproject failed"));
        return false;
    }

    OutLocation =
        WorldOrigin +
        WorldDirection.GetSafeNormal() * SpawnFallbackDistance;

    return true;
}

void AAssembleLevelManager::BeginSpawnDrag(
    TSubclassOf<AAssemblyPart> PartClass
)
{
    if (!PartClass)
    {
        UE_LOG(LogTemp, Error, TEXT("BeginSpawnDrag: PartClass NULL"));
        return;
    }

    CancelSpawnDrag();

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(GetWorld(), 0);

    if (!PC)
    {
        return;
    }

    FVector CameraLocation;
    FRotator CameraRotation;

    PC->GetPlayerViewPoint(
        CameraLocation,
        CameraRotation
    );

    const FVector InitialLocation =
        CameraLocation +
        CameraRotation.Vector() * SpawnPlaneDistance;

    FActorSpawnParameters SpawnParams;

    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    SpawnPreviewActor =
        GetWorld()->SpawnActor<AAssemblyPart>(
            PartClass,
            InitialLocation,
            FRotator::ZeroRotator,
            SpawnParams
        );

    if (!SpawnPreviewActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Spawn FAILED"));
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Spawn SUCCESS: %s  Location=%s"),
            *SpawnPreviewActor->GetName(),
            *SpawnPreviewActor->GetActorLocation().ToString());
    }


    bIsDraggingSpawnPart = true;

    SpawnPreviewActor->SetActorHiddenInGame(false);
    SpawnPreviewActor->SetActorEnableCollision(false);
    SpawnPreviewActor->SetDragging(true);
    SpawnPreviewActor->bDisableAutoSnap = true;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Begin spawn drag: %s"),
        *SpawnPreviewActor->GetName()
    );
}

void AAssembleLevelManager::UpdateSpawnDrag()
{
    if (!bIsDraggingSpawnPart || !IsValid(SpawnPreviewActor))
    {
        return;
    }

    FVector NewLocation;

    if (!GetSpawnLocationUnderMouse(NewLocation))
    {
        return;
    }

    SpawnPreviewActor->SetActorLocation(
        NewLocation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics
    );
}
void AAssembleLevelManager::ConfirmSpawnDrag()
{
    if (!IsValid(SpawnPreviewActor))
    {
        UE_LOG(LogTemp, Error,
            TEXT("ConfirmSpawnDrag: preview actor invalid"));

        SpawnPreviewActor = nullptr;
        bIsDraggingSpawnPart = false;
        return;
    }

    SpawnPreviewActor->SetActorHiddenInGame(false);
    SpawnPreviewActor->SetActorEnableCollision(true);
    SpawnPreviewActor->SetDragging(false);
    SpawnPreviewActor->bDisableAutoSnap = false;

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Confirm spawn drag: %s Location=%s Scale=%s"),
        *SpawnPreviewActor->GetName(),
        *SpawnPreviewActor->GetActorLocation().ToString(),
        *SpawnPreviewActor->GetActorScale3D().ToString()
    );

    SpawnPreviewActor = nullptr;
    bIsDraggingSpawnPart = false;

}

void AAssembleLevelManager::UpdateSpawnDragFromScreenPosition(
    FVector2D ScreenPosition
)
{
    if (!bIsDraggingSpawnPart || !IsValid(SpawnPreviewActor))
    {
        return;
    }

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(GetWorld(), 0);

    if (!PC)
    {
        return;
    }

    FVector RayOrigin;
    FVector RayDirection;

    if (!PC->DeprojectScreenPositionToWorld(
        ScreenPosition.X,
        ScreenPosition.Y,
        RayOrigin,
        RayDirection
    ))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("UpdateSpawnDragFromScreenPosition: deproject failed")
        );

        return;
    }

    RayDirection.Normalize();

    FVector CameraLocation;
    FRotator CameraRotation;

    PC->GetPlayerViewPoint(
        CameraLocation,
        CameraRotation
    );

    const FVector PlaneNormal =
        CameraRotation.Vector();

    const FVector PlaneOrigin =
        CameraLocation +
        PlaneNormal * SpawnPlaneDistance;

    const FVector RayEnd =
        RayOrigin +
        RayDirection * 100000.f;

    const FPlane DragPlane(
        PlaneOrigin,
        PlaneNormal
    );

    const FVector NewLocation =
        FMath::LinePlaneIntersection(
            RayOrigin,
            RayEnd,
            DragPlane
        );

    SpawnPreviewActor->SetActorLocation(
        NewLocation,
        false,
        nullptr,
        ETeleportType::TeleportPhysics
    );
}
void AAssembleLevelManager::CancelSpawnDrag()
{
    UE_LOG(
        LogTemp,
        Warning,
        TEXT("CancelSpawnDrag called. Preview=%s"),
        SpawnPreviewActor
        ? *SpawnPreviewActor->GetName()
        : TEXT("None")
    );

    if (IsValid(SpawnPreviewActor))
    {
        SpawnPreviewActor->Destroy();
    }

    SpawnPreviewActor = nullptr;
    bIsDraggingSpawnPart = false;
}