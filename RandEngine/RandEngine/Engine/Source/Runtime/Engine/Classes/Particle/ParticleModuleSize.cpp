#include "ParticleModuleSize.h"

UParticleModuleSize::UParticleModuleSize()
{
}


void UParticleModuleSize::SpawnParticle(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime,
                                        FBaseParticle& Particle)
{
    Particle.BaseSize = StartSize;
    Particle.Size = StartSize;
}
void UParticleModuleSize::UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle, 
                                         const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset, float DeltaTime)
{
    if (bInterpolateSize)
    {
      
        Particle.Size.X = FMath::Lerp(StartSize.X, EndSize.X, Particle.RelativeTime);
        Particle.Size.Y = FMath::Lerp(StartSize.Y, EndSize.Y, Particle.RelativeTime);
        Particle.Size.Z = FMath::Lerp(StartSize.Z, EndSize.Z, Particle.RelativeTime);
    }
    else
    {
        Particle.Size = Particle.BaseSize;
    }
}
FArchive& operator<<(FArchive& Ar, UParticleModuleSize& M)
{
    M.Serialize(Ar);

    return Ar;

}

void UParticleModuleSize::Serialize(FArchive& Ar)
{
    // 1. 부모 클래스(UParticleModule)의 직렬화 호출
    Super::Serialize(Ar);

    // 2. StartSize 직렬화
    Ar << StartSize;

    // 3. EndSize 직렬화
    Ar << EndSize;

    // 4. bInterpolateSize 직렬화
    Ar << bInterpolateSize;
}
