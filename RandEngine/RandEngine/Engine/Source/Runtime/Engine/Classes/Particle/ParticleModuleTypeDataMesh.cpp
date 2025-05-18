#include "ParticleModuleTypeDataMesh.h"
#include "Engine/AssetManager.h"
#include "ParticleEmitter.h"
#include "Engine/Engine.h"
#include "UObject/Casts.h"
#include "Runtime/Windows/SubWindow/SubEngine.h"
#include "Components/Mesh/StaticMeshRenderData.h"

void UParticleModuleTypeDataMesh::Build(const FParticleEmitterBuildInfo& EmitterBuildInfo)
{
    Super::Build(EmitterBuildInfo);

    if (!Mesh && !MeshAssetPath.IsEmpty())
    {
        UE_LOG(ELogLevel::Warning, TEXT("UParticleModuleTypeDataMesh::Build (Runtime) - Mesh is null but MeshAssetPath ('%s') exists. Mesh should have been serialized."), *MeshAssetPath);

    }

    if ((EmitterBuildInfo.bIsEditorBuild || !Mesh) && !MeshAssetPath.IsEmpty())
    {
        Mesh = UAssetManager::Get().GetStaticMesh(MeshAssetPath);

        if (Mesh)
        {
            UE_LOG(ELogLevel::Display, TEXT("Successfully 'loaded' mesh: %s"), *Mesh->GetName());
        }
        else
        {
            UE_LOG(ELogLevel::Warning, TEXT("Failed to 'load' mesh from path: %s"), *this->MeshAssetPath);
        }
    }

    // 런타임에는 이미 직렬화된 Mesh 포인터가 로드되어 있거나,
    // 여기서 로드된 Mesh 포인터를 사용합니다.
}
