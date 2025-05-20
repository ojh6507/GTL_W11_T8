#pragma once
#include "ParticleModule.h"
struct FLinearColor;
struct FArchive;
class UParticleModuleColor : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleColor, UParticleModule);

public:
    UParticleModuleColor();
    ~UParticleModuleColor() override = default;
public: 
    // 파티클의 시작 색상
    FLinearColor StartColor;

    // 파티클의 끝 색상
    FLinearColor EndColor;
    bool bInterpolateColor;
public:
    virtual EModuleType GetModuleType() const override { return EModuleType::Color; }
    virtual void UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle, const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset,  float DeltaTime) override;
    virtual void Serialize(FArchive& Ar) override;
    friend FArchive& operator<<(FArchive& Ar, UParticleModuleColor& M);
};
