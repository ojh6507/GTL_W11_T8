#include "ParticleModuleRequired.h"


FArchive& operator<<(FArchive& Ar, UParticleModuleRequired& M)
{
    M.Serialize(Ar);
    return Ar;
}
