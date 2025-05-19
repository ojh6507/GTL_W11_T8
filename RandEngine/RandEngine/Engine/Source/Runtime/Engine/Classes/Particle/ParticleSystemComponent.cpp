#include "ParticleSystemComponent.h"
#include "ParticleEmitterInstances.h"
#include "ParticleLODLevel.h"
#include "UObject/UObjectIterator.h"
#include "ParticleSystem.h"
#include "UObject/Casts.h"

UParticleSystemComponent::UParticleSystemComponent()
{
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    // 파티클 시스템의 모든 이미터 인스턴스에 대해 Tick 호출
    for (FParticleEmitterInstance* EmitterInstance : EmitterInstances)
    {
        if (EmitterInstance)
        {
            EmitterInstance->Tick(DeltaTime);
        }
    }
}

void UParticleSystemComponent::InitParticles()
{
    for (auto pcs : TObjectRange<UParticleSystem>())
    {
        Template = pcs;
        break;
    }
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

void UParticleSystemComponent::FillRenderData(const FVector& InCameraPosition)
{
    for (int32 Idx = 0; Idx < EmitterRenderData.Num(); ++Idx)
    {
        FDynamicSpriteEmitterData* SpriteData = static_cast<FDynamicSpriteEmitterData*>(EmitterRenderData[Idx]);
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
            /* InLocalToWorld: */      GetWorldMatrix(),
            /* InVertsPerParticle: */  4
        );
    }
}

void UParticleSystemComponent::SetParticleTemplate(UParticleSystem* InTemplate)
{
    Template = InTemplate;
}
