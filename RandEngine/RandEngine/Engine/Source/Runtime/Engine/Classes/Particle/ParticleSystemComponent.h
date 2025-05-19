#pragma once
#include "ParticleSystem.h"
#include "Components/PrimitiveComponent.h"
#include "Renderer/GlobalRenderResource.h"

struct FParticleEmitterInstance;
struct FDynamicEmitterDataBase;
class UParticleSystemComponent : public UPrimitiveComponent 
{
    DECLARE_CLASS(UParticleSystemComponent, UPrimitiveComponent);

public:
    UParticleSystemComponent();
    ~UParticleSystemComponent() override = default;

    void InitParticles();
    void PrepareRenderData();

    FGlobalDynamicVertexBuffer DynamicVB;
    FGlobalDynamicIndexBuffer  DynamicIB;

    TArray<FParticleEmitterInstance*> EmitterInstances;
    UParticleSystem* Template;

    TArray<FDynamicEmitterDataBase*> EmitterRenderData;
};
