#pragma once
#include "ParticleModule.h"

class UParticleModuleLifetime : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleLifetime, UParticleModule);

public:
    UParticleModuleLifetime();
    ~UParticleModuleLifetime() override = default;

};

