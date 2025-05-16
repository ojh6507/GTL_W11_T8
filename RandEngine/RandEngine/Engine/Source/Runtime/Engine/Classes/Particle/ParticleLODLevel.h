#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UParticleModule;
class UParticleModuleRequired;
class UParticleModuleTypeDataBase;
class UParticleLODLevel : public UObject
{
    DECLARE_CLASS(UParticleLODLevel, UObject);

public:
    UParticleLODLevel();
    ~UParticleLODLevel() override = default;

    int32 Level;
    bool bEnabled;

    UParticleModuleRequired* RequiredModule;
    TArray<UParticleModule*> Modules;
    UParticleModuleTypeDataBase* TypeDataModule;
};

