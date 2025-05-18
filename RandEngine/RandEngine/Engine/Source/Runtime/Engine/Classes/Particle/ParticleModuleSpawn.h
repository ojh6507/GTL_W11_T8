#pragma once
#include "ParticleModule.h"
#include "Math/DistributionFloat.h"


// 간단한 파티클 버스트 구조체
struct FParticleBurst
{
    int32 Count;
    float Time;
    FParticleBurst() : Count(0), Time(0.0f) {}
};

class UParticleModuleSpawn : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleSpawn, UParticleModule);

public:
    // 파티클 생성 속도 (초당 파티클 수)
    FDistributionFloat Rate;
    FDistributionFloat RateScale;

    // 버스트 목록
    TArray<FParticleBurst> BurstList;

    // ... 기타 스폰 관련 설정들 (예: bApplyGlobalSpawnRateScale) ...
    bool bProcessSpawnRate; // Rate를 사용할지 여부
    bool bProcessBurstList; // BurstList를 사용할지 여부

    UParticleModuleSpawn()
        : bProcessSpawnRate(true)
        , bProcessBurstList(true)
    {
        Rate.DistributionType = EDistributionType::Constant;
        Rate.Constant = 10.0f;

        RateScale.DistributionType = EDistributionType::Constant;
        RateScale.Constant = 1.0f;
    }

    virtual EModuleType GetModuleType() const override { return EModuleType::Spawn; }

    /**
   * 현재 이미터 시간(EmitterTime)를 기준으로
   * 초당 파티클 생성률을 반환합니다. Rate 및 RateScale 분포 설정을 고려합니다.
   */
    float GetEffectiveSpawnRate(float EmitterTime) const;

    /**
      * 이번 델타 시간(DeltaTime) 동안, 현재 이미터 시간(EmitterTime)을 기준으로
      * 시간상으로 발생해야 하는 모든 버스트의 총 파티클 수를 반환합니다.
      */
    int32 GetBurstAmountForThisTick(float EmitterTime, float DeltaTime) const;

    /**
     * (CalculateMaxActiveParticleCount 용도)
     * BurstList에 정의된 모든 버스트의 총 파티클 수를 반환합니다.
     * (루프에 따른 중첩은 고려하지 않는 단순 합계)
     */
    int32 GetTotalBurstAmountSimple() const;
};
