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
#include "AssemblyPart.h"
#include "AssemblySaveGame.h"

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
    if (!HitActor) return;

    AAssemblyPart* Part = Cast<AAssemblyPart>(HitActor);

    if (Part)
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

    TArray<AActor*> Parts;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AAssemblyPart::StaticClass(),
        Parts
    );

    for (AActor* Actor : Parts)
    {
        FString PartName =
            GetPartNameFromActor(Actor);

        if (PartName.IsEmpty())
        {
            continue;
        }

        FAssemblyPartSaveData Data;
        Data.PartName = PartName;
        Data.Transform = Actor->GetActorTransform();

        SaveGame->SavedParts.Add(Data);
    }

    bool bSuccess =
        UGameplayStatics::SaveGameToSlot(
            SaveGame,
            SaveSlotName,
            0
        );

    UE_LOG(LogTemp, Warning,
        TEXT("SaveAssembly %s, Count=%d"),
        bSuccess ? TEXT("Success") : TEXT("Failed"),
        SaveGame->SavedParts.Num());
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

    for (const FAssemblyPartSaveData& Data : SaveGame->SavedParts)
    {
        if (!PartDatabase->Parts.Contains(Data.PartName))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Part not found in database: %s"),
                *Data.PartName);
            continue;
        }

        FPartInfoData PartData =
            PartDatabase->Parts[Data.PartName];

        if (!PartData.PartActorClass)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ActorClass missing for: %s"),
                *Data.PartName);
            continue;
        }

        GetWorld()->SpawnActor<AActor>(
            PartData.PartActorClass,
            Data.Transform
        );
    }

    UE_LOG(LogTemp, Warning,
        TEXT("LoadAssembly finished, Count=%d"),
        SaveGame->SavedParts.Num());
}
FString AAssembleLevelManager::GetPartNameFromActor(AActor* Actor) const
{
    if (!Actor || !PartDatabase)
    {
        return TEXT("");
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