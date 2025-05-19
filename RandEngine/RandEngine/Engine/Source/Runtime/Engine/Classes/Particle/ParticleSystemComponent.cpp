#include "ParticleSystemComponent.h"
#include "ParticleEmitterInstances.h"
#include "ParticleLODLevel.h"
#include "ParticleSystem.h"

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
        int VerticesPerParticle = 4;
        SpriteData->BuildViewFillData(
            EmitterInstance->ActiveParticles * VerticesPerParticle,
            SpriteData->GetDynamicVertexStride(),
            SpriteData->GetDynamicParameterVertexStride(),
            DynamicIB,
            DynamicVB,
            SpriteData->VertexAllocation,
            SpriteData->IndexAllocation,
            &SpriteData->ParamAllocation,
            SpriteData->AsyncFillData
        );

        EmitterRenderData.Add(SpriteData);
    }
}
