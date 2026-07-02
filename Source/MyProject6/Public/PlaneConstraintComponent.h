#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "PlaneConstraintComponent.generated.h"

class USnapPointComponent;

UCLASS(ClassGroup = (Assembly), meta = (BlueprintSpawnableComponent))
class MYPROJECT6_API UPlaneConstraintComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UPlaneConstraintComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction
    ) override;

    UPROPERTY(BlueprintReadOnly, Category = "Plane")
    AActor* MovingActor = nullptr;

    UPROPERTY()
    USnapPointComponent* MovingSnapPoint = nullptr;

    UPROPERTY()
    USnapPointComponent* BoardSnapPoint = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane")
    FComponentReference CornerA;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane")
    FComponentReference CornerB;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane")
    FComponentReference CornerC;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane")
    FComponentReference CornerD;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane")
    float DetachExtraDistance = 50.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Plane")
    bool bIsPlaneActive = false;

    UFUNCTION(BlueprintCallable, Category = "Plane")
    void SetMovingActorWithSnapPoints(
        AActor* NewMovingActor,
        USnapPointComponent* NewMovingSnapPoint,
        USnapPointComponent* NewBoardSnapPoint
    );

    UPROPERTY()
    FVector LastPlaneOrigin = FVector::ZeroVector;

    UPROPERTY()
    bool bHasLastPlaneOrigin = false;
    void ClearMovingActor();

    void ApplyPlaneConstraint();
};