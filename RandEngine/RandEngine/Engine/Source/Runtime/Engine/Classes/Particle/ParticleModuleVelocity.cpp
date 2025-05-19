#include "ParticleModuleVelocity.h"

UParticleModuleVelocity::UParticleModuleVelocity()
    : MinStartVelocity(0.0f, 0.0f, 0.0f)
    , MaxStartVelocity(0.0f, 0.0f, 100.0f)
{
}
void UParticleModuleVelocity::SpawnParticle(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle& Particle)
{
    FVector StartVelocity;
    StartVelocity.X = FMath::FRandRange(MinStartVelocity.X, MaxStartVelocity.X);
    StartVelocity.Y = FMath::FRandRange(MinStartVelocity.Y, MaxStartVelocity.Y);
    StartVelocity.Z = FMath::FRandRange(MinStartVelocity.Z, MaxStartVelocity.Z);

    Particle.Velocity = StartVelocity;
    Particle.BaseVelocity = StartVelocity;
}
void UParticleModuleVelocity::UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle, 
                                             const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset, float DeltaTime)
{

}


void UParticleModuleVelocity::Serialize(FArchive& Ar)
{
    Ar << MinStartVelocity;

    Ar << MaxStartVelocity;

}
FArchive& operator<<(FArchive& Ar, UParticleModuleVelocity& M)
{
    M.Serialize(Ar);
    return Ar;
}
