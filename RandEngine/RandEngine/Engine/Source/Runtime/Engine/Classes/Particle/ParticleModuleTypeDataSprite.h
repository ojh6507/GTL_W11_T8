#pragma once
#pragma once
#include "ParticleModuleTypeDataBase.h"

class UParticleModuleTypeDataSprite : public UParticleModuleTypeDataBase
{
    DECLARE_CLASS(UParticleModuleTypeDataSprite, UParticleModuleTypeDataBase);
public:
    UParticleModuleTypeDataSprite() {}
    virtual EModuleType GetModuleType() const override { return EModuleType::TypeDataSprite; }
    // 스프라이트는 특별한 추가 데이터나 빌드 과정이 거의 필요 없을 수 있음
};
