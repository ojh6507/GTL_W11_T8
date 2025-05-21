#include "ParticleSystemComponent.h"
#include "ParticleEmitterInstances.h"
#include "ParticleLODLevel.h"
#include "UObject/UObjectIterator.h"
#include "ParticleSystem.h"
#include "UObject/Casts.h"
#include "Editor/UnrealEd/EditorViewportClient.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleModuleTypeDataMesh.h"
#include "Components/Mesh/StaticMeshRenderData.h"
#include "Engine/Asset/StaticMeshAsset.h"
#include "ParticleModuleTypeDataSprite.h"

UParticleSystemComponent::UParticleSystemComponent()
{
}

void UParticleSystemComponent::InitializeComponent()
{
    Super::InitializeComponent();
    InitParticles();
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
        EModuleType ModuleType = EmitterInstance->CurrentLODLevel->TypeDataModule->GetModuleType();
        if (ModuleType == EModuleType::TypeDataSprite)
        {
            FDynamicSpriteEmitterData* SpriteData = new FDynamicSpriteEmitterData(EmitterInstance->CurrentLODLevel->RequiredModule);
            EmitterInstance->FillReplayData(SpriteData->Source);

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
        else if (ModuleType == EModuleType::TypeDataMesh)
        {
            FDynamicMeshEmitterData* SpriteData = new FDynamicMeshEmitterData(EmitterInstance->CurrentLODLevel->RequiredModule);
            EmitterInstance->FillReplayData(SpriteData->Source);

            SpriteData->BuildViewFillData(
                EmitterInstance->ActiveParticles,
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
}

void UParticleSystemComponent::FillRenderData(const std::shared_ptr<FEditorViewportClient>& View)
{
    for (int32 Idx = 0; Idx < EmitterRenderData.Num(); ++Idx)
    {
        FParticleEmitterInstance* EmitterInstance = EmitterInstances[Idx];

        if (FDynamicSpriteEmitterData* SpriteData = dynamic_cast<FDynamicSpriteEmitterData*>(EmitterRenderData[Idx]))
        {
            int32 ParticleCount = SpriteData->Source.ActiveParticleCount;
           
            // 파티클 순서 정렬
            FParticleOrder* ParticleOrder = nullptr;
            if (EmitterInstance->bRequiresSorting && SpriteData->Source.SortMode)
            {
                ParticleOrder = (FParticleOrder*)FPlatformMemory::Malloc<EAllocationType::EAT_Container>(sizeof(FParticleOrder) * ParticleCount);

                SpriteData->SortSpriteParticles(SpriteData->Source.SortMode, SpriteData->Source.bUseLocalSpace, SpriteData->Source.ActiveParticleCount,
                    SpriteData->Source.DataContainer.ParticleData, SpriteData->Source.ParticleStride, SpriteData->Source.DataContainer.ParticleIndices,
                    View.get(), GetWorldMatrix(), ParticleOrder);
            }

            // 텍스쳐 매핑
            SpriteData->Texture = Cast<UParticleModuleTypeDataSprite>(EmitterInstance->CurrentLODLevel->TypeDataModule)->CachedTexture;

            // 렌더 데이터 채우기
            SpriteData->GetVertexAndIndexDataNonInstanced(
                /* OutVertexData: */       SpriteData->VertexAllocation.Buffer,
                /* OutParamData: */        SpriteData->ParamAllocation.Buffer,
                /* OutIndexData: */        SpriteData->IndexAllocation.Buffer,
                /* InParticleOrder: */     ParticleOrder,
                /* InViewOrigin: */        View->GetCameraLocation(),
                /* InLocalToWorld: */      GetWorldMatrix(),
                /* InVertsPerParticle: */  4
            );

            // 메모리 할당 해제
            if (ParticleOrder)
            {
                FPlatformMemory::Free<EAllocationType::EAT_Container>(ParticleOrder, sizeof(FParticleOrder) * ParticleCount);
            }
        }
        else if (FDynamicMeshEmitterData* SpriteData = dynamic_cast<FDynamicMeshEmitterData*>(EmitterRenderData[Idx]))
        {
            int32 ParticleCount = SpriteData->Source.ActiveParticleCount;

            // 파티클 순서 정렬
            FParticleOrder* ParticleOrder = nullptr;
            if (EmitterInstance->bRequiresSorting && SpriteData->Source.SortMode)
            {
                ParticleOrder = (FParticleOrder*)FPlatformMemory::Malloc<EAllocationType::EAT_Container>(sizeof(FParticleOrder) * ParticleCount);

                SpriteData->SortSpriteParticles(SpriteData->Source.SortMode, SpriteData->Source.bUseLocalSpace, SpriteData->Source.ActiveParticleCount,
                    SpriteData->Source.DataContainer.ParticleData, SpriteData->Source.ParticleStride, SpriteData->Source.DataContainer.ParticleIndices,
                    View.get(), GetWorldMatrix(), ParticleOrder);
            }

            // UStaticMesh 관련 처리 임시로 여기에 배치
            SpriteData->StaticMesh = Cast<UParticleModuleTypeDataMesh>(EmitterInstance->CurrentLODLevel->TypeDataModule)->Mesh;
            if (SpriteData->StaticMesh)
            {
                TArray<UMaterial*> Materials;
                SpriteData->StaticMesh->GetUsedMaterials(Materials);
                SpriteData->MeshMaterials = Materials;

                // 2) 실제 Vertex 버퍼 채우기
                SpriteData->GetVertexData(
                    /* OutVertexData: */       SpriteData->VertexAllocation.Buffer,
                    /* OutParamData: */        SpriteData->ParamAllocation.Buffer,
                    /* InParticleOrder: */     ParticleOrder,
                    /* InViewOrigin: */        View->GetCameraLocation(),
                    /* InLocalToWorld: */      GetWorldMatrix()
                );
            }

            // 메모리 할당 해제
            if (ParticleOrder)
            {
                FPlatformMemory::Free<EAllocationType::EAT_Container>(ParticleOrder, sizeof(FParticleOrder) * ParticleCount);
            }
        }
    }

    ClearRenderData();
}

void UParticleSystemComponent::ClearRenderData()
{
    DynamicVB.Commit();
    DynamicIB.Commit();

    for (auto* RenderData : EmitterRenderData)
    {
        RenderData->FreeSourceData();
    }
}

void UParticleSystemComponent::SetParticleTemplate(UParticleSystem* InTemplate)
{
    Template = InTemplate;
}
