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
    virtual EModuleType GetModuleType() const override { return EModuleType::Lifetime; }
};

