#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UParticleLODLevel;
class UParticleEmitter : public UObject
{
    DECLARE_CLASS(UParticleEmitter, UObject);
    
public:
    UParticleEmitter();
    ~UParticleEmitter() override = default;

    void CacheEmitterModuleInfo();

    TArray<UParticleLODLevel*> LODLevels;
};

