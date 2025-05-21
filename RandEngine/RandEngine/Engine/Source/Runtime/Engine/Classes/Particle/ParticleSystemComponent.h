#pragma once
#include "ParticleSystem.h"
#include "Components/PrimitiveComponent.h"
#include "Renderer/GlobalRenderResource.h"

struct FParticleEmitterInstance;
struct FDynamicEmitterDataBase;
class FEditorViewportClient;
class UParticleSystemComponent : public UPrimitiveComponent 
{
    DECLARE_CLASS(UParticleSystemComponent, UPrimitiveComponent);

public:
    UParticleSystemComponent();
    ~UParticleSystemComponent() override = default;

    virtual void InitializeComponent() override;
    virtual void TickComponent(float DeltaTime) override;

    virtual int CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const override;

    void InitParticles();
    void PrepareRenderData();
    void FillRenderData(const std::shared_ptr<FEditorViewportClient>& View);
    void ClearRenderData();

    void SetParticleTemplate(UParticleSystem* InTemplate);
   

    FGlobalDynamicVertexBuffer DynamicVB;
    FGlobalDynamicIndexBuffer  DynamicIB;

    TArray<FParticleEmitterInstance*> EmitterInstances;
    UParticleSystem* Template = nullptr;

    TArray<FDynamicEmitterDataBase*> EmitterRenderData;
};
