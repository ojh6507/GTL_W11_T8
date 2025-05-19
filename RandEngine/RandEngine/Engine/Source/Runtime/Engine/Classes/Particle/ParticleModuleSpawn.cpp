#include "ParticleModuleSpawn.h"

float UParticleModuleSpawn::GetEffectiveSpawnRate(float EmitterTime) const
{
    if (!bProcessSpawnRate || !bEnabled)
    {
        return 0.0f;
    }

    // FDistributionFloat의 GetValue 함수를 사용하여 현재 시간의 스폰율과 스케일 값을 가져옵니다.
    // OwnerContext는 Parameter 타입 분포를 사용할 경우 필요할 수 있습니다.
    float BaseRate = Rate.GetValue(EmitterTime);
    float Scale = RateScale.GetValue(EmitterTime);

    return BaseRate * Scale;
}

int32 UParticleModuleSpawn::GetBurstAmountForThisTick(float EmitterTime, float DeltaTime) const
{
    if (!bProcessBurstList || !bEnabled || BurstList.IsEmpty() || DeltaTime <= 0.0f)
    {
        return 0;
    }

    int32 TotalBurstParticlesThisTick = 0;
    float PreviousEmitterTime = EmitterTime - DeltaTime;

    for (const FParticleBurst& Burst : BurstList)
    {
        if (Burst.Time > PreviousEmitterTime && Burst.Time <= EmitterTime)
        {
            TotalBurstParticlesThisTick += Burst.Count;
        }
        else if (PreviousEmitterTime < 0 && Burst.Time == 0.0f && EmitterTime >= 0.0f) // 첫 프레임 Time=0 버스트
        {
            TotalBurstParticlesThisTick += Burst.Count;
        }
    }
    return TotalBurstParticlesThisTick;
}

int32 UParticleModuleSpawn::GetTotalBurstAmountSimple() const
{
    if (!bProcessBurstList || !bEnabled) return 0;
    int32 Total = 0;
    for (const FParticleBurst& Burst : BurstList) { Total += Burst.Count; }
    return Total;
}
