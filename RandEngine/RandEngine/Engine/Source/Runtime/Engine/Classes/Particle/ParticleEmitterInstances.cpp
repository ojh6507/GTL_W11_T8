#include "ParticleEmitterInstances.h"
#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"    
#include "ParticleLODLevel.h"   
#include "ParticleModuleRequired.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleDefines.h"

void FParticleEmitterInstance::SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity, FParticleEventInstancePayload* EventPayload)
{
}

void FParticleEmitterInstance::KillParticle(int32 Index)
{
}

//임시로 ParticleEmitterInstance의 InitParameters()를 구현함
void FParticleEmitterInstance::InitParameters(UParticleEmitter* InEmitterTemplate, UParticleSystemComponent* InComponent)
{
    if (!InEmitterTemplate) 
    { 
        bEnabled = false;
        MaxActiveParticles = 0;
        return;
    }
    
    SpriteTemplate = InEmitterTemplate;
    Component = InComponent;

    CurrentLODLevelIndex = 0;
    CurrentLODLevel = InEmitterTemplate->GetHighestLODLevel();
    
    if (!CurrentLODLevel)
    {
        bEnabled = false;
        MaxActiveParticles = 0;
        return;
    }

    UParticleModuleRequired* ReqModule = CurrentLODLevel->RequiredModule;
    if (!ReqModule) 
    { 
        bEnabled = false; 
        MaxActiveParticles = 0;
        return;
    }

    // --- UParticleEmitter의 _Cached 멤버로부터 값 복사 ---
    ParticleSize = InEmitterTemplate->ParticleSize_Cached;
    ParticleStride = InEmitterTemplate->ParticleSize_Cached;
    InstancePayloadSize = InEmitterTemplate->ReqInstanceBytes_Cached;
    MaxActiveParticles = InEmitterTemplate->EstimatedMaxActiveParticles_Cached; // 이 값은 UPE::Build에서 설정됨

    TypeDataOffset = InEmitterTemplate->TypeDataOffset_Cached;
    TypeDataInstanceOffset = InEmitterTemplate->TypeDataInstanceOffset_Cached;

    // --- 지원하지 않는 모듈에 대한 오프셋은 UParticleEmitter에 해당 _Cached 멤버가 없음 ---
    // --- 따라서 FParticleEmitterInstance의 해당 멤버들은 생성자에서 설정된 기본값(0)을 유지 ---
    SubUVDataOffset = 0;
    DynamicParameterDataOffset = 0;

    // ... Light, Orbit, Camera 오프셋들도 마찬가지 ... 미지원..
    // PayloadOffset은 생성자에서 sizeof(FBaseParticle)로 초기화된 것을 사용.
    // 실제 모듈 데이터 접근은 SpriteTemplate->ModuleOffsetMap_Cached 를 직접 참조하여 수행.

    // --- UParticleModuleRequired (ReqModule) 에서 값 가져오기 ---
    bEnabled = CurrentLODLevel->bEnabled;
    bKillOnDeactivate = ReqModule->bKillOnDeactivate;
    bKillOnCompleted = ReqModule->bKillOnCompleted;
    bRequiresSorting = ReqModule->bRequiresSorting;
    SortMode = ReqModule->SortMode;
    bIgnoreComponentScale = ReqModule->bIgnoreComponentScale;

    // --- UParticleEmitter에서 직접 캐시된 플래그 가져오기 ---
    bRequiresLoopNotification = InEmitterTemplate->bRequiresLoopNotification_Cached;

    // = InEmitterTemplate->bAxisLockEnabled_Cached; // 미지원
    bAxisLockEnabled = false;

    // --- TypeDataModule 기반 정보 ---
    bIsBeam = false; // 미지원

    // --- 런타임 상태 변수 초기화 ---
    ActiveParticles = 0;
    ParticleCounter = 0;
    SpawnFraction = 0.0f;
    SecondsSinceCreation = 0.0f;
    EmitterTime = 0.0f;
    LastDeltaTime = 0.0f;

    bEmitterIsDone = false;
    bHaltSpawning = false;
    bHaltSpawningExternal = false;
    bFakeBurstsWhenSpawningSupressed = false;

    // --- 메모리 할당 ---
    delete[] ParticleData;       ParticleData = nullptr;
    delete[] ParticleIndices;    ParticleIndices = nullptr;
    delete[] InstanceData;       InstanceData = nullptr;

    if (MaxActiveParticles > 0 && ParticleStride > 0)
    {
        ParticleData = new uint8[MaxActiveParticles * ParticleStride];
        memset(ParticleData, 0, MaxActiveParticles * ParticleStride);

        ParticleIndices = new uint16[MaxActiveParticles];
        for (int32 i = 0; i < MaxActiveParticles; ++i) { ParticleIndices[i] = i; }
    }
    else
    {
        bEnabled = false;
    }

    if (InstancePayloadSize > 0)
    {
        InstanceData = new uint8[InstancePayloadSize];
        memset(InstanceData, 0, InstancePayloadSize);
    }

    // --- 위치 및 트랜스폼 ---
    if (Component)
    {
        Location = Component->GetRelativeLocation();
    }
    else
    {
        Location = FVector(0.f);
    }
}

FDynamicSpriteEmitterReplayDataBase::FDynamicSpriteEmitterReplayDataBase()
    : MaterialInterface(nullptr)
    , RequiredModule(nullptr)
    , NormalsSphereCenter(FVector::ZeroVector)
    , NormalsCylinderDirection(FVector::ZeroVector)
    , InvDeltaSeconds(0.0f)
    , MaxDrawCount(0)
    , OrbitModuleOffset(0)
    , DynamicParameterDataOffset(0)
    , LightDataOffset(0)
    , LightVolumetricScatteringIntensity(0)
    , CameraPayloadOffset(0)
    , SubUVDataOffset(0)
    , SubImages_Horizontal(1)
    , SubImages_Vertical(1)
    , bUseLocalSpace(false)
    , bLockAxis(false)
    , ScreenAlignment(0)
    , LockAxisFlag(0)
    , EmitterRenderMode(0)
    , EmitterNormalsMode(0)
    , PivotOffset(-0.5f, -0.5f, 0.0f)
    , bUseVelocityForMotionBlur(false)
    , bRemoveHMDRoll(false)
    , MinFacingCameraBlendDistance(0.f)
    , MaxFacingCameraBlendDistance(0.f)
{
}


FDynamicSpriteEmitterReplayDataBase::~FDynamicSpriteEmitterReplayDataBase()
{
    delete RequiredModule;
}

FDynamicEmitterDataBase::FDynamicEmitterDataBase(const UParticleModuleRequired* RequiredModule)
    : bSelected(false)
    , EmitterIndex(INDEX_NONE)
{
}

void FParticleDataContainer::Alloc(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts)
{
    if (InParticleDataNumBytes > 0 && ParticleIndicesNumShorts >= 0 // we assume that the particle storage has reasonable alignment below
        && InParticleDataNumBytes % sizeof(uint16) == 0)
    {
        ParticleDataNumBytes = InParticleDataNumBytes;
        ParticleIndicesNumShorts = InParticleIndicesNumShorts;

        MemBlockSize = ParticleDataNumBytes + ParticleIndicesNumShorts * sizeof(uint16);

        //ParticleData = (uint8*)FastParticleSmallBlockAlloc(MemBlockSize);
        ParticleIndices = (uint16*)(ParticleData + ParticleDataNumBytes);
    }
    else
    {
        UE_LOG(ELogLevel::Error, TEXT("FParticleDataContainer::Alloc - ParticleDataNumBytes is <= 0 or ParticleIndicesNumShorts is < 0"));
        return;
    }

}

void FParticleDataContainer::Free()
{
	if (ParticleData)
	{
        if (MemBlockSize <= 0)
        {
            UE_LOG(ELogLevel::Error, TEXT("FParticleDataContainer::Free - MemBlockSize is <= 0"));
            return;
        }
		//FastParticleSmallBlockFree(ParticleData, MemBlockSize);
	}
	MemBlockSize = 0;
	ParticleDataNumBytes = 0;
	ParticleIndicesNumShorts = 0;
	ParticleData = nullptr;
	ParticleIndices = nullptr;
}
