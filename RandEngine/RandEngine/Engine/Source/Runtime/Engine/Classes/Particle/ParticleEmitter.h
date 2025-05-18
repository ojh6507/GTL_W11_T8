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
    bool bRequiresLoopNotification;
    bool bMeshRotationActive; // 메시 회전이 필요한지 여부 (주로 TouchesMeshRotation() 결과)
    TMap<UParticleModule*, int32> ModuleOffsetMap; // 파티클 데이터 내 모듈 페이로드 오프셋
    TMap<UParticleModule*, int32> ModuleInstanceOffsetMap; // 이미터 인스턴스 데이터 내 모듈 페이로드 오프셋
    TArray<UParticleModule*> ModulesNeedingInstanceData; // 인스턴스 데이터가 필요한 모듈 목록
    int32 DynamicParameterDataOffset; // UParticleModuleParameterDynamic 지원 시 페이로드 오프셋
    int32 ParticleSize; // 최종 계산된 단일 파티클의 총 크기 (FBaseParticle + 모든 모듈 페이로드)
    int32 ReqInstanceBytes; // 이미터 인스턴스당 필요한 총 추가 바이트
    int32 TypeDataOffset; // TypeDataModule 페이로드의 시작 오프셋
    int32 TypeDataInstanceOffset; // TypeDataModule의 인스턴스별 데이터 오프셋

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
