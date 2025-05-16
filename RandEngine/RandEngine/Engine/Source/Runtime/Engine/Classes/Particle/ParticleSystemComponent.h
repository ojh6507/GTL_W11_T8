#pragma once
#include "ParticleSystem.h"
#include "Components/PrimitiveComponent.h"

struct FDynamicEmitterDataBase;
class UParticleSystemComponent : public UPrimitiveComponent 
{
    TArray<struct FParticleEmitterInstance*> EmitterInstances;
    UParticleSystem* Template;

    TArray<FDynamicEmitterDataBase*> EmitterRenderData;
};
