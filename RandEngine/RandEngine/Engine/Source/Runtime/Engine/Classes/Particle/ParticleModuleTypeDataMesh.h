#pragma once
#include "ParticleModuleTypeDataBase.h"
#include "Container/String.h"

struct FMeshParticlePayload
{
    float Scale;
    // FVector RotationAxis;
    // float RotationAngle;
    // int32 MaterialIndex; // 머티리얼 오버라이드용
};
class UStaticMesh; // 전방 선언

class UParticleModuleTypeDataMesh : public UParticleModuleTypeDataBase
{
    DECLARE_CLASS(UParticleModuleTypeDataMesh, UParticleModuleTypeDataBase);
public:
    UStaticMesh* Mesh; // 사용할 스태틱 메시

    FVector MeshScale; // 메시의 기본 스케일

    FString MeshAssetPath;

    UParticleModuleTypeDataMesh() : Mesh(nullptr), MeshScale(1.0f, 1.0f, 1.0f) {}
    virtual EModuleType GetModuleType() const override { return EModuleType::TypeDataMesh; }

    virtual int32 RequiredBytes(UParticleModuleTypeDataBase* SpawningTypeData) const override
    {
        return sizeof(FMeshParticlePayload);
    }
    virtual void UpdateParticle(FParticleEmitterInstance* Owner, FBaseParticle& Particle, const uint8* ParticleBaseForPayload, int32 PayloadRelativeOffset, float DeltaTime) override;

    virtual bool RequiresBuild() const override
    {
        return true;
    }
    virtual void Serialize(FArchive& Ar) override;
    virtual void Build(const FParticleEmitterBuildInfo& EmitterBuildInfo) override;
    virtual void SpawnParticle(FParticleEmitterInstance* Owner, int32 PayloadRelativeOffset, float SpawnTime, FBaseParticle& Particle) override;
    friend FArchive& operator<<(FArchive& Ar, UParticleModuleTypeDataMesh& M);
};
