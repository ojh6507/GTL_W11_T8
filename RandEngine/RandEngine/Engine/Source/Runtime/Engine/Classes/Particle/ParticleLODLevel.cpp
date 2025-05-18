#include "ParticleLODLevel.h"
#include "ParticleEmitter.h"
#include "UObject/Casts.h"

int32 UParticleLODLevel::CalculateMaxActiveParticleCount()
{
     //실제 구현에서는 RequiredModule, SpawnModule, LifetimeModule 등을 참조하여 계산
     //float SpawnRate = 0.0f;
     //float Lifetime = 1.0f;
     //if (RequiredModule) { /* ... */ }
     //for (UParticleModule* Mod : Modules) {
     //    if (UParticleModuleSpawn* SpawnMod = Cast<UParticleModuleSpawn>(Mod)) {
     //         SpawnRate = SpawnMod->Rate.GetValue(); // 분포 값 가져오기
     //    } else if (UParticleModuleLifetime* LifetimeMod = Cast<UParticleModuleLifetime>(Mod)) {
     //         Lifetime = LifetimeMod->Lifetime.GetValue();
     //    }
     //}
     //return static_cast<int32>(SpawnRate * Lifetime) + 10; // 매우 단순화된 예시
    return 50; // 임시 값
}
void UParticleLODLevel::CompileModules(FParticleEmitterBuildInfo& EmitterBuildInfo)
{
    EmitterBuildInfo.OwningLODLevel = this;
    EmitterBuildInfo.RequiredModule = this->RequiredModule;
    EmitterBuildInfo.CurrentTypeDataModule = this->TypeDataModule;

    // Spawn 모듈 찾아서 할당 (Modules 배열에 있다고 가정)
    EmitterBuildInfo.SpawnModule = nullptr; // 기본값
    for (UParticleModule* Mod : this->Modules)
    {
        //if (UParticleModuleSpawn* SpawnMod = Cast<UParticleModuleSpawn>(Mod)) // Cast가 작동한다고 가정
        //{
        //    EmitterBuildInfo.SpawnModule = SpawnMod;
        //    break;
        //}
    }

    // 예상 최대 파티클 수 계산
    EmitterBuildInfo.EstimatedMaxActiveParticleCount = CalculateMaxActiveParticleCount();

    // TypeDataModule이 메시 타입이라면, 메시 경로 같은 추가 정보를 EmitterBuildInfo에 설정할 수 있음
    // (하지만 UParticleModuleTypeDataMesh::Build 에서 직접 MeshAssetPath를 사용하므로 여기서는 필수 아님)
    // if (this->TypeDataModule)
    // {
    //     if (UParticleModuleTypeDataMesh* MeshTypeData = Cast<UParticleModuleTypeDataMesh>(this->TypeDataModule))
    //     {
    //         // EmitterBuildInfo 에는 이미 CurrentTypeDataModule 로 TypeDataModule 참조가 있으므로,
    //         // UParticleModuleTypeDataMesh::Build 에서 직접 MeshTypeData->MeshAssetPath 접근 가능.
    //         // 특별히 EmitterBuildInfo 에 중복해서 넣을 필요는 없을 수 있음.
    //     }
    // }

    // (선택적) 다른 일반 모듈들의 CompileModule 호출
    // if (this->RequiredModule) this->RequiredModule->CompileModule(EmitterBuildInfo);
    // if (EmitterBuildInfo.SpawnModule) EmitterBuildInfo.SpawnModule->CompileModule(EmitterBuildInfo);
    // for (UParticleModule* Mod : this->Modules)
    // {
    //     if (Mod) Mod->CompileModule(EmitterBuildInfo);
    // }
}
