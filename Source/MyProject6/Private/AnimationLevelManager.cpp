#include "AnimationLevelManager.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"

#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieSceneSequencePlaybackSettings.h"

#include "PartInfo.h"

AAnimationLevelManager::AAnimationLevelManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAnimationLevelManager::BeginPlay()
{
    Super::BeginPlay();

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

    EnableInput(PC);

    if (PC)
    {
        PC->bEnableMouseOverEvents = true;
        PC->bEnableClickEvents = true;
    }

    if (InputComponent)
    {
        FInputKeyBinding& LeftMouseBinding = InputComponent->BindKey(
            EKeys::LeftMouseButton,
            IE_Pressed,
            this,
            &AAnimationLevelManager::ShowPartInfo
        );
        LeftMouseBinding.bConsumeInput = false;

        InputComponent->BindKey(
            EKeys::Escape,
            IE_Pressed,
            this,
            &AAnimationLevelManager::TogglePauseMenu
        );

        InputComponent->BindKey(
            EKeys::M,
            IE_Pressed,
            this,
            &AAnimationLevelManager::TogglePauseMenu
        );

        InputComponent->BindKey(
            EKeys::Enter,
            IE_Pressed,
            this,
            &AAnimationLevelManager::ToggleAnimation
        );
    }

    if (AnimationSequence)
    {
        FMovieSceneSequencePlaybackSettings Settings;

        SequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
            GetWorld(),
            AnimationSequence,
            Settings,
            SequenceActor
        );

        if (SequencePlayer)
        {
            FMovieSceneSequencePlaybackParams Params;
            Params.Frame = FFrameTime(7000);
            Params.PositionType = EMovieScenePositionType::Frame;
            Params.UpdateMethod = EUpdatePositionMethod::Jump;

            SequencePlayer->SetPlaybackPosition(Params);
            SequencePlayer->Pause();

            bIsPlaying = false;

            UE_LOG(LogTemp, Warning, TEXT("Sequence set to frame 7000"));
        }
    }

    if (SliderWidgetClass)
    {
        SliderWidget = CreateWidget<UUserWidget>(
            GetWorld(),
            SliderWidgetClass
        );

        if (SliderWidget)
        {
            SliderWidget->AddToViewport();
        }
    }
}

void AAnimationLevelManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TraceMouse();
}

void AAnimationLevelManager::TraceMouse()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
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
        }

        LastHoverActor = NewHitActor;
    }

    HitActor = NewHitActor;
}

void AAnimationLevelManager::HoverActor(AActor* NewActor)
{
    if (!NewActor || !HoverOverlayMaterial) return;

    UStaticMeshComponent* MeshComp =
        NewActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp) return;

    MeshComp->bDisallowNanite = true;
    MeshComp->MarkRenderStateDirty();

    MeshComp->SetOverlayMaterial(HoverOverlayMaterial);
}

void AAnimationLevelManager::UnhoverActor(AActor* OldActor)
{
    if (!OldActor) return;

    UStaticMeshComponent* MeshComp =
        OldActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp) return;

    MeshComp->SetOverlayMaterial(nullptr);

    MeshComp->bDisallowNanite = false;
    MeshComp->MarkRenderStateDirty();
}

void AAnimationLevelManager::ShowPartInfo()
{
    if (!HitActor)
    {
        ClosePartInfo();
        return;
    }
    if (!HitActor || !PartInfoWidgetClass) return;

    UStaticMeshComponent* MeshComp =
        HitActor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp || !MeshComp->GetStaticMesh()) return;

    FString MeshName = MeshComp->GetStaticMesh()->GetName();

    FPartInfoData PartData;

    if (!PartDatabase || !PartDatabase->FindPartInfo(MeshName, PartData))
    {
        UE_LOG(LogTemp, Warning, TEXT("No PartData found for: %s"), *MeshName);
        return;
    }

    if (CurrentPartInfoWidget)
    {
        CurrentPartInfoWidget->RemoveFromParent();
        CurrentPartInfoWidget = nullptr;
    }

    UPartInfo* Widget = CreateWidget<UPartInfo>(
        GetWorld(),
        PartInfoWidgetClass
    );

    if (!Widget) return;

    Widget->PartName = PartData.PartName;
    Widget->PartDescription = PartData.PartDescription;
    Widget->PartURL = PartData.PartURL;
    Widget->PartImage = PartData.PartImage;

    Widget->AddToViewport();

    CurrentPartInfoWidget = Widget;
}

void AAnimationLevelManager::TogglePauseMenu()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    if (!PauseWidget)
    {
        PauseWidget = CreateWidget<UUserWidget>(
            GetWorld(),
            PauseWidgetClass
        );

        if (!PauseWidget) return;

        PauseWidget->AddToViewport();

        if (SequencePlayer && SequencePlayer->IsPlaying())
        {
            SequencePlayer->Pause();
            bIsPlaying = false;
        }

        PC->SetShowMouseCursor(true);
        PC->SetInputMode(FInputModeUIOnly());
    }
    else
    {
        PauseWidget->RemoveFromParent();
        PauseWidget = nullptr;

        PC->SetInputMode(FInputModeGameOnly());
        PC->SetShowMouseCursor(false);
    }
}

void AAnimationLevelManager::ToggleAnimation()
{
    if (!SequencePlayer) return;

    if (SequencePlayer->IsPlaying())
    {
        SequencePlayer->Pause();
        bIsPlaying = false;
    }
    else
    {
        SequencePlayer->Play();
        bIsPlaying = true;
    }
}
void AAnimationLevelManager::ClosePartInfo()
{
    if (CurrentPartInfoWidget)
    {
        CurrentPartInfoWidget->RemoveFromParent();
        CurrentPartInfoWidget = nullptr;
    }
}