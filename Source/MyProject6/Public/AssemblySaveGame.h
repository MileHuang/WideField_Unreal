#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AssemblySaveGame.generated.h"

USTRUCT(BlueprintType)
struct FAssemblyPartSaveData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString PartName;

    UPROPERTY(BlueprintReadWrite)
    FTransform Transform;
};

UCLASS()
class MYPROJECT6_API UAssemblySaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite)
    TArray<FAssemblyPartSaveData> SavedParts;
};