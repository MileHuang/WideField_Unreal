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
    MovingActor = nullptr;
    MovingSnapPoint = nullptr;
    BoardSnapPoint = nullptr;
}

void UPlaneConstraintComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ApplyPlaneConstraint();
}

void UPlaneConstraintComponent::SetMovingActorWithSnapPoints(
    AActor* NewMovingActor,
    USnapPointComponent* NewMovingSnapPoint,
    USnapPointComponent* NewBoardSnapPoint
)
{
    MovingActor = NewMovingActor;
    MovingSnapPoint = NewMovingSnapPoint;
    BoardSnapPoint = NewBoardSnapPoint;

    bIsPlaneActive = MovingActor != nullptr;

    if (MovingSnapPoint)
    {
        MovingSnapPoint->bIsSlideConnection = true;
    }

    if (BoardSnapPoint)
    {
        BoardSnapPoint->bIsSlideConnection = true;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Plane constraint started: %s"),
        MovingActor ? *MovingActor->GetName() : TEXT("None"));
}

void UPlaneConstraintComponent::ClearMovingActor()
{
    if (MovingSnapPoint)
    {
        MovingSnapPoint->bIsConnected = false;
        MovingSnapPoint->bIsSlideConnection = false;
        MovingSnapPoint->ConnectedSnapPoint = nullptr;
    }

    if (BoardSnapPoint)
    {
        BoardSnapPoint->bIsConnected = false;
        BoardSnapPoint->bIsSlideConnection = false;
        BoardSnapPoint->ConnectedSnapPoint = nullptr;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Plane constraint cleared: %s"),
        MovingActor ? *MovingActor->GetName() : TEXT("None"));

    MovingActor = nullptr;
    MovingSnapPoint = nullptr;
    BoardSnapPoint = nullptr;
    bIsPlaneActive = false;
}

void UPlaneConstraintComponent::ApplyPlaneConstraint()
{
    if (!bIsPlaneActive) return;
    if (!MovingActor) return;
    if (!MovingSnapPoint) return;
    AAssemblyPart* MovingPart = Cast<AAssemblyPart>(MovingActor);

    if (!MovingPart || !MovingPart->bIsBeingDragged)
    {
        return;
    }
    AActor* Owner = GetOwner();
    if (!Owner) return;

    USceneComponent* AComp =
        Cast<USceneComponent>(CornerA.GetComponent(Owner));

    USceneComponent* BComp =
        Cast<USceneComponent>(CornerB.GetComponent(Owner));

    USceneComponent* DComp =
        Cast<USceneComponent>(CornerD.GetComponent(Owner));

    if (!AComp || !BComp || !DComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("Plane corners missing"));
        return;
    }

    FVector A = AComp->GetComponentLocation();
    FVector B = BComp->GetComponentLocation();
    FVector D = DComp->GetComponentLocation();

    FVector XVector = B - A;
    FVector YVector = D - A;

    float XLength = XVector.Size();
    float YLength = YVector.Size();

    if (XLength <= KINDA_SMALL_NUMBER || YLength <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    FVector XDir = XVector.GetSafeNormal();
    FVector YDir = YVector.GetSafeNormal();

    FVector Current =
        MovingSnapPoint->GetComponentLocation();

    FVector Relative = Current - A;

    float XValue = FVector::DotProduct(Relative, XDir);
    float YValue = FVector::DotProduct(Relative, YDir);

    bool bTooFar =
        XValue < -DetachExtraDistance ||
        XValue > XLength + DetachExtraDistance ||
        YValue < -DetachExtraDistance ||
        YValue > YLength + DetachExtraDistance;

    if (bTooFar)
    {
        ClearMovingActor();
        return;
    }

    float ClampedX =
        FMath::Clamp(XValue, 0.f, XLength);

    float ClampedY =
        FMath::Clamp(YValue, 0.f, YLength);

    FVector NewSnapLocation =
        A + XDir * ClampedX + YDir * ClampedY;

    FVector Offset =
        NewSnapLocation - MovingSnapPoint->GetComponentLocation();

    if (!Offset.IsNearlyZero())
    {
        MovingActor->AddActorWorldOffset(Offset);
    }
}