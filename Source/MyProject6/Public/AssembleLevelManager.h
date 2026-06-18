#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Blueprint/UserWidget.h"
#include "AssembleLevelManager.generated.h"
class UPartDatabase;
class AAssemblyPart;
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
    UPartDatabase* PartDatabase;

    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    AActor* HitActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Selection")
    AActor* LastHoverActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Drag")
    AActor* MoveActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Drag")
    bool bIsDragging = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover")
    UMaterialInterface* HoverOverlayMaterial;

    UFUNCTION(BlueprintCallable)
    void TraceMouse();

    UFUNCTION(BlueprintCallable)
    void BeginDrag();

    UFUNCTION(BlueprintCallable)
    void EndDrag();

    UFUNCTION(BlueprintCallable)
    void DeleteSelected();

    UPROPERTY(BlueprintReadOnly, Category = "Drag")
    float DragPlaneZ = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Drag")
    FVector DragOffset = FVector::ZeroVector;

    bool GetMousePointOnDragPlane(FVector& OutPoint) const;
    UFUNCTION(BlueprintCallable)
    void UpdateDrag();

    UPROPERTY(BlueprintReadOnly, Category = "Drag")
    float DragStartMouseY = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Drag")
    float DragStartActorZ = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drag")
    float VerticalDragSpeed = 2.f;

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
    void TogglePauseMenu();

    UFUNCTION(BlueprintCallable)
    void ToggleSpawnWidget();

    UFUNCTION(BlueprintCallable)
    void ToggleLaser();

    UFUNCTION(BlueprintCallable)
    void HideAllLasers();

    UFUNCTION(BlueprintCallable)
    void ShowPartInfo();

    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = "true"))
    AActor* SetObject = nullptr;
    UFUNCTION(BlueprintCallable)
    void ClosePartInfo();

private:
    void HoverActor(AActor* NewActor);
    void UnhoverActor(AActor* OldActor);
};