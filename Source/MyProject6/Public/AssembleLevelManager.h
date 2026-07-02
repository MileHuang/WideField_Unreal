#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "AssemblySaveGame.h"
#include "AssembleLevelManager.generated.h"

class UPartDatabase;
class UMaterialInterface;
class UPartInfo;

UCLASS()
class MYPROJECT6_API AAssembleLevelManager : public AActor
{
    GENERATED_BODY()

public:
    AAssembleLevelManager();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Database")
    UPartDatabase* PartDatabase = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    AActor* HitActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    AActor* LastHoverActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trace")
    float TraceDistance = 2000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover")
    UMaterialInterface* HoverOverlayMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> PauseWidgetClass;

    UPROPERTY()
    UUserWidget* PauseWidget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> TipsWidgetClass;

    UPROPERTY()
    UUserWidget* TipsWidget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> SpawnWidgetClass;

    UPROPERTY()
    UUserWidget* SpawnWidget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> PartInfoWidgetClass;

    UPROPERTY()
    UUserWidget* CurrentPartInfoWidget = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser")
    TSubclassOf<AActor> LaserClass;

    UPROPERTY(BlueprintReadWrite, Category = "Laser")
    bool bLaserPressed = true;

    UFUNCTION(BlueprintCallable)
    void TraceMouse();

    UFUNCTION(BlueprintCallable)
    void DeleteSelected();

    UFUNCTION(BlueprintCallable)
    void TogglePauseMenu();

    UFUNCTION(BlueprintCallable)
    void ToggleSpawnWidget();

    UFUNCTION(BlueprintCallable)
    void ToggleLaser();

    UFUNCTION(BlueprintCallable)
    void HideAllLasers();

    UFUNCTION(BlueprintCallable)
    void ShowPartInfo();

    UFUNCTION(BlueprintCallable)
    void ClosePartInfo();

    UFUNCTION(BlueprintCallable, Category = "Save")
    void SaveAssembly();

    UFUNCTION(BlueprintCallable, Category = "Save")
    void LoadAssembly();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FString SaveSlotName = TEXT("AssemblySaveSlot");

    UFUNCTION(BlueprintCallable, Category = "Save")
    FString GetPartNameFromActor(AActor* Actor) const;
private:
    void HoverActor(AActor* NewActor);
    void UnhoverActor(AActor* OldActor);
};