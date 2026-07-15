#include "PartDatabase.h"

bool UPartDatabase::FindPartInfo(
    const FString& MeshName,
    FPartInfoData& OutPartInfo
) const
{
    FString BestKey;
    bool bFound = false;

    for (const TPair<FString, FPartInfoData>& Pair : Parts)
    {
        const FString& Key = Pair.Key;

        if (MeshName.Contains(Key))
        {
            if (!bFound || Key.Len() > BestKey.Len())
            {
                BestKey = Key;
                OutPartInfo = Pair.Value;
                bFound = true;
            }
        }
    }

    return bFound;
}
bool UPartDatabase::FindPartByName(
    const FString& PartName,
    FPartInfoData& OutPartInfo
) const
{
    for (const TPair<FString, FPartInfoData>& Pair : Parts)
    {
        if (Pair.Value.PartName == PartName || Pair.Key == PartName)
        {
            OutPartInfo = Pair.Value;
            return true;
        }
    }

    return false;
}