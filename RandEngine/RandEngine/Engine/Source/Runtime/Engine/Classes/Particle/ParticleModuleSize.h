#pragma once
#include "ParticleModule.h"

class UParticleModuleSize : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleSize, UParticleModule);
public:
    FVector StartSize;
    FVector EndSize;
    bool bInterpolateSize;
public:
    UParticleModuleSize();
    ~UParticleModuleSize() override = default;
    friend FArchive& operator<<(FArchive& Ar, UParticleModuleSize& M);
    virtual void SpawnParticle(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle& ParticleBase) override;
    virtual void UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle, const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset, float DeltaTime) override;
    virtual void Serialize(FArchive& Ar) override;
};

