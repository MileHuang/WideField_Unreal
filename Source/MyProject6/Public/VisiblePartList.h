#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VisiblePartList.generated.h"

class UUserWidget;
class UPartDatabase;
class UTexture2D;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct FVisiblePartGroup
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString PartName;

    UPROPERTY(BlueprintReadOnly)
    UTexture2D* PartImage = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TArray<AActor*> Actors;

    UPROPERTY(BlueprintReadWrite)
    bool bExpanded = false;
};

UCLASS()
class MYPROJECT6_API AVisiblePartList : public AActor
{
    GENERATED_BODY()

public:
    AVisiblePartList();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UUserWidget> VisiblePartWidgetClass;

    UPROPERTY()
    UUserWidget* VisiblePartWidget;

    UPROPERTY(BlueprintReadOnly)
    TArray<AActor*> VisibleParts;

    UPROPERTY(BlueprintReadOnly)
    TArray<FVisiblePartGroup> VisiblePartGroups;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Database")
    UPartDatabase* PartDatabase;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
    UMaterialInterface* HighlightMaterial;

    UPROPERTY(BlueprintReadOnly)
    AActor* SelectedActor = nullptr;

    UFUNCTION(BlueprintCallable)
    void RefreshVisibleParts();

    UFUNCTION(BlueprintCallable)
    void SelectVisibleActor(AActor* Actor);

    UFUNCTION(BlueprintCallable)
    FString GetDisplayNameForActor(AActor* Actor) const;

    UFUNCTION(BlueprintCallable)
    UTexture2D* GetImageForActor(AActor* Actor) const;

    bool IsActorVisibleOnScreen(AActor* Actor) const;
    UPROPERTY()
    FTimerHandle RefreshTimerHandle;

    UPROPERTY(BlueprintReadOnly, Category = "Visible Parts")
    TSet<FString> ExpandedPartNames;

    UFUNCTION(BlueprintCallable)
    void ToggleGroupExpanded(const FString& PartName);

    UFUNCTION(BlueprintCallable)
    bool IsGroupExpanded(const FString& PartName) const;

    UPROPERTY()
    TArray<AActor*> HighlightedActors;

    UFUNCTION(BlueprintCallable)
    void ClearHighlights();

    UFUNCTION(BlueprintCallable)
    void HighlightActor(AActor* Actor);

    UFUNCTION(BlueprintCallable)
    void HighlightActorGroup(const TArray<AActor*>& Actors);
};