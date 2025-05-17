#pragma once
#include "ParticleModule.h"

// 간단한 파티클 버스트 구조체
struct FParticleBurst
{
    int32 Count;    // 한 번에 생성할 파티클 수
    float Time;     // 이미터 시작 후 버스트가 발생할 시간
    // int32 CountLow; // 최소/최대 범위로 스폰할 경우 (선택적)

    FParticleBurst() : Count(0), Time(0.0f) {}
};

class UParticleModuleSpawn : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleSpawn, UParticleModule);

public:
    // 파티클 생성 속도 (초당 파티클 수)
    float Rate; 
    float RateScale;

    // 버스트 목록
    TArray<FParticleBurst> BurstList;

    // ... 기타 스폰 관련 설정들 (예: bApplyGlobalSpawnRateScale) ...
    bool bProcessSpawnRate; // Rate를 사용할지 여부
    bool bProcessBurstList; // BurstList를 사용할지 여부

    UParticleModuleSpawn()
        : bProcessSpawnRate(true)
        , bProcessBurstList(true)
    {
        // Rate, RateScale 기본값 설정 (예: 상수 분포로 10개/초)
        // Rate.Distribution = NewObject<UDistributionFloatConstant>();
        // Cast<UDistributionFloatConstant>(Rate.Distribution)->Constant = 10.0f;
        // RateScale.Distribution = NewObject<UDistributionFloatConstant>();
        // Cast<UDistributionFloatConstant>(RateScale.Distribution)->Constant = 1.0f;
    }

    virtual EModuleType GetModuleType() const override { return EModuleType::Spawn; }

    // 스폰 관련 헬퍼 함수 (FParticleEmitterInstance에서 사용될 수 있음)
    float GetSpawnRate(float EmitterTime) const; // 현재 시간에 맞는 스폰율 반환
    int32 GetBurstCount(float EmitterTime, float DeltaTime) const; // 이번 틱에 발생할 버스트 파티클 수 반환
};
