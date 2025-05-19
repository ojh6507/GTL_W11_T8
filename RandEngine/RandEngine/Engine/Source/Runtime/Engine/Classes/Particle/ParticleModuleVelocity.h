#pragma once
#include "ParticleModule.h"

class UParticleModuleVelocity : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleVelocity, UParticleModule);
public:
    // 파티클의 최소 시작 속도
    FVector MinStartVelocity;

    // 파티클의 최대 시작 속도
    // MinStartVelocity와 MaxStartVelocity가 같으면 고정된 시작 속도가 됩니다.
    FVector MaxStartVelocity;

public:
    UParticleModuleVelocity();
    ~UParticleModuleVelocity() override = default;
    virtual EModuleType GetModuleType() const override { return EModuleType::Velocity; }

    friend FArchive& operator<<(FArchive& Ar, UParticleModuleVelocity& M);

    virtual void Serialize(FArchive& Ar) override // 오버라이드
    {
        Ar << MinStartVelocity;

        Ar << MaxStartVelocity;

    }
};
