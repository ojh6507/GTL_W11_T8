#pragma once
#include "ParticleModule.h"
class FArchive; // 전방 선언

// 파티클 생성 위치 결정 방식
enum class ELocationShape : uint8
{
    Point,  // 단일 지점
    Box,    // 박스 영역
    Sphere, // 구 영역
};

class UParticleModuleLocation : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleLocation, UParticleModule);
public:
    // 위치 결정 방식
    ELocationShape Shape;

    // Shape가 Point, Box, Sphere일 때 기준이 되는 시작 위치 (또는 박스/구의 중심)
    FVector StartLocation;

    // Shape가 Box일 때, StartLocation을 중심으로 각 축 방향으로의 절반 크기 (Extents)
    FVector BoxExtent;

    // Shape가 Sphere일 때, StartLocation을 중심으로 하는 구의 반지름
    float SphereRadius;
public:
    UParticleModuleLocation();
    ~UParticleModuleLocation() override = default;
    
    virtual void Serialize(FArchive& Ar) override;

    virtual void SpawnParticle(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle& ParticleBase) override;

    virtual void UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle, const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset, float DeltaTime) override;
    
    virtual EModuleType GetModuleType() const override { return EModuleType::Location; }

    friend FArchive& operator<<(FArchive& Ar, UParticleModuleLocation& M);
};

