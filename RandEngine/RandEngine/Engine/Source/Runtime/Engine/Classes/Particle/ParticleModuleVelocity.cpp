#include "ParticleModuleVelocity.h"

UParticleModuleVelocity::UParticleModuleVelocity()
    : MinStartVelocity(0.0f, 0.0f, 0.0f)
    , MaxStartVelocity(0.0f, 0.0f, 100.0f)
{
}

FArchive& operator<<(FArchive& Ar, UParticleModuleVelocity& M)
{
    M.Serialize(Ar);
    return Ar;
}
