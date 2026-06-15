#include "SlideConstraintComponent.h"
#include "SnapPointComponent.h"
#include "AssemblyPart.h"
USlideConstraintComponent::USlideConstraintComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void USlideConstraintComponent::BeginPlay()
{
    Super::BeginPlay();

    bIsSliding = false;
    MovingActor = nullptr;
    ActorToMove = nullptr;
    MovingSnapPoint = nullptr;
    HolderSnapPoint = nullptr;
    bHasLastSlideLocations = false;
}

void USlideConstraintComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction
)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    ApplySlideConstraint();
}

AActor* USlideConstraintComponent::GetTopAttachParent(AActor* Actor) const
{
    if (!Actor) return nullptr;

    AActor* Current = Actor;

    while (Current->GetAttachParentActor())
    {
        Current = Current->GetAttachParentActor();
    }

    return Current;
}

void USlideConstraintComponent::SetMovingActorWithSnapPoints(
    AActor* NewMovingActor,
    USnapPointComponent* NewMovingSnapPoint,
    USnapPointComponent* NewHolderSnapPoint
)
{
    MovingActor = NewMovingActor;
    MovingSnapPoint = NewMovingSnapPoint;
    HolderSnapPoint = NewHolderSnapPoint;

    ActorToMove = MovingActor;

    if (bMoveAttachRoot)
    {
        ActorToMove = GetTopAttachParent(MovingActor);
    }

    bIsSliding = MovingActor != nullptr;

    if (MovingSnapPoint)
    {
        MovingSnapPoint->bIsSlideConnection = true;
    }

    if (HolderSnapPoint)
    {
        HolderSnapPoint->bIsSlideConnection = true;
    }

    AActor* Owner = GetOwner();

    USceneComponent* StartComp =
        Owner ? Cast<USceneComponent>(SlideStart.GetComponent(Owner)) : nullptr;

    USceneComponent* EndComp =
        Owner ? Cast<USceneComponent>(SlideEnd.GetComponent(Owner)) : nullptr;

    if (StartComp && EndComp)
    {
        LastSlideStartLocation = StartComp->GetComponentLocation();
        LastSlideEndLocation = EndComp->GetComponentLocation();
        bHasLastSlideLocations = true;
    }
    if (bDisableChildSlidesWhenActive && MovingActor)
    {
        TArray<AActor*> AttachedActors;
        MovingActor->GetAttachedActors(AttachedActors);

        for (AActor* Attached : AttachedActors)
        {
            if (!Attached) continue;

            USlideConstraintComponent* Slide =
                Attached->FindComponentByClass<USlideConstraintComponent>();

            if (Slide && Slide != this)
            {
                Slide->bIsSliding = false;
            }
        }
    }
    UE_LOG(LogTemp, Warning,
        TEXT("Slide started. MovingActor=%s ActorToMove=%s Holder=%s MoveAttachRoot=%s"),
        MovingActor ? *MovingActor->GetName() : TEXT("None"),
        ActorToMove ? *ActorToMove->GetName() : TEXT("None"),
        GetOwner() ? *GetOwner()->GetName() : TEXT("None"),
        bMoveAttachRoot ? TEXT("true") : TEXT("false"));
}

void USlideConstraintComponent::ClearMovingActor()
{
    if (MovingSnapPoint)
    {
        MovingSnapPoint->bIsConnected = false;
        MovingSnapPoint->bIsSlideConnection = false;
        MovingSnapPoint->ConnectedSnapPoint = nullptr;
    }

    if (HolderSnapPoint)
    {
        HolderSnapPoint->bIsConnected = false;
        HolderSnapPoint->bIsSlideConnection = false;
        HolderSnapPoint->ConnectedSnapPoint = nullptr;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("Slide cleared: %s"),
        MovingActor ? *MovingActor->GetName() : TEXT("None"));

    MovingActor = nullptr;
    ActorToMove = nullptr;
    MovingSnapPoint = nullptr;
    HolderSnapPoint = nullptr;
    bIsSliding = false;
    bHasLastSlideLocations = false;
}

void USlideConstraintComponent::ApplySlideConstraint()
{
    if (!bIsSliding) return;
    if (!MovingActor) return;
    if (!ActorToMove) return;
    if (!MovingSnapPoint) return;

    AActor* Owner = GetOwner();
    if (!Owner) return;

    USceneComponent* StartComp =
        Cast<USceneComponent>(SlideStart.GetComponent(Owner));

    USceneComponent* EndComp =
        Cast<USceneComponent>(SlideEnd.GetComponent(Owner));

    if (!StartComp || !EndComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("SlideStart or SlideEnd is missing"));
        return;
    }

    FVector Start = StartComp->GetComponentLocation();
    FVector End = EndComp->GetComponentLocation();

    bool bRailMoved = false;

    if (bHasLastSlideLocations)
    {
        FVector RailDelta = Start - LastSlideStartLocation;

        if (!RailDelta.IsNearlyZero())
        {
            ActorToMove->AddActorWorldOffset(RailDelta);
            bRailMoved = true;
        }
    }

    if (bRailMoved)
    {
        LastSlideStartLocation = Start;
        LastSlideEndLocation = End;
        bHasLastSlideLocations = true;
        return;
    }

    FVector SlideVector = End - Start;
    float SlideLength = SlideVector.Size();

    if (SlideLength <= KINDA_SMALL_NUMBER) return;

    FVector SlideDirection = SlideVector.GetSafeNormal();

    FVector CurrentSnapLocation =
        MovingSnapPoint->GetComponentLocation();

    float DistanceAlongAxis =
        FVector::DotProduct(CurrentSnapLocation - Start, SlideDirection);

    if (DetachMode == ESlideDetachMode::CanDetach)
    {
        if (DistanceAlongAxis < -DetachExtraDistance ||
            DistanceAlongAxis > SlideLength + DetachExtraDistance)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("Slide detached by range: %s"),
                *MovingActor->GetName());

            ClearMovingActor();
            return;
        }
    }

    float ClampedDistance =
        FMath::Clamp(DistanceAlongAxis, 0.f, SlideLength);

    FVector NewSnapPointLocation =
        Start + SlideDirection * ClampedDistance;

    FVector Offset =
        NewSnapPointLocation - MovingSnapPoint->GetComponentLocation();

    if (!Offset.IsNearlyZero())
    {
        ActorToMove->AddActorWorldOffset(Offset);
    }

    LastSlideStartLocation = Start;
    LastSlideEndLocation = End;
    bHasLastSlideLocations = true;
}