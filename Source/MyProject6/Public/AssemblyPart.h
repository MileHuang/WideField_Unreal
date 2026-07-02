#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AssemblyPart.generated.h"

class USnapPointComponent;

UCLASS()
class MYPROJECT6_API AAssemblyPart : public AActor
{
    GENERATED_BODY()

public:
    AAssemblyPart();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    void CheckSnap();
    void CheckDetach();
    UFUNCTION(BlueprintCallable)
    void SetDragging(bool bDragging);
    UPROPERTY(BlueprintReadOnly)
    bool bIsBeingDragged = false;

    UFUNCTION(BlueprintCallable, Category = "Assembly")
    void ClearAllSnapConnections();
private:
    TArray<USnapPointComponent*> MySnapPoints;

    bool IsAngleValid(
        USnapPointComponent* MyPoint,
        USnapPointComponent* OtherPoint
    ) const;

    void ApplyNormalSnapAttachment(
        AAssemblyPart* OtherPart,
        USnapPointComponent* MyPoint,
        USnapPointComponent* OtherPoint
    );

};