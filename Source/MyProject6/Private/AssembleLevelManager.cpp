#include "AssembleLevelManager.h"
#include "AssemblyPart.h"
#include "PartDatabase.h"
#include "PartInfo.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"

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
        InputComponent->BindKey(
            EKeys::E,
            IE_Pressed,
            this,
            &AAssembleLevelManager::BeginDrag
        );

        InputComponent->BindKey(
            EKeys::E,
            IE_Released,
            this,
            &AAssembleLevelManager::EndDrag
        );

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
}

void AAssembleLevelManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TraceMouse();

    if (bIsDragging)
    {
        UpdateDrag();
    }
}

void AAssembleLevelManager::TraceMouse()
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC) return;

    float MouseX = 0.f;
    float MouseY = 0.f;

    if (!PC->GetMousePosition(MouseX, MouseY)) return;

    FVector WorldLocation;
    FVector WorldDirection;

    PC->DeprojectScreenPositionToWorld(
        MouseX,
        MouseY,
        WorldLocation,
        WorldDirection
    );

    FVector Start = WorldLocation;
    FVector End = Start + WorldDirection * 2000.f;

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

void AAssembleLevelManager::BeginDrag()
{
    if (!HitActor) return;

    MoveActor = HitActor;
    bIsDragging = true;

    DragPlaneZ = MoveActor->GetActorLocation().Z;

    FVector MousePoint;

    if (GetMousePointOnDragPlane(MousePoint))
    {
        DragOffset =
            MoveActor->GetActorLocation() - MousePoint;
    }
    else
    {
        DragOffset = FVector::ZeroVector;
    }

    AAssemblyPart* Part =
        Cast<AAssemblyPart>(MoveActor);

    if (Part)
    {
        Part->SetDragging(true);
    }

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (PC)
    {
        float MouseX = 0.f;
        float MouseY = 0.f;

        PC->GetMousePosition(MouseX, MouseY);

        DragStartMouseY = MouseY;
        DragStartActorZ = MoveActor->GetActorLocation().Z;
    }
}

void AAssembleLevelManager::EndDrag()
{
    bIsDragging = false;

    AAssemblyPart* Part =
        Cast<AAssemblyPart>(MoveActor);

    if (Part)
    {
        Part->SetDragging(false);
    }

    MoveActor = nullptr;
}

void AAssembleLevelManager::UpdateDrag()
{
    if (!MoveActor) return;

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC) return;

    bool bVerticalMode =
        PC->IsInputKeyDown(EKeys::LeftShift) ||
        PC->IsInputKeyDown(EKeys::RightShift);

    if (bVerticalMode)
    {
        float MouseX = 0.f;
        float MouseY = 0.f;

        if (!PC->GetMousePosition(MouseX, MouseY)) return;

        float DeltaY =
            DragStartMouseY - MouseY;

        FVector NewLocation =
            MoveActor->GetActorLocation();

        NewLocation.Z =
            DragStartActorZ + DeltaY * VerticalDragSpeed;

        MoveActor->SetActorLocation(NewLocation);
    }
    else
    {
        FVector MousePoint;

        if (!GetMousePointOnDragPlane(MousePoint)) return;

        FVector NewLocation =
            MousePoint + DragOffset;

        MoveActor->SetActorLocation(NewLocation);
    }
}

bool AAssembleLevelManager::GetMousePointOnDragPlane(
    FVector& OutPoint
) const
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(GetWorld(), 0);

    if (!PC) return false;

    float MouseX = 0.f;
    float MouseY = 0.f;

    if (!PC->GetMousePosition(MouseX, MouseY)) return false;

    FVector WorldLocation;
    FVector WorldDirection;

    PC->DeprojectScreenPositionToWorld(
        MouseX,
        MouseY,
        WorldLocation,
        WorldDirection
    );

    if (FMath::IsNearlyZero(WorldDirection.Z))
    {
        return false;
    }

    float T =
        (DragPlaneZ - WorldLocation.Z) / WorldDirection.Z;

    if (T < 0.f)
    {
        return false;
    }

    OutPoint =
        WorldLocation + WorldDirection * T;

    return true;
}

void AAssembleLevelManager::DeleteSelected()
{
    AActor* ActorToDelete =
        MoveActor ? MoveActor : HitActor;

    if (!ActorToDelete) return;

    ActorToDelete->Destroy();

    if (ActorToDelete == HitActor)
    {
        HitActor = nullptr;
    }

    if (ActorToDelete == LastHoverActor)
    {
        LastHoverActor = nullptr;
    }

    if (ActorToDelete == MoveActor)
    {
        MoveActor = nullptr;
        bIsDragging = false;
    }
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

void AAssembleLevelManager::ShowPartInfo()
{
    UE_LOG(LogTemp, Warning, TEXT("ShowPartInfo called"));

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

    UE_LOG(LogTemp, Warning, TEXT("PartInfo Widget Added"));
}
void AAssembleLevelManager::ClosePartInfo()
{
    if (CurrentPartInfoWidget)
    {
        CurrentPartInfoWidget->RemoveFromParent();
        CurrentPartInfoWidget = nullptr;
    }
}