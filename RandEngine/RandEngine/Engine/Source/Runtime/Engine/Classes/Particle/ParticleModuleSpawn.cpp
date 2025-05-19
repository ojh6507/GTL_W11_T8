#include "ParticleModuleSpawn.h"
UParticleModuleSpawn::UParticleModuleSpawn()
    : bProcessSpawnRate(true)
    , bProcessBurstList(true)
{
    Rate.DistributionType = EDistributionType::Constant;
    Rate.Constant = 10.0f;

    RateScale.DistributionType = EDistributionType::Constant;
    RateScale.Constant = 1.0f;
}

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

void UParticleModuleSpawn::Serialize(FArchive& Ar)
{
    // 1. 부모 클래스(UParticleModule)의 Serialize 호출
    Super::Serialize(Ar);

    // 2. Rate (FDistributionFloat) 직렬화
    Ar << Rate;

    // 3. RateScale (FDistributionFloat) 직렬화
    Ar << RateScale;

    // 4. BurstList (TArray<FParticleBurst>) 직렬화
    if (Ar.IsLoading())
    {
        int32 NumBursts = 0;
        Ar << NumBursts; // 배열 크기 읽기
        BurstList.Empty();
        BurstList.Reserve(NumBursts);
        for (int32 i = 0; i < NumBursts; ++i)
        {
            FParticleBurst NewBurst;
            Ar << NewBurst;
            BurstList.Add(NewBurst);
        }
    }
    else // Saving
    {
        int32 NumBursts = BurstList.Num();
        Ar << NumBursts; // 배열 크기 쓰기
        for (const FParticleBurst& Burst : BurstList)
        {
            FParticleBurst TempBurst = Burst;
            Ar << TempBurst;
        }
    }

    // 5. bProcessSpawnRate (bool) 직렬화
    Ar << bProcessSpawnRate;

    // 6. bProcessBurstList (bool) 직렬화
    Ar << bProcessBurstList;
}

FArchive& operator<<(FArchive& Ar, UParticleModuleSpawn& M)
{
    M.Serialize(Ar);
    return Ar;
}
