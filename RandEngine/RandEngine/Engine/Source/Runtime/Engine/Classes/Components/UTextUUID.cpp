#include "UTextUUID.h"

UTextUUID::UTextUUID()
{
    SetRelativeScale3D(FVector(0.1f, 0.25f, 0.25f));
    SetRelativeLocation(FVector(0.0f, 0.0f, -0.5f));
}

int UTextUUID::CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const
{
    return 0;
}

void UTextUUID::SetUUID(uint32 UUID)
{
    SetText(std::to_wstring(UUID));
}


