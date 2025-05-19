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

    virtual void TickComponent(float DeltaTime) override;

    void InitParticles();
    void PrepareRenderData();
    void FillRenderData(const FVector& InCameraPosition);

    FGlobalDynamicVertexBuffer DynamicVB;
    FGlobalDynamicIndexBuffer  DynamicIB;

    TArray<FParticleEmitterInstance*> EmitterInstances;
    UParticleSystem* Template;

    TArray<FDynamicEmitterDataBase*> EmitterRenderData;
};
