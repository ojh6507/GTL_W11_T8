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
        UE_LOG(ELogLevel::Warning, TEXT("UParticleModuleTypeDataMesh::Build (Runtime) - Mesh is null but MeshAssetPath ('%s') exists."), *MeshAssetPath);

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
void UParticleModuleTypeDataMesh::SpawnParticle(FParticleEmitterInstance* Owner,
                                                int32 PayloadRelativeOffset, // FBaseParticle 이후 이 모듈 페이로드 시작 오프셋
                                                float SpawnTime,
                                                FBaseParticle& Particle)     // FBaseParticle 멤버 직접 접근용
{
 
    if (PayloadRelativeOffset < 0)
    {
        return;
    }

    // PARTICLE_ELEMENT 매크로가 사용할 변수 설정
    // ParticleBase는 현재 FBaseParticle 객체의 시작 주소를 가리켜야 합니다.
    const uint8* ParticleBase = reinterpret_cast<const uint8*>(&Particle);
  
    // CurrentOffset은 전달받은 PayloadRelativeOffset (FBaseParticle 이후의 상대 오프셋)으로 시작합니다.
    uint32 CurrentOffset = static_cast<uint32>(PayloadRelativeOffset);

    PARTICLE_ELEMENT(FMeshParticlePayload, MeshPayload);

    //MeshPayload.Scale = this->MeshScale.X;
    // 또는 랜덤 스케일 적용
    MeshPayload.Scale = FMath::FRandRange(0.8f, 1.2f) * this->MeshScale.X;

}

void UParticleModuleTypeDataMesh::UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle,
                                                const uint8* ParticleBaseForPayload,
                                                int32 PayloadRelativeOffset,
                                                float DeltaTime)    
{

    if (PayloadRelativeOffset < 0) return;

    // PARTICLE_ELEMENT 매크로가 사용할 변수 설정 (SpawnParticle과 동일한 방식)
    const uint8* ParticleBase = ParticleBaseForPayload; // 전달받은 기준 주소 사용
    uint32 CurrentOffset = static_cast<uint32>(PayloadRelativeOffset);

    PARTICLE_ELEMENT(FMeshParticlePayload, MeshPayload);

    MeshPayload.Scale += 0.1f * DeltaTime * Particle.RelativeTime;
    // 또는 회전 업데이트
    // MeshPayload.RotationAngle += SomeAngularSpeed * DeltaTime;
    // (실제 회전 적용은 렌더링 시 또는 FBaseParticle::Rotation과 조합하여 처리)
}
void UParticleModuleTypeDataMesh::Serialize(FArchive & Ar)
{
    Super::Serialize(Ar);

    Ar << MeshAssetPath;

    Ar << MeshScale;

}
FArchive& operator<<(FArchive& Ar, UParticleModuleTypeDataMesh& M)
{
    M.Serialize(Ar);
    return Ar;
}
