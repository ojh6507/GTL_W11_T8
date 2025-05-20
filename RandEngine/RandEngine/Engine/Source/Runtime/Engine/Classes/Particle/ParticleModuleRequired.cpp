#include "ParticleModuleRequired.h"



void UParticleModuleRequired::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);
    // --- UParticleModuleRequired 고유 멤버 직렬화 ---
    Ar << EmitterDuration;
    Ar << EmitterLoops;
    Ar << SubImages_Horizontal;
    Ar << SubImages_Vertical;
    Ar << bKillOnDeactivate;
    Ar << bKillOnCompleted;
    Ar << bRequiresSorting;
    Ar << SortMode;
    Ar << bIgnoreComponentScale;

}

FArchive& operator<<(FArchive& Ar, UParticleModuleRequired& M)
{
    M.Serialize(Ar);
    return Ar;
}
