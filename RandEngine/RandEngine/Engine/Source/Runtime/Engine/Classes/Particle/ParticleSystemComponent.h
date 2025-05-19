#pragma once
#include "ParticleSystem.h"
#include "Components/PrimitiveComponent.h"

struct FParticleEmitterInstance;
struct FDynamicEmitterDataBase;
class UParticleSystemComponent : public UPrimitiveComponent 
{
    DECLARE_CLASS(UParticleSystemComponent, UPrimitiveComponent);

public:
    UParticleSystemComponent();
    ~UParticleSystemComponent() override = default;

    void InitParticles();
    void PreareRenderData();


    TArray<FParticleEmitterInstance*> EmitterInstances;
    UParticleSystem* Template;

    TArray<FDynamicEmitterDataBase*> EmitterRenderData;
};
