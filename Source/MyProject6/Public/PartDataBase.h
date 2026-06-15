#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PartDatabase.generated.h"

class UTexture2D;
class AActor;

UENUM(BlueprintType)
enum class EPartCategory : uint8
{
    All UMETA(DisplayName = "All"),
    Mount UMETA(DisplayName = "Mount"),
    Post UMETA(DisplayName = "Post"),
    Cage UMETA(DisplayName = "Cage"),
    Cube UMETA(DisplayName = "Cube"),
    Mirror UMETA(DisplayName = "Mirror")
};

USTRUCT(BlueprintType)
struct FPartInfoData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PartName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PartDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString PartURL;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UTexture2D* PartImage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AActor> PartActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EPartCategory Category = EPartCategory::All;
};

UCLASS(BlueprintType)
class MYPROJECT6_API UPartDatabase : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parts")
    TMap<FString, FPartInfoData> Parts;

    UFUNCTION(BlueprintCallable, Category = "Parts")
    bool FindPartInfo(const FString& MeshName, FPartInfoData& OutPartInfo) const;
};