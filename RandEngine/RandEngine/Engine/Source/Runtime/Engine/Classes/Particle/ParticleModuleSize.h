#pragma once
#include "ParticleModule.h"

class UParticleModuleSize : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleSize, UParticleModule);
public:
    FVector StartSize;
    FVector EndSize;
    bool bInterpolateSize;
public:
    UParticleModuleSize();
    ~UParticleModuleSize() override = default;
    friend FArchive& operator<<(FArchive& Ar, UParticleModuleSize& M);
    virtual void Serialize(FArchive& Ar) override
    {
        // 1. 부모 클래스(UParticleModule)의 직렬화 호출
        Super::Serialize(Ar);

        // 2. StartSize 직렬화
        Ar << StartSize;

        // 3. EndSize 직렬화
        Ar << EndSize;

        // 4. bInterpolateSize 직렬화
        Ar << bInterpolateSize;
    }
};

