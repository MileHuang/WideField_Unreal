#include "AssembleLevelManager.h"

#include "AssemblyPart.h"
#include "AssemblySaveGame.h"
#include "DeleteAllConfirmWidget.h"
#include "PartDatabase.h"
#include "PartInfo.h"
#include "PlaneConstraintComponent.h"
#include "SlideConstraintComponent.h"
#include "SnapPointComponent.h"

#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace AssembleLevelManagerNames
{
    const FName HideFocus(TEXT("HideFocus"));
    const FName ShutdownLaser(TEXT("ShutdownLaser"));
    const FName DeactiveLaser(TEXT("DeactiveLaser"));
    const FName DeactivateLaser(TEXT("DeactivateLaser"));
    const FName TraceOnly(TEXT("TraceOnly"));
}

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
        InputComponent->BindKey(
            EKeys::Delete,
            IE_Pressed,
            this,
            &AAssembleLevelManager::DeleteSelected
        );

        InputComponent->BindKey(
            EKeys::Escape,
            IE_Pressed,
            this,
            &AAssembleLevelManager::TogglePauseMenu
        );
        InputComponent->BindKey(
            EKeys::M,
            IE_Pressed,
            this,
            &AAssembleLevelManager::TogglePauseMenu
        );

        InputComponent->BindKey(
            EKeys::L,
            IE_Pressed,
            this,
            &AAssembleLevelManager::ToggleLaser
        );
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

    const FString MeshName =
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

    if (!PC)
    {
        return;
    }

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

    const FVector Start = WorldLocation;
    const FVector End =
        Start + WorldDirection * TraceDistance;

    FHitResult Hit;

    const bool bHit =
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
    if (!NewActor || !HoverOverlayMaterial)
    {
        return;
    }

    UStaticMeshComponent* MeshComp =
        NewActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp)
    {
        return;
    }

    MeshComp->bDisallowNanite = true;
    MeshComp->MarkRenderStateDirty();
    MeshComp->SetOverlayMaterial(HoverOverlayMaterial);
}

void AAssembleLevelManager::UnhoverActor(AActor* OldActor)
{
    if (!OldActor)
    {
        return;
    }

    UStaticMeshComponent* MeshComp =
        OldActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp)
    {
        return;
    }

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

    if (!IsValid(HitActor))
    {
        return;
    }

    AActor* ActorToDelete = HitActor;

    ClosePartInfo();

    if (LastHoverActor == ActorToDelete)
    {
        UnhoverActor(ActorToDelete);
    }

    HitActor = nullptr;
    LastHoverActor = nullptr;

    DestroyPartAndAssociatedLasers(ActorToDelete);
}

void AAssembleLevelManager::TogglePauseMenu()
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC)
    {
        return;
    }

    if (!PauseWidget)
    {
        if (!PauseWidgetClass)
        {
            return;
        }

        PauseWidget =
            CreateWidget<UUserWidget>(
                GetWorld(),
                PauseWidgetClass
            );

        if (!PauseWidget)
        {
            return;
        }

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
        if (!SpawnWidgetClass)
        {
            return;
        }

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

void AAssembleLevelManager::CallNoParamFunction(
    UObject* Target,
    FName FunctionName
) const
{
    if (!IsValid(Target))
    {
        return;
    }

    if (UFunction* Function = Target->FindFunction(FunctionName))
    {
        Target->ProcessEvent(Function, nullptr);
    }
}

bool AAssembleLevelManager::GetBoolPropertyByName(
    const UObject* Object,
    FName PropertyName,
    bool& OutValue
) const
{
    if (!IsValid(Object))
    {
        return false;
    }

    const FBoolProperty* BoolProperty =
        FindFProperty<FBoolProperty>(
            Object->GetClass(),
            PropertyName
        );

    if (!BoolProperty)
    {
        return false;
    }

    OutValue =
        BoolProperty->GetPropertyValue_InContainer(Object);

    return true;
}

void AAssembleLevelManager::HideAllFocusCones()
{
    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Actor = *It;

        if (
            IsValid(Actor) &&
            Actor->FindFunction(AssembleLevelManagerNames::HideFocus)
            )
        {
            CallNoParamFunction(
                Actor,
                AssembleLevelManagerNames::HideFocus
            );
        }
    }
}

void AAssembleLevelManager::SetLaserSystemEnabled(bool bEnabled)
{
    if (!LaserClass)
    {
        return;
    }

    TArray<AActor*> Lasers;

    UGameplayStatics::GetAllActorsOfClass(
        this,
        LaserClass,
        Lasers
    );

    if (!bEnabled)
    {
        // Hiding alone is insufficient because an active Tick would continue
        // tracing and would immediately recreate the cones.
        for (AActor* Laser : Lasers)
        {
            if (!IsValid(Laser))
            {
                continue;
            }

            Laser->SetActorTickEnabled(false);
            Laser->SetActorHiddenInGame(true);
        }

        // BP_FocusLen::HideFocus hides both cones and shuts down its
        // DivergingTraceLaser / ParallelOutputLaser chain.
        HideAllFocusCones();
        return;
    }

    for (AActor* Laser : Lasers)
    {
        if (!IsValid(Laser))
        {
            continue;
        }

        bool bTraceOnly = false;
        GetBoolPropertyByName(
            Laser,
            AssembleLevelManagerNames::TraceOnly,
            bTraceOnly
        );

        // TraceOnly lasers must continue tracing but must stay invisible.
        Laser->SetActorHiddenInGame(bTraceOnly);
        Laser->SetActorTickEnabled(true);
    }
}

void AAssembleLevelManager::HideAllLasers()
{
    SetLaserSystemEnabled(false);

    // Preserve the original state convention: the next press shows lasers.
    bLaserPressed = true;
}

void AAssembleLevelManager::ToggleLaser()
{
    const bool bEnableLaserSystem = bLaserPressed;

    SetLaserSystemEnabled(bEnableLaserSystem);

    bLaserPressed = !bLaserPressed;
}

bool AAssembleLevelManager::IsLaserActor(const AActor* Actor) const
{
    return
        IsValid(Actor) &&
        LaserClass &&
        Actor->IsA(LaserClass);
}

void AAssembleLevelManager::CollectReferencedLaserActors(
    UObject* Object,
    TSet<AActor*>& OutLasers
) const
{
    if (!IsValid(Object) || !LaserClass)
    {
        return;
    }

    // This finds Blueprint object-reference variables such as AttachedLaser,
    // DivergingTraceLaser and ParallelOutputLaser without requiring a direct
    // C++ dependency on those Blueprint classes.
    for (
        TFieldIterator<FObjectPropertyBase> PropertyIt(
            Object->GetClass(),
            EFieldIteratorFlags::IncludeSuper
        );
        PropertyIt;
        ++PropertyIt
        )
    {
        const FObjectPropertyBase* ObjectProperty = *PropertyIt;

        if (!ObjectProperty)
        {
            continue;
        }

        UObject* ReferencedObject =
            ObjectProperty->GetObjectPropertyValue_InContainer(Object);

        AActor* ReferencedActor =
            Cast<AActor>(ReferencedObject);

        if (IsLaserActor(ReferencedActor))
        {
            OutLasers.Add(ReferencedActor);
        }
    }
}

void AAssembleLevelManager::DeactivateLaserOnObject(UObject* Object)
{
    if (!IsValid(Object))
    {
        return;
    }

    if (Object->FindFunction(AssembleLevelManagerNames::DeactiveLaser))
    {
        CallNoParamFunction(
            Object,
            AssembleLevelManagerNames::DeactiveLaser
        );
        return;
    }

    if (Object->FindFunction(AssembleLevelManagerNames::DeactivateLaser))
    {
        CallNoParamFunction(
            Object,
            AssembleLevelManagerNames::DeactivateLaser
        );
    }
}

void AAssembleLevelManager::ShutdownOrDestroyLaser(AActor* LaserActor)
{
    if (!IsValid(LaserActor))
    {
        return;
    }

    if (LaserActor->FindFunction(AssembleLevelManagerNames::ShutdownLaser))
    {
        CallNoParamFunction(
            LaserActor,
            AssembleLevelManagerNames::ShutdownLaser
        );
    }

    // ShutdownLaser may already have destroyed or marked the actor pending kill.
    if (IsValid(LaserActor))
    {
        LaserActor->Destroy();
    }
}

void AAssembleLevelManager::DestroyAssociatedLasers(AActor* OwnerActor)
{
    if (!IsValid(OwnerActor) || !LaserClass)
    {
        return;
    }

    TSet<AActor*> LasersToDestroy;

    if (IsLaserActor(OwnerActor))
    {
        LasersToDestroy.Add(OwnerActor);
    }

    // Capture Blueprint references before DeactiveLaser clears AttachedLaser.
    CollectReferencedLaserActors(
        OwnerActor,
        LasersToDestroy
    );

    TInlineComponentArray<UActorComponent*> Components;
    OwnerActor->GetComponents(Components);

    for (UActorComponent* Component : Components)
    {
        CollectReferencedLaserActors(
            Component,
            LasersToDestroy
        );
    }

    // Also include laser actors attached under this part.
    TArray<AActor*> AttachedActors;
    OwnerActor->GetAttachedActors(
        AttachedActors,
        true,
        true
    );

    for (AActor* AttachedActor : AttachedActors)
    {
        if (IsLaserActor(AttachedActor))
        {
            LasersToDestroy.Add(AttachedActor);
        }
    }

    // Include lasers spawned with Owner set to this part, or attached through
    // a deeper attachment chain.
    TArray<AActor*> AllLasers;

    UGameplayStatics::GetAllActorsOfClass(
        this,
        LaserClass,
        AllLasers
    );

    for (AActor* Laser : AllLasers)
    {
        if (!IsValid(Laser))
        {
            continue;
        }

        bool bBelongsToOwner =
            Laser->GetOwner() == OwnerActor;

        AActor* AttachParent =
            Laser->GetAttachParentActor();

        while (!bBelongsToOwner && AttachParent)
        {
            if (AttachParent == OwnerActor)
            {
                bBelongsToOwner = true;
                break;
            }

            AttachParent =
                AttachParent->GetAttachParentActor();
        }

        if (bBelongsToOwner)
        {
            LasersToDestroy.Add(Laser);
        }
    }

    // Let SC_Emitter perform its own normal cleanup first.
    DeactivateLaserOnObject(OwnerActor);

    for (UActorComponent* Component : Components)
    {
        DeactivateLaserOnObject(Component);
    }

    // The explicit pass is a safety net in case a Blueprint component cleared
    // its reference but failed to destroy the associated A_Laser actor.
    for (AActor* Laser : LasersToDestroy)
    {
        ShutdownOrDestroyLaser(Laser);
    }
}

void AAssembleLevelManager::DestroyPartAndAssociatedLasers(AActor* Actor)
{
    if (!IsValid(Actor))
    {
        return;
    }

    if (AAssemblyPart* Part = Cast<AAssemblyPart>(Actor))
    {
        Part->ClearAllSnapConnections();
    }

    DestroyAssociatedLasers(Actor);

    if (IsValid(Actor))
    {
        Actor->Destroy();
    }
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
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("PartInfoWidgetClass is null")
        );
        return;
    }

    if (!PartDatabase)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("PartDatabase is null")
        );
        return;
    }

    UStaticMeshComponent* MeshComp =
        HitActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp || !MeshComp->GetStaticMesh())
    {
        ClosePartInfo();
        return;
    }

    const FString MeshName =
        MeshComp->GetStaticMesh()->GetName();

    FPartInfoData PartData;

    if (!PartDatabase->FindPartInfo(MeshName, PartData))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("No PartData found for %s"),
            *MeshName
        );

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
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("CreateWidget failed")
        );
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
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Save failed: PartDatabase is null")
        );
        return;
    }

    UAssemblySaveGame* SaveGame =
        Cast<UAssemblySaveGame>(
            UGameplayStatics::CreateSaveGameObject(
                UAssemblySaveGame::StaticClass()
            )
        );

    if (!SaveGame)
    {
        return;
    }

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
        if (!Actor)
        {
            continue;
        }

        if (Actor->IsChildActor())
        {
            continue;
        }

        const FString PartName =
            GetPartNameFromActor(Actor);

        if (PartName.IsEmpty())
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("SKIP SAVE: %s no PartName"),
                *Actor->GetName()
            );
            continue;
        }

        const FString SaveID =
            FString::Printf(TEXT("Part_%d"), Index++);

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

        if (!Part)
        {
            continue;
        }

        if (!ActorIDMap.Contains(Actor))
        {
            continue;
        }

        TArray<USnapPointComponent*> SnapPoints;
        Part->GetComponents<USnapPointComponent>(SnapPoints);

        for (USnapPointComponent* Point : SnapPoints)
        {
            if (
                !Point ||
                !Point->bIsConnected ||
                !Point->ConnectedSnapPoint
                )
            {
                continue;
            }

            AActor* OtherActor =
                Point->ConnectedSnapPoint->GetOwner();

            if (
                !OtherActor ||
                !ActorIDMap.Contains(OtherActor)
                )
            {
                continue;
            }

            const FString ThisID = ActorIDMap[Actor];
            const FString OtherID = ActorIDMap[OtherActor];

            if (ThisID > OtherID)
            {
                continue;
            }

            FAssemblySnapConnectionSaveData Conn;
            Conn.PartAID = ThisID;
            Conn.SnapAName = Point->GetFName();
            Conn.PartBID = OtherID;
            Conn.SnapBName =
                Point->ConnectedSnapPoint->GetFName();
            Conn.bIsSlideConnection =
                Point->bIsSlideConnection;

            SaveGame->SavedConnections.Add(Conn);
        }
    }

    const bool bSuccess =
        UGameplayStatics::SaveGameToSlot(
            SaveGame,
            SaveSlotName,
            0
        );

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("SaveAssembly %s, Parts=%d Connections=%d"),
        bSuccess ? TEXT("Success") : TEXT("Failed"),
        SaveGame->SavedParts.Num(),
        SaveGame->SavedConnections.Num()
    );
}

void AAssembleLevelManager::LoadAssembly()
{
    if (!PartDatabase)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("Load failed: PartDatabase is null")
        );
        return;
    }

    if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("No save file found")
        );
        return;
    }

    UAssemblySaveGame* SaveGame =
        Cast<UAssemblySaveGame>(
            UGameplayStatics::LoadGameFromSlot(
                SaveSlotName,
                0
            )
        );

    if (!SaveGame)
    {
        return;
    }

    ClosePartInfo();
    HideAllFocusCones();

    TArray<AActor*> CurrentParts;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AAssemblyPart::StaticClass(),
        CurrentParts
    );

    for (AActor* Actor : CurrentParts)
    {
        DestroyPartAndAssociatedLasers(Actor);
    }

    TMap<FString, AAssemblyPart*> LoadedPartMap;

    for (const FAssemblyPartSaveData& Data : SaveGame->SavedParts)
    {
        FPartInfoData PartData;

        if (!PartDatabase->FindPartByName(Data.PartName, PartData))
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Part not found by PartName: %s"),
                *Data.PartName
            );
            continue;
        }

        if (!PartData.PartActorClass)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("ActorClass missing: %s"),
                *Data.PartName
            );
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

    for (
        const FAssemblySnapConnectionSaveData& Conn :
        SaveGame->SavedConnections
        )
    {
        AAssemblyPart** PartAPtr =
            LoadedPartMap.Find(Conn.PartAID);
        AAssemblyPart** PartBPtr =
            LoadedPartMap.Find(Conn.PartBID);

        if (!PartAPtr || !PartBPtr)
        {
            continue;
        }

        AAssemblyPart* PartA = *PartAPtr;
        AAssemblyPart* PartB = *PartBPtr;

        if (!PartA || !PartB)
        {
            continue;
        }

        USnapPointComponent* SnapA =
            PartA->FindSnapPointByName(Conn.SnapAName);
        USnapPointComponent* SnapB =
            PartB->FindSnapPointByName(Conn.SnapBName);

        if (!SnapA || !SnapB)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("Restore connection failed: missing snap point")
            );
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
            for (
                const TPair<FString, AAssemblyPart*>& Pair :
                LoadedPartMap
                )
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

    // When the global laser state is off, newly loaded LaserEmitter parts may
    // spawn a laser during BeginPlay. Hide those new lasers on the next tick.
    if (bLaserPressed)
    {
        GetWorldTimerManager().SetTimerForNextTick(
            this,
            &AAssembleLevelManager::HideAllLasers
        );
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("LoadAssembly finished. Parts=%d Connections=%d"),
        SaveGame->SavedParts.Num(),
        SaveGame->SavedConnections.Num()
    );
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

    if (
        SnapA->SnapRole == ESnapRole::Parent &&
        SnapB->SnapRole == ESnapRole::Child
        )
    {
        PartB->AttachToActor(
            PartA,
            FAttachmentTransformRules::KeepWorldTransform
        );
    }
    else if (
        SnapA->SnapRole == ESnapRole::Child &&
        SnapB->SnapRole == ESnapRole::Parent
        )
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
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("DeleteAllConfirmWidgetClass is null")
        );
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
    HideAllFocusCones();

    TArray<AActor*> Parts;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AAssemblyPart::StaticClass(),
        Parts
    );

    // Clear all snap references before any actor is destroyed.
    for (AActor* Actor : Parts)
    {
        if (AAssemblyPart* Part = Cast<AAssemblyPart>(Actor))
        {
            Part->ClearAllSnapConnections();
        }
    }

    for (AActor* Actor : Parts)
    {
        if (IsValid(Actor))
        {
            DestroyAssociatedLasers(Actor);

            if (IsValid(Actor))
            {
                Actor->Destroy();
            }
        }
    }

    HitActor = nullptr;
    LastHoverActor = nullptr;

    CloseDeleteAllConfirm();

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Deleted all assembly parts and associated lasers. Count=%d"),
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
        UE_LOG(
            LogTemp,
            Error,
            TEXT("GetSpawnLocation: PC null")
        );
        return false;
    }

    if (!GEngine || !GEngine->GameViewport)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("GetSpawnLocation: GameViewport null")
        );
        return false;
    }

    FVector2D MousePosition;

    if (!GEngine->GameViewport->GetMousePosition(MousePosition))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("GetSpawnLocation: viewport mouse position failed")
        );
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
        UE_LOG(
            LogTemp,
            Error,
            TEXT("GetSpawnLocation: deproject failed")
        );
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
        UE_LOG(
            LogTemp,
            Error,
            TEXT("BeginSpawnDrag: PartClass NULL")
        );
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
        return;
    }

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("Spawn SUCCESS: %s  Location=%s"),
        *SpawnPreviewActor->GetName(),
        *SpawnPreviewActor->GetActorLocation().ToString()
    );

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

    // A LaserEmitter preview can spawn its laser during BeginPlay. Keep that
    // newly spawned laser off when the global laser system is currently off.
    if (bLaserPressed)
    {
        GetWorldTimerManager().SetTimerForNextTick(
            this,
            &AAssembleLevelManager::HideAllLasers
        );
    }
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
        UE_LOG(
            LogTemp,
            Error,
            TEXT("ConfirmSpawnDrag: preview actor invalid")
        );

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
        DestroyPartAndAssociatedLasers(SpawnPreviewActor);
    }

    SpawnPreviewActor = nullptr;
    bIsDraggingSpawnPart = false;
}
