#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DeleteAllConfirmWidget.generated.h"

class AAssembleLevelManager;

UCLASS()
class MYPROJECT6_API UDeleteAllConfirmWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Delete")
    AAssembleLevelManager* AssembleManager = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Delete")
    void ConfirmDeleteAll();

    UFUNCTION(BlueprintCallable, Category = "Delete")
    void CancelDeleteAll();
};
