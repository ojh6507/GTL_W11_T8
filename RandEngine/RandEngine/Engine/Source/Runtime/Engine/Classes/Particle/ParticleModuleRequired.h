#pragma once
#include "ParticleModule.h"
#include "Container/String.h" 

class UParticleModuleRequired : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleRequired, UParticleModule);

public:
   
    // 이미터 지속 시간 (0이면 무한)
    float EmitterDuration;
    // 이미터 반복 횟수 (0이면 무한 반복, 1이면 한 번 실행 후 종료 등)
    int32 EmitterLoops;

     // ... 기타 필수 설정들 (예: SubUV 이미지 수, 보간 방식 등) ...
    int32 SubImages_Horizontal;
    int32 SubImages_Vertical;


    bool bKillOnDeactivate;

    bool bKillOnCompleted;

    bool bRequiresSorting;

    int32 SortMode; // 또는 EParticleSortMode SortMode;

    bool bIgnoreComponentScale; // 메시 이미터에만 주로 해당


    UParticleModuleRequired()
        : EmitterDuration(1.0f)
        , EmitterLoops(1)
        , SubImages_Horizontal(1)
        , SubImages_Vertical(1)
        , bKillOnDeactivate(false)
        , bKillOnCompleted(false)
        , bRequiresSorting(false)
        , SortMode(0) 
        , bIgnoreComponentScale(false)
    {
    }

    virtual EModuleType GetModuleType() const override { return EModuleType::Required; }

    friend FArchive& operator<<(FArchive& Ar, UParticleModuleRequired& M);

    virtual void Serialize(FArchive& Ar) override
    {

        Super::Serialize(Ar);

        // --- UParticleModuleRequired 고유 멤버 직렬화 ---
        Ar << EmitterDuration;
        Ar << EmitterLoops;
        Ar << SubImages_Horizontal;
        Ar << SubImages_Vertical;
        Ar << bKillOnDeactivate;
        Ar << bKillOnCompleted;
        Ar << bRequiresSorting;
        Ar << SortMode;
        Ar << bIgnoreComponentScale;

    }
};
