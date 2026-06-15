#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "SlideConstraintComponent.generated.h"

class USnapPointComponent;

UENUM(BlueprintType)
enum class ESlideDetachMode : uint8
{
    CanDetach UMETA(DisplayName = "Can Detach"),
    Locked UMETA(DisplayName = "Locked")
};

UCLASS(ClassGroup = (Assembly), meta = (BlueprintSpawnableComponent))
class MYPROJECT6_API USlideConstraintComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    USlideConstraintComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;

    UPROPERTY(BlueprintReadOnly, Category = "Slide")
    AActor* MovingActor = nullptr;

    UPROPERTY()
    AActor* ActorToMove = nullptr;

    UPROPERTY()
    USnapPointComponent* MovingSnapPoint = nullptr;

    UPROPERTY()
    USnapPointComponent* HolderSnapPoint = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide")
    FComponentReference SlideStart;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide")
    FComponentReference SlideEnd;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide")
    ESlideDetachMode DetachMode = ESlideDetachMode::CanDetach;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide")
    float DetachExtraDistance = 30.f;


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide")
    bool bMoveAttachRoot = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide")
    bool bSkipConstraintWhenOwnerMoves = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide")
    bool bIsSliding = false;

    UFUNCTION(BlueprintCallable, Category = "Slide")
    void SetMovingActorWithSnapPoints(
        AActor* NewMovingActor,
        USnapPointComponent* NewMovingSnapPoint,
        USnapPointComponent* NewHolderSnapPoint
    );

    UFUNCTION(BlueprintCallable, Category = "Slide")
    void ClearMovingActor();

    void ApplySlideConstraint();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide")
    bool bDisableChildSlidesWhenActive = false;
private:
    FVector LastSlideStartLocation;
    FVector LastSlideEndLocation;
    bool bHasLastSlideLocations = false;

    AActor* GetTopAttachParent(AActor* Actor) const;
};