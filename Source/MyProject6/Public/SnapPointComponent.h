#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SnapPointComponent.generated.h"

UENUM(BlueprintType)
enum class ESnapRole : uint8
{
    Auto UMETA(DisplayName = "Auto"),
    Parent UMETA(DisplayName = "Parent"),
    Child UMETA(DisplayName = "Child")
};

UCLASS(ClassGroup = (Assembly), meta = (BlueprintSpawnableComponent))
class MYPROJECT6_API USnapPointComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    USnapPointComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
    FName SnapID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
    TArray<FName> CompatibleSnapIDs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
    float SnapDistance = 30.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
    float DetachDistance = 80.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
    float AngleTolerance = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
    bool bUseSlideConstraint = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
    ESnapRole SnapRole = ESnapRole::Auto;

    UPROPERTY(BlueprintReadOnly, Category = "Snap")
    bool bIsConnected = false;

    UPROPERTY(BlueprintReadOnly, Category = "Snap")
    bool bIsSlideConnection = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Snap")
    bool bAllowMultipleConnections = false;
    UPROPERTY()
    USnapPointComponent* ConnectedSnapPoint = nullptr;

    bool IsCompatibleWith(USnapPointComponent* Other) const;
};