#pragma once
#pragma once
#include "ParticleModuleTypeDataBase.h"

class UParticleModuleTypeDataSprite : public UParticleModuleTypeDataBase
{
    DECLARE_CLASS(UParticleModuleTypeDataSprite, UParticleModuleTypeDataBase);
public:
    UParticleModuleTypeDataSprite() {}
    virtual EModuleType GetModuleType() const override { return EModuleType::TypeDataSprite; }
 
    FString TextureAssetPath;
    FTexture* CachedTexture = nullptr;
    virtual void Build(const FParticleEmitterBuildInfo& EmitterBuildInfo) override;
    virtual int32 RequiredBytes(UParticleModuleTypeDataBase* SpawningTypeData) const override { return 0; }
    virtual int32 RequiredBytesPerInstance() const override { return 0; }
};
