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
FArchive& operator<<(FArchive& Ar, UParticleModuleColor& M)
{
    // 1. 부모 클래스(UParticleModule)의 직렬화 호출
    Ar << static_cast<UParticleModule&>(M); // UParticleModule의 bEnabled 등을 직렬화

    // 2. StartColor 직렬화
    Ar << M.StartColor.R; Ar << M.StartColor.G; Ar << M.StartColor.B; Ar << M.StartColor.A;

    // 3. EndColor 직렬화
    Ar << M.EndColor.R; Ar << M.EndColor.G; Ar << M.EndColor.B; Ar << M.EndColor.A;
    // 4. bInterpolateColor 직렬화
    Ar << M.bInterpolateColor;

    return Ar;
}
