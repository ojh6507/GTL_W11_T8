#include "ParticleModuleSize.h"

UParticleModuleSize::UParticleModuleSize()
{
}

FArchive& operator<<(FArchive& Ar, UParticleModuleSize& M)
{
    M.Serialize(Ar);

    return Ar;
}
