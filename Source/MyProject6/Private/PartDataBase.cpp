#include "PartDatabase.h"

bool UPartDatabase::FindPartInfo(
    const FString& MeshName,
    FPartInfoData& OutPartInfo
) const
{
    for (const TPair<FString, FPartInfoData>& Pair : Parts)
    {
        if (MeshName.Contains(Pair.Key))
        {
            OutPartInfo = Pair.Value;
            return true;
        }
    }

    return false;
}