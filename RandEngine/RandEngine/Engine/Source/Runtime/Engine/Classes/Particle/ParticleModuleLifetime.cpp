#include "ParticleModuleLifetime.h"

UParticleModuleLifetime::UParticleModuleLifetime()
{
}
void UParticleModuleLifetime::SpawnParticle(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle& Particle)
{
    // Lifetime 분포로부터 현재 파티클의 최대 수명 값을 가져옴
    // GetValue()는 분포 타입에 따라 다른 값을 반환 (상수, 균일분포 내 랜덤값 등)
   
    float MaxLifetimeForThisParticle = Lifetime.GetValue(SpawnTime);

    if (MaxLifetimeForThisParticle <= 0.0f)
    {
        // 수명이 0 이하면 파티클이 즉시 죽도록 처리 (또는 아주 작은 값으로 설정)
        Particle.OneOverMaxLifetime = FLT_MAX; // 매우 큰 값으로 설정하여 RelativeTime이 즉시 1.0을 넘도록 함
        Particle.RelativeTime = 1.0f;         // 즉시 죽음 상태로 설정
    }
    else
    {
        Particle.OneOverMaxLifetime = 1.0f / MaxLifetimeForThisParticle;
    }
    // Particle.Flags 등 다른 초기화는 FParticleEmitterInstance::SpawnParticles에서 처리
}
void UParticleModuleLifetime::UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle,
                                             const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset, float DeltaTime)
{

}
FArchive& operator<<(FArchive& Ar, UParticleModuleLifetime& M)
{
    M.Serialize(Ar);
    return Ar;
}
