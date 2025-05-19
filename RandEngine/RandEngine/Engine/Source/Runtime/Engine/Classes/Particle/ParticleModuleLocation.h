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
    virtual void Serialize(FArchive& Ar)
    {
        // 2. Shape (enum ELocationShape) 직렬화
        if (Ar.IsLoading())
        {
            uint8 ShapeInt;
            Ar << ShapeInt;
            Shape = static_cast<ELocationShape>(ShapeInt);
        }
        else // Saving
        {
            uint8 ShapeInt = static_cast<uint8>(Shape);
            Ar << ShapeInt;
        }

        // 3. StartLocation 직렬화
        Ar << StartLocation; // FVector에 대한 operator<< 가정

        // 4. Shape에 따라 필요한 추가 데이터 직렬화
        switch (Shape)
        {
        case ELocationShape::Point:
            // Point는 StartLocation만 사용하므로 추가 데이터 없음
            break;
        case ELocationShape::Box:
            Ar << BoxExtent; // FVector에 대한 operator<< 가정
            break;
        case ELocationShape::Sphere:
            Ar << SphereRadius; // float
            break;
        default:
            break;
        }

    }
    friend FArchive& operator<<(FArchive& Ar, UParticleModuleLocation& M);
};

