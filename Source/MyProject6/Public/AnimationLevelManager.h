#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PartDatabase.h"
#include "AnimationLevelManager.generated.h"

class UUserWidget;
class UMaterialInterface;
class ULevelSequence;
class ULevelSequencePlayer;
class ALevelSequenceActor;

UCLASS()
class MYPROJECT6_API AAnimationLevelManager : public AActor
{
    GENERATED_BODY()

public:
    AAnimationLevelManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY()
    AActor* HitActor;

    UPROPERTY()
    AActor* LastHoverActor;

    UPROPERTY()
    UUserWidget* CurrentPartInfoWidget;

    UPROPERTY()
    UUserWidget* PauseWidget;

    UPROPERTY()
    UUserWidget* SliderWidget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover")
    UMaterialInterface* HoverOverlayMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> PartInfoWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> PauseWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> SliderWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parts")
    UPartDatabase* PartDatabase;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    ULevelSequence* AnimationSequence;

    UPROPERTY()
    ULevelSequencePlayer* SequencePlayer;

    UPROPERTY()
    ALevelSequenceActor* SequenceActor;

    UPROPERTY(BlueprintReadWrite, Category = "Animation")
    bool bIsPlaying = false;

    void TraceMouse();
    void HoverActor(AActor* NewActor);
    void UnhoverActor(AActor* OldActor);

    void ShowPartInfo();
    void TogglePauseMenu();
    void ToggleAnimation();
};