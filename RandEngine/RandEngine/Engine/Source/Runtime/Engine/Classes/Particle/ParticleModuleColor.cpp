#include "ParticleModuleColor.h"
#include "Serialization/Archive.h"
#include "Math/Color.h"
UParticleModuleColor::UParticleModuleColor()
{
}

void UParticleModuleColor::UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle, 
                                          const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset, float DeltaTime)
{
    if (bInterpolateColor)
    {
        Particle.Color.R = FMath::Lerp(StartColor.R, EndColor.R, Particle.RelativeTime);
        Particle.Color.G = FMath::Lerp(StartColor.G, EndColor.G, Particle.RelativeTime);
        Particle.Color.B = FMath::Lerp(StartColor.B, EndColor.B, Particle.RelativeTime);
        Particle.Color.A = FMath::Lerp(StartColor.A, EndColor.A, Particle.RelativeTime);
    }
    else
    {
        Particle.Color = StartColor;
    }
}
void UParticleModuleColor::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);

    Ar << StartColor.R; Ar << StartColor.G; Ar << StartColor.B; Ar << StartColor.A;

    Ar << EndColor.R; Ar << EndColor.G; Ar << EndColor.B; Ar << EndColor.A;

    Ar << bInterpolateColor;
}
FArchive& operator<<(FArchive& Ar, UParticleModuleColor& M)
{
    M.Serialize(Ar);

    return Ar;
}
