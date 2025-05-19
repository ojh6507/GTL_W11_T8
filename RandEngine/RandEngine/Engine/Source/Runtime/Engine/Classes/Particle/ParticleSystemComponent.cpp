#include "ParticleSystemComponent.h"
#include "ParticleEmitterInstances.h"
#include "ParticleLODLevel.h"
#include "ParticleSystem.h"
#include "UObject/Casts.h"

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

void UParticleSystemComponent::FillRenderData(const FVector& InCameraPosition, const FMatrix& InLocalToWorld)
{
    for (int32 Idx = 0; Idx < EmitterRenderData.Num(); ++Idx)
    {
        FDynamicSpriteEmitterData* SpriteData = Cast<FDynamicSpriteEmitterData>(EmitterRenderData[Idx]);
        FParticleEmitterInstance* EmitterInstance = EmitterInstances[Idx];

        // 1) 파티클 순서 정렬 (투명 블렌딩시 뒤→앞 순서 보장을 위해)
        //TArray<int32> ParticleOrder = EmitterInstance->GetParticleIndices();

        // 2) 실제 Vertex/Index 버퍼 채우기
        SpriteData->GetVertexAndIndexDataNonInstanced(
            /* OutVertexData: */       SpriteData->VertexAllocation.Buffer,
            /* OutParamData: */        SpriteData->ParamAllocation.Buffer,
            /* OutIndexData: */        SpriteData->IndexAllocation.Buffer,
            /* InParticleOrder: */     nullptr,
            /* InViewOrigin: */        InCameraPosition,
            /* InLocalToWorld: */      InLocalToWorld,
            /* InVertsPerParticle: */  4
        );
    }
}
