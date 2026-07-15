#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/EngineTypes.h"
#include "PlaneConstraintComponent.generated.h"

class USnapPointComponent;

USTRUCT(BlueprintType)
struct FPlaneMovingActorData
{
    GENERATED_BODY()

    UPROPERTY()
    AActor* MovingActor = nullptr;

    UPROPERTY()
    USnapPointComponent* MovingSnapPoint = nullptr;

    UPROPERTY()
    USnapPointComponent* BoardSnapPoint = nullptr;

    UPROPERTY()
    FVector LastPlaneOrigin = FVector::ZeroVector;

    UPROPERTY()
    bool bHasLastPlaneOrigin = false;
};

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

    UPROPERTY(BlueprintReadOnly, Category = "Plane")
    bool bIsPlaneActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Plane")
    TArray<FPlaneMovingActorData> MovingActors;

    UFUNCTION(BlueprintCallable, Category = "Plane")
    void SetMovingActorWithSnapPoints(
        AActor* NewMovingActor,
        USnapPointComponent* NewMovingSnapPoint,
        USnapPointComponent* NewBoardSnapPoint
    );

    UFUNCTION(BlueprintCallable, Category = "Plane")
    void ClearMovingActor();

    void ApplyPlaneConstraint();

private:
    void ApplyPlaneConstraintToOne(int32 Index);
};