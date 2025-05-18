#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

// 전방 선언
class UParticleLODLevel;
class UParticleModule;
class UParticleModuleRequired;
class UParticleModuleSpawn;
class UParticleModuleTypeDataBase;

// --- FParticleEmitterBuildInfo 구조체 정의 ---
struct FParticleEmitterBuildInfo
{
    UParticleLODLevel* OwningLODLevel;
    UParticleModuleRequired* RequiredModule;
    UParticleModuleSpawn* SpawnModule;
    UParticleModuleTypeDataBase* CurrentTypeDataModule;
    int32 EstimatedMaxActiveParticleCount;
    bool bIsEditorBuild;

public:
    FParticleEmitterBuildInfo()
        : OwningLODLevel(nullptr)
        , RequiredModule(nullptr)
        , SpawnModule(nullptr)
        , CurrentTypeDataModule(nullptr)
        , EstimatedMaxActiveParticleCount(0)
        , bIsEditorBuild(false) {
    }
};


class UParticleEmitter : public UObject
{
    DECLARE_CLASS(UParticleEmitter, UObject);

public:
    FString EmitterName;

    TArray<UParticleLODLevel*> LODLevels; // 실제로는 LODLevels[0]만 사용

    // --- CacheEmitterModuleInfo 결과 저장용 멤버 ---
    bool bRequiresLoopNotification_Cached;
    bool bMeshRotationActive_Cached;

    TMap<UParticleModule*, int32>ModuleOffsetMap_Cached; // 파티클 데이터 내 각 모듈 페이로드의 시작 오프셋 (FBaseParticle 이후)
    TMap<UParticleModule*, int32> ModuleInstanceOffsetMap_Cached; // 이미터 인스턴스 데이터 내 각 모듈 페이로드의 시작 오프셋
    TArray<UParticleModule*> ModulesNeedingInstanceData_Cached; // 인스턴스 데이터가 필요한 모듈 목록

    int32 EstimatedMaxActiveParticles_Cached; // 예상 최대 활성 파티클 수 (BuildInfo로부터 가져옴)

    int32 ParticleSize_Cached;            // 최종 계산된 단일 파티클의 총 메모리 크기
    int32 ReqInstanceBytes_Cached;        // 이미터 인스턴스당 필요한 총 추가 바이트 (모든 모듈 합산)
    int32 TypeDataOffset_Cached;          // TypeDataModule 페이로드의 시작 오프셋 (ParticleSize_Cached 내)
    int32 TypeDataInstanceOffset_Cached;  // TypeDataModule의 인스턴스별 데이터 오프셋 (ReqInstanceBytes_Cached 내)

    // --- 현재 지원 안 함 --- 의논 필요
    // int32 SubUVDataOffset_Cached;
    // int32 DynamicParameterDataOffset_Cached;
    // int32 OrbitModuleOffset_Cached;
    // int32 LightDataOffset_Cached;
    // float LightVolumetricScatteringIntensity_Cached;
    // int32 CameraPayloadOffset_Cached;
    // bool bAxisLockEnabled_Cached;
    // int32 LockAxisFlags_Cached;

public:
    UParticleEmitter();
    virtual ~UParticleEmitter() = default;

    // 이미터 설정 기반으로 런타임 데이터 레이아웃 및 상태 캐싱
    void CacheEmitterModuleInfo();
    // 에디터에서 또는 로드 시 호출되어 이미터 빌드 (주로 TypeDataModule 빌드)
    void Build();

    // LOD 0의 UParticleLODLevel 반환 (유효성 검사 포함)
    UParticleLODLevel* GetHighestLODLevel() const;
};
