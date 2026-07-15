#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AssemblySaveGame.generated.h"

USTRUCT(BlueprintType)
struct FAssemblyPartSaveData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString SaveID;

    UPROPERTY(BlueprintReadWrite)
    FString PartName;

    UPROPERTY(BlueprintReadWrite)
    FTransform Transform;
};

USTRUCT(BlueprintType)
struct FAssemblySnapConnectionSaveData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString PartAID;

    UPROPERTY(BlueprintReadWrite)
    FName SnapAName;

    UPROPERTY(BlueprintReadWrite)
    FString PartBID;

    UPROPERTY(BlueprintReadWrite)
    FName SnapBName;

    UPROPERTY(BlueprintReadWrite)
    bool bIsSlideConnection = false;
};

UCLASS()
class MYPROJECT6_API UAssemblySaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
    TArray<FAssemblyPartSaveData> SavedParts;

    UPROPERTY(BlueprintReadWrite)
    TArray<FAssemblySnapConnectionSaveData> SavedConnections;
};