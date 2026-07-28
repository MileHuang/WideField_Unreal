#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "AssemblySaveGame.h"
#include "AssembleLevelManager.generated.h"

class UObject;
class UPartDatabase;
class AAssemblyPart;
class USnapPointComponent;
class UMaterialInterface;
class UDeleteAllConfirmWidget;
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

    // Preserves the original meaning:
    // true = the next L press turns the laser system on.
    // false = the next L press turns the laser system off.
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UDeleteAllConfirmWidget> DeleteAllConfirmWidgetClass;

    UPROPERTY()
    UDeleteAllConfirmWidget* DeleteAllConfirmWidget = nullptr;

    UFUNCTION(BlueprintCallable, Category = "Delete")
    void ShowDeleteAllConfirm();

    UFUNCTION(BlueprintCallable, Category = "Delete")
    void DeleteAllParts();

    UFUNCTION(BlueprintCallable, Category = "Save")
    void RestoreSnapConnection(
        AAssemblyPart* PartA,
        USnapPointComponent* SnapA,
        AAssemblyPart* PartB,
        USnapPointComponent* SnapB,
        bool bIsSlideConnection
    );

    UFUNCTION(BlueprintCallable)
    void CloseDeleteAllConfirm();

    UPROPERTY(BlueprintReadOnly, Category = "Spawn Drag")
    AAssemblyPart* SpawnPreviewActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Spawn Drag")
    bool bIsDraggingSpawnPart = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Drag")
    float SpawnTraceDistance = 10000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Drag")
    float SpawnFallbackDistance = 1000.f;

    UFUNCTION(BlueprintCallable, Category = "Spawn Drag")
    void BeginSpawnDrag(TSubclassOf<AAssemblyPart> PartClass);

    UFUNCTION(BlueprintCallable, Category = "Spawn Drag")
    void UpdateSpawnDrag();

    UFUNCTION(BlueprintCallable, Category = "Spawn Drag")
    void ConfirmSpawnDrag();

    UFUNCTION(BlueprintCallable, Category = "Spawn Drag")
    void CancelSpawnDrag();

    UFUNCTION(BlueprintCallable, Category = "Spawn Drag")
    void UpdateSpawnDragFromScreenPosition(FVector2D ScreenPosition);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn Drag")
    float SpawnPlaneDistance = 500.f;

private:
    bool GetSpawnLocationUnderMouse(FVector& OutLocation) const;

    void HoverActor(AActor* NewActor);
    void UnhoverActor(AActor* OldActor);

    // Laser visibility and cone control.
    void SetLaserSystemEnabled(bool bEnabled);
    void HideAllFocusCones();

    // Blueprint reflection helpers. These let the C++ manager work with
    // Blueprint-only A_Laser, BP_FocusLen and SC_Emitter logic.
    void CallNoParamFunction(UObject* Target, FName FunctionName) const;
    bool GetBoolPropertyByName(
        const UObject* Object,
        FName PropertyName,
        bool& OutValue
    ) const;

    // Deletion helpers. They remove the selected part and any laser actor
    // referenced, owned or attached to it before destroying the part.
    bool IsLaserActor(const AActor* Actor) const;
    void CollectReferencedLaserActors(
        UObject* Object,
        TSet<AActor*>& OutLasers
    ) const;
    void DeactivateLaserOnObject(UObject* Object);
    void ShutdownOrDestroyLaser(AActor* LaserActor);
    void DestroyAssociatedLasers(AActor* OwnerActor);
    void DestroyPartAndAssociatedLasers(AActor* Actor);
};