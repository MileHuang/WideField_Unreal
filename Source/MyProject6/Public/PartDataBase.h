#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PartDatabase.generated.h"

class UTexture2D;

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