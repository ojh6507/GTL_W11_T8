#pragma once
#include "ParticleModule.h"
#include "Container/String.h" 

enum EParticleSortMode : int
{
    PSORTMODE_None,
    PSORTMODE_ViewProjDepth,
    PSORTMODE_DistanceToView,
    PSORTMODE_Age_OldestFirst,
    PSORTMODE_Age_NewestFirst,
    PSORTMODE_MAX,
};

struct FParticleRequiredModule
{
    uint32 NumFrames;
    uint32 NumBoundingVertices;
    uint32 NumBoundingTriangles;
    float AlphaThreshold;
    TArray<FVector2D> FrameData;
    //FRHIShaderResourceView* BoundingGeometryBufferSRV;
    bool bCutoutTexureIsValid = false;
    bool bUseVelocityForMotionBlur = false;
};

class UParticleModuleRequired : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleRequired, UParticleModule);

public:
    UMaterial* Material; // 파티클에 적용할 머티리얼

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

    bool bUseLocalSpace; // 로컬 스페이스에서 파티클을 생성할지 여부

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
    virtual void Serialize(FArchive& Ar) override;
};
