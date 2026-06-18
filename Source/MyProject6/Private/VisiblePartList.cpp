#include "VisiblePartList.h"
#include "PartDatabase.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

AVisiblePartList::AVisiblePartList()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AVisiblePartList::BeginPlay()
{
    Super::BeginPlay();

    if (VisiblePartWidgetClass)
    {
        VisiblePartWidget = CreateWidget<UUserWidget>(
            GetWorld(),
            VisiblePartWidgetClass
        );

        if (VisiblePartWidget)
        {
            VisiblePartWidget->AddToViewport();
        }
    }

    GetWorldTimerManager().SetTimer(
        RefreshTimerHandle,
        this,
        &AVisiblePartList::RefreshVisibleParts,
        0.5f,
        true
    );
}

void AVisiblePartList::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

bool AVisiblePartList::IsActorVisibleOnScreen(AActor* Actor) const
{
    if (!Actor) return false;

    APlayerController* PC =
        UGameplayStatics::GetPlayerController(GetWorld(), 0);

    if (!PC) return false;

    UStaticMeshComponent* MeshComp =
        Actor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp) return false;

    FVector WorldPoint = MeshComp->Bounds.Origin;

    FVector2D ScreenPos;

    bool bProjected =
        PC->ProjectWorldLocationToScreen(
            WorldPoint,
            ScreenPos
        );

    if (!bProjected) return false;

    int32 SizeX = 0;
    int32 SizeY = 0;

    PC->GetViewportSize(SizeX, SizeY);

    return
        ScreenPos.X >= 0 &&
        ScreenPos.X <= SizeX &&
        ScreenPos.Y >= 0 &&
        ScreenPos.Y <= SizeY;
}

void AVisiblePartList::RefreshVisibleParts()
{
    VisibleParts.Empty();
    VisiblePartGroups.Empty();

    TArray<AActor*> AllActors;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AActor::StaticClass(),
        AllActors
    );

    TMap<FString, int32> GroupIndexMap;

    for (AActor* Actor : AllActors)
    {
        if (!Actor || Actor == this) continue;

        UStaticMeshComponent* MeshComp =
            Actor->FindComponentByClass<UStaticMeshComponent>();

        if (!MeshComp || !MeshComp->GetStaticMesh()) continue;

        if (!IsActorVisibleOnScreen(Actor)) continue;

        FString DisplayName = GetDisplayNameForActor(Actor);
        UTexture2D* Image = GetImageForActor(Actor);

        VisibleParts.Add(Actor);

        if (!GroupIndexMap.Contains(DisplayName))
        {
            FVisiblePartGroup NewGroup;
            NewGroup.PartName = DisplayName;
            NewGroup.PartImage = Image;
            NewGroup.Actors.Add(Actor);
            NewGroup.bExpanded = IsGroupExpanded(DisplayName);

            int32 NewIndex = VisiblePartGroups.Add(NewGroup);
            GroupIndexMap.Add(DisplayName, NewIndex);
        }
        else
        {
            int32 Index = GroupIndexMap[DisplayName];
            VisiblePartGroups[Index].Actors.Add(Actor);
        }
    }
}

FString AVisiblePartList::GetDisplayNameForActor(AActor* Actor) const
{
    if (!Actor) return TEXT("");

    UStaticMeshComponent* MeshComp =
        Actor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp || !MeshComp->GetStaticMesh())
    {
        return Actor->GetName();
    }

    FString MeshName =
        MeshComp->GetStaticMesh()->GetName();

    if (PartDatabase)
    {
        FPartInfoData PartData;

        if (PartDatabase->FindPartInfo(MeshName, PartData))
        {
            return PartData.PartName;
        }
    }

    return MeshName;
}

UTexture2D* AVisiblePartList::GetImageForActor(AActor* Actor) const
{
    if (!Actor) return nullptr;

    UStaticMeshComponent* MeshComp =
        Actor->FindComponentByClass<UStaticMeshComponent>();

    if (!MeshComp || !MeshComp->GetStaticMesh()) return nullptr;

    FString MeshName =
        MeshComp->GetStaticMesh()->GetName();

    if (PartDatabase)
    {
        FPartInfoData PartData;

        if (PartDatabase->FindPartInfo(MeshName, PartData))
        {
            return PartData.PartImage;
        }
    }

    return nullptr;
}

void AVisiblePartList::SelectVisibleActor(AActor* Actor)
{
    HighlightActor(Actor);
}
void AVisiblePartList::ToggleGroupExpanded(const FString& PartName)
{
    if (ExpandedPartNames.Contains(PartName))
    {
        ExpandedPartNames.Remove(PartName);
    }
    else
    {
        ExpandedPartNames.Add(PartName);
    }
}

bool AVisiblePartList::IsGroupExpanded(const FString& PartName) const
{
    return ExpandedPartNames.Contains(PartName);
}
void AVisiblePartList::ClearHighlights()
{
    for (AActor* Actor : HighlightedActors)
    {
        if (!Actor) continue;

        UStaticMeshComponent* Mesh =
            Actor->FindComponentByClass<UStaticMeshComponent>();

        if (Mesh)
        {
            Mesh->SetOverlayMaterial(nullptr);
            Mesh->bDisallowNanite = false;
            Mesh->MarkRenderStateDirty();
        }
    }

    HighlightedActors.Empty();
    SelectedActor = nullptr;
}
void AVisiblePartList::HighlightActor(AActor* Actor)
{
    if (!Actor) return;

    ClearHighlights();

    UStaticMeshComponent* Mesh =
        Actor->FindComponentByClass<UStaticMeshComponent>();

    if (Mesh && HighlightMaterial)
    {
        Mesh->bDisallowNanite = true;
        Mesh->MarkRenderStateDirty();
        Mesh->SetOverlayMaterial(HighlightMaterial);

        HighlightedActors.Add(Actor);
        SelectedActor = Actor;
    }
}
void AVisiblePartList::HighlightActorGroup(const TArray<AActor*>& Actors)
{
    ClearHighlights();

    for (AActor* Actor : Actors)
    {
        if (!Actor) continue;

        UStaticMeshComponent* Mesh =
            Actor->FindComponentByClass<UStaticMeshComponent>();

        if (Mesh && HighlightMaterial)
        {
            Mesh->bDisallowNanite = true;
            Mesh->MarkRenderStateDirty();
            Mesh->SetOverlayMaterial(HighlightMaterial);

            HighlightedActors.Add(Actor);
        }
    }

    SelectedActor = nullptr;
}