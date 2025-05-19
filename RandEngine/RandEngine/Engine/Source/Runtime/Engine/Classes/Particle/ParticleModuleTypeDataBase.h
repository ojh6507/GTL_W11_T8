#pragma once
#include "ParticleModule.h" 

class FParticleEmitterBuildInfo;
class UParticleEmitter;

class UParticleModuleTypeDataBase : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleTypeDataBase, UParticleModule);
public:
    UParticleModuleTypeDataBase() {}
    
    virtual EModuleType GetModuleType() const override { return EModuleType::TypeDataBase; }

    // 이 TypeData를 사용하는 파티클이 FBaseParticle 외에 추가로 필요한 바이트 수
    // UParticleModule의 RequiredBytes를 오버라이드하여 더 구체적인 로직 제공 가능
    virtual int32 RequiredBytes(UParticleModuleTypeDataBase* SpawningTypeData) const override { return 0; }
    // 이미터 인스턴스당 필요한 추가 바이트 수
    virtual int32 RequiredBytesPerInstance() const override { return 0; }

    // 이 TypeDataModule이 빌드를 필요로 하는지 여부
    virtual bool RequiresBuild() const override { return false; }
    // 실제 빌드 로직
    virtual void Build(const FParticleEmitterBuildInfo& EmitterBuildInfo) override {}
    // 이미터 정보 캐싱
    virtual void CacheModuleInfo(UParticleEmitter* Emitter) override {}
};
