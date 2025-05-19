#pragma once
#include "ParticleModule.h"
#include "Math/DistributionFloat.h"
class UParticleModuleLifetime : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleLifetime, UParticleModule);

public:
    UParticleModuleLifetime();
    ~UParticleModuleLifetime() override = default;


    FDistributionFloat Lifetime;
    float GetMaxLifetime() const
    {
        if (Lifetime.IsConstant())
            return Lifetime.GetConstantValue();
        else
            return Lifetime.GetMaxValue();
    }
    virtual void SpawnParticle(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle& ParticleBase) override;
    virtual void UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle, const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset, float DeltaTime) override;
    
    
    virtual EModuleType GetModuleType() const override { return EModuleType::Lifetime; }

    friend FArchive& operator<<(FArchive& Ar, UParticleModuleLifetime& M);
    virtual void Serialize(FArchive& Ar)
    {
        Super::Serialize(Ar);
        Ar << Lifetime;
    }
};

