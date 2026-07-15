#include "PlaneConstraintComponent.h"
#include "SnapPointComponent.h"
#include "AssemblyPart.h"

UPlaneConstraintComponent::UPlaneConstraintComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UPlaneConstraintComponent::BeginPlay()
{
    Super::BeginPlay();

    bIsPlaneActive = false;
    MovingActors.Empty();
}

void UPlaneConstraintComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(
        DeltaTime,
        TickType,
        ThisTickFunction
    );

    ApplyPlaneConstraint();
}

void UPlaneConstraintComponent::SetMovingActorWithSnapPoints(
    AActor* NewMovingActor,
    USnapPointComponent* NewMovingSnapPoint,
    USnapPointComponent* NewBoardSnapPoint
)
{
    if (!NewMovingActor ||
        !NewMovingSnapPoint ||
        !NewBoardSnapPoint)
    {
        return;
    }

    for (const FPlaneMovingActorData& Data : MovingActors)
    {
        if (Data.MovingActor == NewMovingActor)
        {
            return;
        }
    }

    FPlaneMovingActorData NewData;
    NewData.MovingActor = NewMovingActor;
    NewData.MovingSnapPoint = NewMovingSnapPoint;
    NewData.BoardSnapPoint = NewBoardSnapPoint;

    AActor* Owner = GetOwner();

    USceneComponent* AComp =
        Owner ? Cast<USceneComponent>(
            CornerA.GetComponent(Owner)
        ) : nullptr;

    if (AComp)
    {
        NewData.LastPlaneOrigin =
            AComp->GetComponentLocation();

        NewData.bHasLastPlaneOrigin = true;
    }

    NewMovingSnapPoint->bIsSlideConnection = true;
    NewBoardSnapPoint->bIsSlideConnection = true;

    MovingActors.Add(NewData);

    bIsPlaneActive = MovingActors.Num() > 0;

    UE_LOG(LogTemp, Warning,
        TEXT("Plane constraint added: %s Count=%d"),
        *NewMovingActor->GetName(),
        MovingActors.Num());
}

void UPlaneConstraintComponent::ClearMovingActor()
{
    for (FPlaneMovingActorData& Data : MovingActors)
    {
        if (Data.MovingSnapPoint)
        {
            Data.MovingSnapPoint->bIsConnected = false;
            Data.MovingSnapPoint->bIsSlideConnection = false;
            Data.MovingSnapPoint->ConnectedSnapPoint = nullptr;
        }

        if (Data.BoardSnapPoint)
        {
            Data.BoardSnapPoint->bIsConnected = false;
            Data.BoardSnapPoint->bIsSlideConnection = false;
            Data.BoardSnapPoint->ConnectedSnapPoint = nullptr;
        }
    }

    MovingActors.Empty();
    bIsPlaneActive = false;

    UE_LOG(LogTemp, Warning,
        TEXT("All plane constraints cleared"));
}

void UPlaneConstraintComponent::ApplyPlaneConstraint()
{
    if (!bIsPlaneActive)
    {
        return;
    }

    for (int32 i = MovingActors.Num() - 1; i >= 0; --i)
    {
        ApplyPlaneConstraintToOne(i);
    }

    bIsPlaneActive = MovingActors.Num() > 0;
}

void UPlaneConstraintComponent::ApplyPlaneConstraintToOne(int32 Index)
{
    if (!MovingActors.IsValidIndex(Index))
    {
        return;
    }

    FPlaneMovingActorData& Data = MovingActors[Index];

    if (!Data.MovingActor || !Data.MovingSnapPoint)
    {
        MovingActors.RemoveAt(Index);
        return;
    }

    AActor* Owner = GetOwner();

    if (!Owner)
    {
        return;
    }

    USceneComponent* AComp =
        Cast<USceneComponent>(
            CornerA.GetComponent(Owner)
        );

    USceneComponent* BComp =
        Cast<USceneComponent>(
            CornerB.GetComponent(Owner)
        );

    USceneComponent* DComp =
        Cast<USceneComponent>(
            CornerD.GetComponent(Owner)
        );

    if (!AComp || !BComp || !DComp)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("Plane corners missing"));
        return;
    }

    FVector A = AComp->GetComponentLocation();
    FVector B = BComp->GetComponentLocation();
    FVector D = DComp->GetComponentLocation();

    if (Data.bHasLastPlaneOrigin)
    {
        FVector PlaneDelta =
            A - Data.LastPlaneOrigin;

        if (!PlaneDelta.IsNearlyZero())
        {
            Data.MovingActor->AddActorWorldOffset(
                PlaneDelta
            );
        }
    }

    Data.LastPlaneOrigin = A;
    Data.bHasLastPlaneOrigin = true;

    AAssemblyPart* MovingPart =
        Cast<AAssemblyPart>(Data.MovingActor);

    if (!MovingPart || !MovingPart->bIsBeingDragged)
    {
        return;
    }

    FVector XVector = B - A;
    FVector YVector = D - A;

    float XLength = XVector.Size();
    float YLength = YVector.Size();

    if (XLength <= KINDA_SMALL_NUMBER ||
        YLength <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    FVector XDir = XVector.GetSafeNormal();
    FVector YDir = YVector.GetSafeNormal();

    FVector Current =
        Data.MovingSnapPoint->GetComponentLocation();

    FVector Relative = Current - A;

    float XValue =
        FVector::DotProduct(Relative, XDir);

    float YValue =
        FVector::DotProduct(Relative, YDir);

    bool bTooFar =
        XValue < -DetachExtraDistance ||
        XValue > XLength + DetachExtraDistance ||
        YValue < -DetachExtraDistance ||
        YValue > YLength + DetachExtraDistance;

    if (bTooFar)
    {
        if (Data.MovingSnapPoint)
        {
            Data.MovingSnapPoint->bIsConnected = false;
            Data.MovingSnapPoint->bIsSlideConnection = false;
            Data.MovingSnapPoint->ConnectedSnapPoint = nullptr;
        }

        if (Data.BoardSnapPoint)
        {
            Data.BoardSnapPoint->bIsConnected = false;
            Data.BoardSnapPoint->bIsSlideConnection = false;
            Data.BoardSnapPoint->ConnectedSnapPoint = nullptr;
        }

        MovingActors.RemoveAt(Index);
        return;
    }

    float ClampedX =
        FMath::Clamp(XValue, 0.f, XLength);

    float ClampedY =
        FMath::Clamp(YValue, 0.f, YLength);

    FVector NewSnapLocation =
        A + XDir * ClampedX + YDir * ClampedY;

    FVector Offset =
        NewSnapLocation -
        Data.MovingSnapPoint->GetComponentLocation();

    if (!Offset.IsNearlyZero())
    {
        Data.MovingActor->AddActorWorldOffset(Offset);
    }
}