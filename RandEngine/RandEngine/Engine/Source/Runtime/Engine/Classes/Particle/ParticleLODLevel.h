#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

// 전방 선언
class UParticleModuleRequired;
class UParticleModule;
class UParticleModuleTypeDataBase;
struct FParticleEmitterBuildInfo; // 컴파일 함수용

class UParticleLODLevel : public UObject
{
    DECLARE_CLASS(UParticleLODLevel, UObject);

public:
 
    bool bEnabled; // 이 LOD 레벨이 활성화되었는지 여부

    UParticleModuleRequired* RequiredModule;

    TArray<UParticleModule*> Modules;

    UParticleModuleTypeDataBase* TypeDataModule;

    UParticleLODLevel() : bEnabled(true), RequiredModule(nullptr), TypeDataModule(nullptr) {}
    
    int32 CalculateMaxActiveParticleCount();
   
    // 에디터 기능: 빌드 정보 수집
    void CompileModules(FParticleEmitterBuildInfo& EmitterBuildInfo);
};
