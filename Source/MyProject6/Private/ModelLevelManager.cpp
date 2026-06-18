#include "ModelLevelManager.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "PartInfo.h"
AModelLevelManager::AModelLevelManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AModelLevelManager::BeginPlay()
{
    Super::BeginPlay();

    GetWorldTimerManager().SetTimerForNextTick(
        this,
        &AModelLevelManager::HideAllLasers
    );

    EnableInput(UGameplayStatics::GetPlayerController(this, 0));

    if (InputComponent)
    {
        InputComponent->BindAction(
            "Laser Emit",
            IE_Pressed,
            this,
            &AModelLevelManager::ToggleLaser
        );

        FInputKeyBinding& LeftMouseBinding = InputComponent->BindKey(
            EKeys::LeftMouseButton,
            IE_Pressed,
            this,
            &AModelLevelManager::ShowPartInfo
        );

        LeftMouseBinding.bConsumeInput = false;

        InputComponent->BindKey(
            EKeys::Escape,
            IE_Pressed,
            this,
            &AModelLevelManager::TogglePauseMenu
        );

        InputComponent->BindKey(
            EKeys::M,
            IE_Pressed,
            this,
            &AModelLevelManager::TogglePauseMenu
        );
    }
    if (TipsWidgetClass)
    {
        TipsWidget = CreateWidget<UUserWidget>(
            GetWorld(),
            TipsWidgetClass
        );

        if (TipsWidget)
        {
            TipsWidget->AddToViewport();
        }
    }

}

void AModelLevelManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TraceMouse();
}

void AModelLevelManager::TraceMouse()
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
    FVector End = Start + (WorldDirection * 2000.f);

    FHitResult Hit;

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility
    );

    AActor* NewHitActor = bHit ? Hit.GetActor() : nullptr;

    if (NewHitActor != LastHoverActor)
    {
        if (LastHoverActor)
        {
            UnhoverActor(LastHoverActor);
        }

        if (NewHitActor)
        {
            HoverActor(NewHitActor);
            UE_LOG(LogTemp, Warning, TEXT("Hover Actor: %s"), *NewHitActor->GetName());
        }

        LastHoverActor = NewHitActor;
    }

    HitActor = NewHitActor;
}

void AModelLevelManager::HoverActor(AActor* NewActor)
{
    if (!NewActor || !HoverOverlayMaterial) return;

    UStaticMeshComponent* MeshComp =
        NewActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp) return;

    MeshComp->bDisallowNanite = true;
    MeshComp->MarkRenderStateDirty();

    MeshComp->SetOverlayMaterial(HoverOverlayMaterial);
}

void AModelLevelManager::UnhoverActor(AActor* OldActor)
{
    if (!OldActor) return;

    UStaticMeshComponent* MeshComp =
        OldActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp) return;

    MeshComp->SetOverlayMaterial(nullptr);

    MeshComp->bDisallowNanite = false;
    MeshComp->MarkRenderStateDirty();
}

void AModelLevelManager::HideAllLasers()
{
    if (!LaserClass) return;

    TArray<AActor*> Lasers;
    UGameplayStatics::GetAllActorsOfClass(this, LaserClass, Lasers);

    for (AActor* Laser : Lasers)
    {
        if (Laser)
        {
            Laser->SetActorHiddenInGame(true);
        }
    }

    bLaserPressed = true;
}

void AModelLevelManager::ToggleLaser()
{
    if (!LaserClass) return;

    TArray<AActor*> Lasers;
    UGameplayStatics::GetAllActorsOfClass(this, LaserClass, Lasers);

    for (AActor* Laser : Lasers)
    {
        if (Laser)
        {
            Laser->SetActorHiddenInGame(!bLaserPressed);
        }
    }

    bLaserPressed = !bLaserPressed;
}

void AModelLevelManager::ShowPartInfo()
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

    UStaticMeshComponent* MeshComp =
        HitActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp || !MeshComp->GetStaticMesh())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("No StaticMesh found on Actor: %s"),
            *HitActor->GetName());
        return;
    }

    FString MeshName =
        MeshComp->GetStaticMesh()->GetName();

    FPartInfoData PartData;

    if (PartDatabase &&
        PartDatabase->FindPartInfo(
            MeshName,
            PartData))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Found Part: %s"),
            *PartData.PartName);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("No PartData found for Mesh: %s"),
            *MeshName);

        return;
    }

    if (CurrentPartInfoWidget)
    {
        CurrentPartInfoWidget->RemoveFromParent();
        CurrentPartInfoWidget = nullptr;
    }

    UPartInfo* Widget =
        CreateWidget<UPartInfo>(
            GetWorld(),
            PartInfoWidgetClass
        );

    if (!Widget)
    {
        return;
    }

    Widget->PartName = PartData.PartName;
    Widget->PartDescription = PartData.PartDescription;
    Widget->PartURL = PartData.PartURL;
    Widget->PartImage = PartData.PartImage;

    Widget->AddToViewport();

    CurrentPartInfoWidget = Widget;

    UE_LOG(LogTemp, Warning,
        TEXT("Create widget for Mesh: %s"),
        *MeshName);
}

void AModelLevelManager::TogglePauseMenu()
{
    APlayerController* PC =
        UGameplayStatics::GetPlayerController(this, 0);

    if (!PC) return;
    if (!PauseWidget)
    {
        PauseWidget =
            CreateWidget<UUserWidget>(
                GetWorld(),
                PauseWidgetClass
            );

        if (!PauseWidget) return;

        PauseWidget->AddToViewport();

        UGameplayStatics::SetGamePaused(
            GetWorld(),
            true
        );

        PC->SetShowMouseCursor(true);

        PC->SetInputMode(FInputModeUIOnly());
    }
    else
    {
        PauseWidget->RemoveFromParent();

        PauseWidget = nullptr;

        UGameplayStatics::SetGamePaused(
            GetWorld(),
            false
        );

        PC->SetInputMode(FInputModeGameOnly());

        PC->SetShowMouseCursor(false);
    }
}
void AModelLevelManager::ClosePartInfo()
{
    if (CurrentPartInfoWidget)
    {
        CurrentPartInfoWidget->RemoveFromParent();
        CurrentPartInfoWidget = nullptr;
    }
}