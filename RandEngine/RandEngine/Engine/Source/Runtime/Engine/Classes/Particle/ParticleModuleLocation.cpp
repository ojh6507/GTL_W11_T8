#include "ParticleModuleLocation.h"

UParticleModuleLocation::UParticleModuleLocation()
    : Shape(ELocationShape::Point)
    , StartLocation(0.0f, 0.0f, 0.0f)
    , BoxExtent(50.0f, 50.0f, 50.0f)
    , SphereRadius(50.0f)
{
}
void UParticleModuleLocation::SpawnParticle(FParticleEmitterInstance* Owner, int32 Offset,
    float SpawnTime, FBaseParticle& Particle)
{
    switch (Shape)
    {
    case ELocationShape::Point:
        Particle.Location = StartLocation;
        break;

    case ELocationShape::Box:
    {
        FVector RandomOffset;
        RandomOffset.X = FMath::FRandRange(-BoxExtent.X, BoxExtent.X);
        RandomOffset.Y = FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y);
        RandomOffset.Z = FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z);
        Particle.Location = StartLocation + RandomOffset;
        break;
    }

    case ELocationShape::Sphere:
    {
        FVector RandomDirection = FMath::VRand(); // 단위 구 표면 벡터
        
        float RandomDistFactor = FMath::Pow(FMath::FRand(), 1.0f / 3.0f); // 0.0 ~ 1.0
        
        float RandomRadiusInSphere = SphereRadius * RandomDistFactor;

        Particle.Location = StartLocation + RandomDirection * RandomRadiusInSphere;
     
        break;
    }
    default:
        Particle.Location = StartLocation; // 알 수 없는 Shape면 기본 위치
        break;
    }
}
void UParticleModuleLocation::UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle,
    const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset, float DeltaTime)
{

}

void UParticleModuleLocation::Serialize(FArchive& Ar)
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
    Ar << StartLocation;

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


FArchive& operator<<(FArchive& Ar, UParticleModuleLocation& M)
{
    M.Serialize(Ar);
    return Ar;
}
