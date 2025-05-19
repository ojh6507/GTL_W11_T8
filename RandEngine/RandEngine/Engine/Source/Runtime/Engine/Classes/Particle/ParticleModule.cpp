#include "ParticleModule.h"

FArchive& operator<<(FArchive& Ar, UParticleModule& M)
{
    M.Serialize(Ar);
    return Ar;
}
