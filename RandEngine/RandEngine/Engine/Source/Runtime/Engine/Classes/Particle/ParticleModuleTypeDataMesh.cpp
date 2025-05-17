#include "ParticleModuleTypeDataMesh.h"
#include "ParticleEmitter.h"
void UParticleModuleTypeDataMesh::Build(const FParticleEmitterBuildInfo& EmitterBuildInfo)
{
    Super::Build(EmitterBuildInfo); 

    if ((EmitterBuildInfo.bIsEditorBuild || !this->Mesh) && !this->MeshAssetPath.IsEmpty())
    {
       
        // this->Mesh = LoadDummyMesh(this->MeshAssetPath); // 실제 메시 로딩 함수 호출
                                                      // LoadObject<UStaticMesh>(nullptr, *this->MeshAssetPath); 와 유사

        // if (this->Mesh)
        // {
        //     // UE_LOG(LogTemp, Log, TEXT("Successfully 'loaded' mesh: %s"), *this->Mesh->GetName());
        // }
        // else
        // {
        //     // UE_LOG(LogTemp, Warning, TEXT("Failed to 'load' mesh from path: %s"), *this->MeshAssetPath);
        // }
    }
    // 런타임에는 이미 직렬화된 Mesh 포인터가 로드되어 있거나,
    // 여기서 로드된 Mesh 포인터를 사용합니다.
}
