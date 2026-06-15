#include "SnapPointComponent.h"

USnapPointComponent::USnapPointComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool USnapPointComponent::IsCompatibleWith(USnapPointComponent* Other) const
{
    if (!Other) return false;

    return CompatibleSnapIDs.Contains(Other->SnapID);
}