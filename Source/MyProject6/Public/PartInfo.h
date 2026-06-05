#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PartInfo.generated.h"

class UTexture2D;

UCLASS()
class MYPROJECT6_API UPartInfo : public UUserWidget
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Part Info")
    FString PartName;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Part Info")
    FString PartDescription;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Part Info")
    FString PartURL;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Part Info")
    UTexture2D* PartImage;

    UFUNCTION(BlueprintCallable, Category = "Part Info")
    void SetPartInfo(
        const FString& NewName,
        const FString& NewDescription,
        const FString& NewURL,
        UTexture2D* NewImage
    );
};