#include "AssemblyPart.h"
#include "SnapPointComponent.h"
#include "SlideConstraintComponent.h"
#include "PlaneConstraintComponent.h"
#include "Kismet/GameplayStatics.h"

AAssemblyPart::AAssemblyPart()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAssemblyPart::BeginPlay()
{
    Super::BeginPlay();

    GetComponents<USnapPointComponent>(MySnapPoints);
}

void AAssemblyPart::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    CheckDetach();
    CheckSnap();
}

bool AAssemblyPart::IsAngleValid(
    USnapPointComponent* MyPoint,
    USnapPointComponent* OtherPoint
) const
{
    if (!MyPoint || !OtherPoint)
    {
        return false;
    }

    FVector MyForward = MyPoint->GetForwardVector();
    FVector OtherForward = OtherPoint->GetForwardVector();

    float Dot = FVector::DotProduct(MyForward, OtherForward);
    Dot = FMath::Clamp(Dot, -1.f, 1.f);

    float Angle = FMath::RadiansToDegrees(FMath::Acos(Dot));

    return Angle <= MyPoint->AngleTolerance;
}

void AAssemblyPart::ApplyNormalSnapAttachment(
    AAssemblyPart* OtherPart,
    USnapPointComponent* MyPoint,
    USnapPointComponent* OtherPoint
)
{
    if (!OtherPart || !MyPoint || !OtherPoint) return;

    if (MyPoint->bIsSlideConnection || OtherPoint->bIsSlideConnection)
    {
        return;
    }

    AActor* ParentActor = nullptr;
    AActor* ChildActor = nullptr;

    if (MyPoint->SnapRole == ESnapRole::Parent &&
        OtherPoint->SnapRole == ESnapRole::Child)
    {
        ParentActor = this;
        ChildActor = OtherPart;
    }
    else if (MyPoint->SnapRole == ESnapRole::Child &&
        OtherPoint->SnapRole == ESnapRole::Parent)
    {
        ParentActor = OtherPart;
        ChildActor = this;
    }
    else
    {
        return;
    }

    ChildActor->AttachToActor(
        ParentActor,
        FAttachmentTransformRules::KeepWorldTransform
    );

    UE_LOG(LogTemp, Warning,
        TEXT("Attached Child %s to Parent %s"),
        *ChildActor->GetName(),
        *ParentActor->GetName());
}

void AAssemblyPart::CheckSnap()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<AActor*> AllParts;
    UGameplayStatics::GetAllActorsOfClass(
        World,
        AAssemblyPart::StaticClass(),
        AllParts
    );

    for (USnapPointComponent* MyPoint : MySnapPoints)
    {
        if (!MyPoint || MyPoint->bIsConnected) continue;

        for (AActor* OtherActor : AllParts)
        {
            if (!OtherActor || OtherActor == this) continue;

            AAssemblyPart* OtherPart = Cast<AAssemblyPart>(OtherActor);
            if (!OtherPart) continue;

            TArray<USnapPointComponent*> OtherSnapPoints;
            OtherPart->GetComponents<USnapPointComponent>(OtherSnapPoints);

            for (USnapPointComponent* OtherPoint : OtherSnapPoints)
            {
                if (!OtherPoint || OtherPoint->bIsConnected) continue;

                if (!MyPoint->IsCompatibleWith(OtherPoint)) continue;

                float Distance = FVector::Dist(
                    MyPoint->GetComponentLocation(),
                    OtherPoint->GetComponentLocation()
                );

                if (Distance > MyPoint->SnapDistance) continue;
                if (!IsAngleValid(MyPoint, OtherPoint)) continue;

                FVector Offset =
                    OtherPoint->GetComponentLocation() -
                    MyPoint->GetComponentLocation();

                AddActorWorldOffset(Offset);

                MyPoint->bIsConnected = true;
                OtherPoint->bIsConnected = true;

                MyPoint->ConnectedSnapPoint = OtherPoint;
                OtherPoint->ConnectedSnapPoint = MyPoint;

                USlideConstraintComponent* OtherSlideComp =
                    OtherPart->FindComponentByClass<USlideConstraintComponent>();

                USlideConstraintComponent* MySlideComp =
                    FindComponentByClass<USlideConstraintComponent>();

                UPlaneConstraintComponent* OtherPlaneComp =
                    OtherPart->FindComponentByClass<UPlaneConstraintComponent>();

                UPlaneConstraintComponent* MyPlaneComp =
                    FindComponentByClass<UPlaneConstraintComponent>();

                if (OtherPlaneComp && OtherPoint->bUseSlideConstraint)
                {
                    MyPoint->bIsSlideConnection = true;
                    OtherPoint->bIsSlideConnection = true;

                    OtherPlaneComp->SetMovingActorWithSnapPoints(
                        this,
                        MyPoint,
                        OtherPoint
                    );
                }
                else if (MyPlaneComp && MyPoint->bUseSlideConstraint)
                {
                    MyPoint->bIsSlideConnection = true;
                    OtherPoint->bIsSlideConnection = true;

                    MyPlaneComp->SetMovingActorWithSnapPoints(
                        OtherPart,
                        OtherPoint,
                        MyPoint
                    );
                }
                else if (OtherSlideComp && OtherPoint->bUseSlideConstraint)
                {
                    MyPoint->bIsSlideConnection = true;
                    OtherPoint->bIsSlideConnection = true;

                    OtherSlideComp->SetMovingActorWithSnapPoints(
                        this,
                        MyPoint,
                        OtherPoint
                    );
                }
                else if (MySlideComp && MyPoint->bUseSlideConstraint)
                {
                    MyPoint->bIsSlideConnection = true;
                    OtherPoint->bIsSlideConnection = true;

                    MySlideComp->SetMovingActorWithSnapPoints(
                        OtherPart,
                        OtherPoint,
                        MyPoint
                    );
                }
                else
                {
                    ApplyNormalSnapAttachment(
                        OtherPart,
                        MyPoint,
                        OtherPoint
                    );
                }

                UE_LOG(LogTemp, Warning,
                    TEXT("Snapped: %s [%s] to %s [%s]"),
                    *GetName(),
                    *MyPoint->GetName(),
                    *OtherPart->GetName(),
                    *OtherPoint->GetName());

                return;
            }
        }
    }
}

void AAssemblyPart::CheckDetach()
{
    for (USnapPointComponent* MyPoint : MySnapPoints)
    {
        if (!MyPoint || !MyPoint->bIsConnected || !MyPoint->ConnectedSnapPoint)
        {
            continue;
        }

        if (MyPoint->bIsSlideConnection)
        {
            continue;
        }

        USnapPointComponent* OtherPoint = MyPoint->ConnectedSnapPoint;

        float Distance = FVector::Dist(
            MyPoint->GetComponentLocation(),
            OtherPoint->GetComponentLocation()
        );

        bool bAngleValid = IsAngleValid(MyPoint, OtherPoint);

        if (Distance > MyPoint->DetachDistance || !bAngleValid)
        {
            DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

            OtherPoint->bIsConnected = false;
            OtherPoint->bIsSlideConnection = false;
            OtherPoint->ConnectedSnapPoint = nullptr;

            MyPoint->bIsConnected = false;
            MyPoint->bIsSlideConnection = false;
            MyPoint->ConnectedSnapPoint = nullptr;

            UE_LOG(LogTemp, Warning,
                TEXT("Detached SnapPoint: %s"),
                *MyPoint->GetName());

            continue;
        }

        FVector Offset =
            OtherPoint->GetComponentLocation() -
            MyPoint->GetComponentLocation();

        AddActorWorldOffset(Offset);
    }
}

void AAssemblyPart::SetDragging(bool bDragging)
{
    bIsBeingDragged = bDragging;
}