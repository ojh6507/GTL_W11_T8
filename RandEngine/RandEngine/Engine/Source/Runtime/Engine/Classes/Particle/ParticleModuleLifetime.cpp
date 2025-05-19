#include "ParticleModuleLifetime.h"

UParticleModuleLifetime::UParticleModuleLifetime()
{
}

FArchive& operator<<(FArchive& Ar, UParticleModuleLifetime& M)
{
    M.Serialize(Ar);
    return Ar;
}
