#include "ParticleSystemComponent.h"
#include "ParticleEmitterInstances.h"
#include "ParticleHelper.h"
#include "ParticleLODLevel.h"

UParticleSystemComponent::UParticleSystemComponent()
{
}

void UParticleSystemComponent::InitParticles()
{
    if (Template)
    {
        Template->InitializeSystem();

        if (EmitterInstances.Num() > 0)
        {
            for (FParticleEmitterInstance* EmitterInstance : EmitterInstances)
            {
                delete EmitterInstance;
            }
            EmitterInstances.Empty();
        }

        for (UParticleEmitter* Emitter : Template->Emitters)
        {
            if (Emitter)
            {
                FParticleEmitterInstance* EmitterInstance = new FParticleEmitterInstance();
                EmitterInstance->InitParameters(Emitter, this);
                EmitterInstances.Add(EmitterInstance);
            }
        }
    }
}

void UParticleSystemComponent::PrepareRenderData()
{
    EmitterRenderData.Empty();
    EmitterRenderData.Reserve(EmitterInstances.Num());
    for (FParticleEmitterInstance* EmitterInstance : EmitterInstances)
    {
        FDynamicSpriteEmitterData* SpriteData = new FDynamicSpriteEmitterData(EmitterInstance->CurrentLODLevel->RequiredModule);
        //SpriteData->BuildViewFillData(
        //    ViewportClient,
        //    Inst->ActiveParticles * VerticesPerParticle,
        //    SpriteData->GetDynamicVertexStride(),
        //    SpriteData->GetDynamicParameterVertexStride(),
        //    GDynamicIndexBufferPool,  // 내부에 GlobalDynamicIB 래핑
        //    GDynamicVertexBufferPool, // 내부에 GlobalDynamicVB 래핑
        //    SpriteData->VertexAllocation,
        //    SpriteData->IndexAllocation,
        //    &SpriteData->ParamAllocation,
        //    SpriteData->AsyncFillData
        //)
        EmitterRenderData.Add(SpriteData);
    }
}
