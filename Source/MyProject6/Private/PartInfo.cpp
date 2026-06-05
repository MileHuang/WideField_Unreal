#include "PartInfo.h"

void UPartInfo::SetPartInfo(
    const FString& NewName,
    const FString& NewDescription,
    const FString& NewURL,
    UTexture2D* NewImage
)
{
    PartName = NewName;
    PartDescription = NewDescription;
    PartURL = NewURL;
    PartImage = NewImage;
}