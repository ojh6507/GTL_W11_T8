#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UParticleEmitter;

class UParticleSystem : public UObject
{
    DECLARE_CLASS(UParticleSystem, UObject);

public:
    TArray<UParticleEmitter*> Emitters; // 이 시스템에 포함된 이미터 목록

    // ... 기타 시스템 전체 설정 (예: 딜레이, 자동 비활성화, 고정 바운딩 박스 등) ...
    float Delay;

    bool bAutoActivate; // 컴포넌트에 의해 활성화될 때 자동으로 시작할지 여부
    bool bIsLooping_Computed = false;
    FString ParticleSystemFileName = "";
public:
    UParticleSystem();
    virtual ~UParticleSystem() = default;

    void InitializeSystem();
    
    void UpdateComputedFlags();

    // 시스템 내 모든 이미터 빌드
    void BuildEmitters();


    void PostLoad();
    void SaveParticleSystemToBinary();

    static UParticleSystem* LoadParticleSystemFromBinary(const FString& FilePath, UObject* OuterForSystem);
};
