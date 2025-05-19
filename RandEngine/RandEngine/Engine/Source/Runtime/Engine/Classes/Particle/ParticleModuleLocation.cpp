#include "ParticleModuleLocation.h"

UParticleModuleLocation::UParticleModuleLocation()
{
}

FArchive& operator<<(FArchive& Ar, UParticleModuleLocation& M)
{
    M.Serialize(Ar);
    return Ar;
}
