#pragma once
#include "UObject/Object.h"
#include "ParticleHelper.h"  
#include "ParticleDefines.h"
#include "UObject/ObjectMacros.h"

class UParticleEmitter;
class UParticleModuleTypeDataBase;
struct FParticleEmitterBuildInfo;
struct FParticleEmitterInstance;

class UParticleModule : public UObject
{
    DECLARE_CLASS(UParticleModule, UObject);

public:
    bool bEnabled; // 이 모듈이 활성화되었는지 여부

    UParticleModule() : bEnabled(true) {}
    virtual ~UParticleModule() = default;

    // --- 모듈 타입 식별 ---
    virtual EModuleType GetModuleType() const { return EModuleType::Base; }
    virtual FString GetModuleDisplayName() const
    {
        switch (GetModuleType())
        {
        case EModuleType::Required:
            return TEXT("Required Module");
        case EModuleType::Base:
            return TEXT("Base Module");
        case EModuleType::Spawn:
            return TEXT("Spawn Module");
        case EModuleType::Color:
            return TEXT("Color Over Life");
        case EModuleType::Velocity:
            return TEXT("Initial Velocity");
        case EModuleType::Size:
            return TEXT("Initial Size");
        case EModuleType::Lifetime:
            return TEXT("Lifetime Module");
        case EModuleType::Location:
            return TEXT("Initial Location");
        case EModuleType::TypeDataMesh:
            return TEXT("Mesh TypeData");
        case EModuleType::TypeDataSprite:
            return TEXT("Sprite TypeData");
        default:
            return TEXT("Unknown Module");
        }
    }
    // --- 파티클 데이터 요구량 ---
    // 이 모듈이 파티클별 데이터(FBaseParticle 이후에 붙는)에 필요한 바이트 수
    virtual int32 RequiredBytes(UParticleModuleTypeDataBase* /*SpawningTypeData*/) const { return 0; }

    // 이 모듈이 이미터 인스턴스별 데이터에 필요한 바이트 수
    virtual int32 RequiredBytesPerInstance() const { return 0; }

    // --- 이미터 상태 관련 ---
    virtual bool RequiresLoopingNotification() const { return false; } // 루프 알림 필요 여부
    virtual bool TouchesMeshRotation() const { return false; }      // 메시 회전에 영향 주는지 여부

    // --- 에디터 기능 관련 ---
    // 이 모듈이 빌드 과정을 필요로 하는지 (주로 TypeDataModule에서 true)
    virtual bool RequiresBuild() const { return false; }
    // 에디터에서 빌드 정보 수집 시 호출 (주로 UParticleLODLevel::CompileModules에서 호출됨)
    virtual void CompileModule(FParticleEmitterBuildInfo& /*EmitterBuildInfo*/) {}
    // 실제 빌드 로직 (주로 TypeDataModule에서 구현)
    virtual void Build(const FParticleEmitterBuildInfo& /*EmitterBuildInfo*/) {}
    // 이미터 정보 캐싱 (주로 TypeDataModule에서 구현)
    virtual void CacheModuleInfo(UParticleEmitter* /*Emitter*/) {}

    // --- 파티클 생명주기 함수 (가상 함수로 만들어 파생 클래스에서 오버라이드) ---
    // 파티클 스폰 직후 호출 (초기값 설정 등)
    virtual void SpawnParticle(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle& ParticleBase) {}

    // 파티클 매 틱 업데이트 시 호출
    virtual void UpdateParticle(FParticleEmitterInstance* Owner,
        FBaseParticle& Particle,
        const uint8* ParticleBaseForPayload, // PARTICLE_ELEMENT용
        int32 PayloadRelativeOffset,       // PARTICLE_ELEMENT용 CurrentOffset 초기값
        float DeltaTime)
    {
    }

    // 파티클 소멸 시 호출 (필요하다면)
    virtual void FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) {}
    friend FArchive& operator<<(FArchive& Ar, UParticleModule& M);
    virtual void Serialize(FArchive& Ar)
    {
        Ar << bEnabled;
    }
};
