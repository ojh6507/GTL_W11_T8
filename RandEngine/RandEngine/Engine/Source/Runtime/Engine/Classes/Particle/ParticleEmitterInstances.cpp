#include "ParticleEmitterInstances.h"
#include "ParticleEmitter.h"
#include "ParticleSystemComponent.h"    
#include "ParticleLODLevel.h"   
#include "ParticleModuleRequired.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleDefines.h"

#include "UObject/Casts.h"
#include "ParticleModuleSpawn.h"
#include "ParticleModuleLifetime.h"


bool FParticleEmitterInstance::FillReplayData(FDynamicEmitterReplayDataBase& OutData)
{
    QUICK_SCOPE_CYCLE_COUNTER(STAT_ParticleEmitterInstance_FillReplayData);

    // NOTE: This the base class implementation that should ONLY be called by derived classes' FillReplayData()!

    // Make sure there is a template present
    if (!SpriteTemplate)
    {
        return false;
    }

    // Allocate it for now, but we will want to change this to do some form
    // of caching
    if (ActiveParticles <= 0 || !bEnabled)
    {
        return false;
    }
    // If the template is disabled, don't return data.
    UParticleLODLevel* LODLevel = CurrentLODLevel;
    if ((LODLevel == NULL) || (LODLevel->bEnabled == false))
    {
        return false;
    }

    // Make sure we will not be allocating enough memory
    if (MaxActiveParticles < ActiveParticles)
    {
        return false;
    }

    // Must be filled in by implementation in derived class
    OutData.eEmitterType = DET_Unknown;

    OutData.ActiveParticleCount = ActiveParticles;
    OutData.ParticleStride = ParticleStride;
    OutData.SortMode = SortMode;

    // Take scale into account
    OutData.Scale = FVector::OneVector;
    if (Component)
    {
        OutData.Scale = FVector(Component->GetRelativeScale3D());
    }

    int32 ParticleMemSize = MaxActiveParticles * ParticleStride;

    // Allocate particle memory

    OutData.DataContainer.Alloc(ParticleMemSize, MaxActiveParticles);

    FPlatformMemory::Memcpy(OutData.DataContainer.ParticleData, ParticleData, ParticleMemSize);
    FPlatformMemory::Memcpy(OutData.DataContainer.ParticleIndices, ParticleIndices, OutData.DataContainer.ParticleIndicesNumShorts * sizeof(uint16));

    // All particle emitter types derived from sprite emitters, so we can fill that data in here too!
    {
        FDynamicSpriteEmitterReplayDataBase* NewReplayData =
            static_cast<FDynamicSpriteEmitterReplayDataBase*>(&OutData);

        //NewReplayData->RequiredModule = LODLevel->RequiredModule->CreateRendererResource();
        NewReplayData->MaterialInterface = NULL;	// Must be set by derived implementation
        NewReplayData->InvDeltaSeconds = (LastDeltaTime > KINDA_SMALL_NUMBER) ? (1.0f / LastDeltaTime) : 0.0f;
        //NewReplayData->LWCTile = ((Component == nullptr) || LODLevel->RequiredModule->bUseLocalSpace) ? FVector::Zero() : Component->GetLWCTile();

        //NewReplayData->MaxDrawCount =
        //    (LODLevel->RequiredModule->bUseMaxDrawCount == true) ? LODLevel->RequiredModule->MaxDrawCount : -1;
        NewReplayData->MaxDrawCount = -1;
        /*NewReplayData->ScreenAlignment = LODLevel->RequiredModule->ScreenAlignment;*/
        NewReplayData->bUseLocalSpace = LODLevel->RequiredModule->bUseLocalSpace;
        /*NewReplayData->EmitterRenderMode = SpriteTemplate->EmitterRenderMode;*/
        NewReplayData->DynamicParameterDataOffset = DynamicParameterDataOffset;
        NewReplayData->LightDataOffset = LightDataOffset;
        NewReplayData->LightVolumetricScatteringIntensity = LightVolumetricScatteringIntensity;
        NewReplayData->CameraPayloadOffset = CameraPayloadOffset;

        NewReplayData->SubUVDataOffset = SubUVDataOffset;
        NewReplayData->SubImages_Horizontal = LODLevel->RequiredModule->SubImages_Horizontal;
        NewReplayData->SubImages_Vertical = LODLevel->RequiredModule->SubImages_Vertical;

        //NewReplayData->MacroUVOverride.bOverride = LODLevel->RequiredModule->bOverrideSystemMacroUV;
        //NewReplayData->MacroUVOverride.Radius = LODLevel->RequiredModule->MacroUVRadius;
        //NewReplayData->MacroUVOverride.Position = FVector3f(LODLevel->RequiredModule->MacroUVPosition);

        NewReplayData->bLockAxis = false;
        //if (bAxisLockEnabled == true)
        //{
        //    NewReplayData->LockAxisFlag = LockAxisFlags;
        //    if (LockAxisFlags != EPAL_NONE)
        //    {
        //        NewReplayData->bLockAxis = true;
        //    }
        //}

        // If there are orbit modules, add the orbit module data
        //if (LODLevel->OrbitModules.Num() > 0)
        //{
        //    UParticleLODLevel* HighestLODLevel = SpriteTemplate->LODLevels[0];
        //    UParticleModuleOrbit* LastOrbit = HighestLODLevel->OrbitModules[LODLevel->OrbitModules.Num() - 1];
        //    check(LastOrbit);

        //    uint32* LastOrbitOffset = SpriteTemplate->ModuleOffsetMap.Find(LastOrbit);
        //    NewReplayData->OrbitModuleOffset = *LastOrbitOffset;
        //}

        //NewReplayData->EmitterNormalsMode = LODLevel->RequiredModule->EmitterNormalsMode;
        //NewReplayData->NormalsSphereCenter = (FVector)LODLevel->RequiredModule->NormalsSphereCenter;
        //NewReplayData->NormalsCylinderDirection = (FVector)LODLevel->RequiredModule->NormalsCylinderDirection;

        NewReplayData->PivotOffset = FVector2D(PivotOffset);

        //NewReplayData->bUseVelocityForMotionBlur = LODLevel->RequiredModule->ShouldUseVelocityForMotionBlur();
        //NewReplayData->bRemoveHMDRoll = LODLevel->RequiredModule->bRemoveHMDRoll;
        //NewReplayData->MinFacingCameraBlendDistance = LODLevel->RequiredModule->MinFacingCameraBlendDistance;
        //NewReplayData->MaxFacingCameraBlendDistance = LODLevel->RequiredModule->MaxFacingCameraBlendDistance;
    }


    return true;
}


void FParticleEmitterInstance::SpawnParticles(int32 InCount, float InStartTime, float InIncrement, const FVector& InInitialLocation, const FVector& InInitialVelocity, FParticleEventInstancePayload* InEventPayload)
{
    if (!bEnabled || !CurrentLODLevel || !SpriteTemplate || InCount <= 0)
    {
        return;
    }

    int32 SpawnCount = 0; // 실제로 스폰된 파티클 수

    // 스폰 가능한 최대 파티클 수로 제한
    int32 MaxCanSpawn = MaxActiveParticles - ActiveParticles;
    int32 NumToSpawn = (InCount < MaxCanSpawn) ? InCount : MaxCanSpawn; // FMath::Min 대체

    if (NumToSpawn <= 0)
    {
        return;
    }

    // RequiredModule과 LifetimeModule은 자주 사용되므로 미리 찾아둡니다.
    UParticleModuleRequired* ReqModule = CurrentLODLevel->RequiredModule;
    UParticleModuleLifetime* LifetimeMod = nullptr;
    for (UParticleModule* Mod : CurrentLODLevel->Modules)
    {
        if (Mod && Mod->bEnabled)
        {
            LifetimeMod = Cast<UParticleModuleLifetime>(Mod);
            if (LifetimeMod) break;
        }
    }
    // 만약 RequiredModule에 기본 수명이 있다면, LifetimeMod가 null일 때 그것을 사용할 수 있습니다.
    // 여기서는 LifetimeMod가 필수라고 가정하거나, 없다면 기본 수명을 사용합니다.

    for (int32 i = 0; i < NumToSpawn; ++i)
    {
        if (ActiveParticles >= MaxActiveParticles) // 이중 체크
        {
            break;
        }

        // ParticleIndices 배열에서 다음 사용 가능한 (비활성) 파티클의 실제 데이터 인덱스를 가져옵니다.
        // KillParticle에서 swap-remove를 사용하므로, ActiveParticles 인덱스가 다음 빈 슬롯을 가리킵니다.
        const int32 CurrentParticleActualIndex = ParticleIndices[ActiveParticles];
        uint8* ParticleBasePtr = ParticleData + CurrentParticleActualIndex * ParticleStride;
        FBaseParticle& NewParticle = *((FBaseParticle*)ParticleBasePtr);

        // --- 1. FBaseParticle 기본값 설정 ---
        NewParticle.Location = InInitialLocation; // 전달받은 초기 위치 사용
        NewParticle.OldLocation = NewParticle.Location; // 스폰 시점에는 동일

        NewParticle.BaseVelocity = InInitialVelocity; // 전달받은 초기 속도 (모듈이 덮어쓸 수 있음)
        NewParticle.Velocity = InInitialVelocity;

        NewParticle.Rotation = 0.0f;
        NewParticle.BaseRotationRate = 0.0f;
        NewParticle.RotationRate = 0.0f;

        NewParticle.BaseSize = FVector(1.0f); // 기본 크기 (모듈이 덮어쓸 수 있음)
        NewParticle.Size = NewParticle.BaseSize;

        NewParticle.BaseColor = FLinearColor::White; // 기본 색상 (모듈이 덮어쓸 수 있음)
        NewParticle.Color = NewParticle.BaseColor;

        NewParticle.RelativeTime = 0.0f; // 생성 시점
        NewParticle.Flags = 0;           // 플래그 초기화

        // 최대 수명 설정 (Lifetime 모듈 또는 기본값)
        float MaxLifetimeForThisParticle = 1.0f; // 기본 수명 1초
        if (LifetimeMod)
        {
            MaxLifetimeForThisParticle = LifetimeMod->Lifetime.GetValue(InStartTime); // InStartTime은 현재 이미터 시간
        }
        else if (ReqModule) // Lifetime 모듈이 없을 경우 RequiredModule의 EmitterDuration을 사용할 수도 있음 (선택적 설계)
        {
            // MaxLifetimeForThisParticle = ReqModule->EmitterDuration; // 단, EmitterDuration이 0이면 무한이므로 주의
        }

        if (MaxLifetimeForThisParticle <= 0.0f)
        {
            NewParticle.OneOverMaxLifetime = FLT_MAX; // 즉시 소멸되도록
            NewParticle.RelativeTime = 1.0f;        // 즉시 소멸 상태
            NewParticle.Flags |= (uint32)EParticleFlags::Death;
        }
        else
        {
            NewParticle.OneOverMaxLifetime = 1.0f / MaxLifetimeForThisParticle;
        }

        // --- 2. 각 모듈의 SpawnParticle 함수 호출 ---
        // 모듈들은 FBaseParticle의 값을 수정하거나, 자신의 페이로드 데이터를 초기화합니다.

        // 2.1. RequiredModule의 SpawnParticle 호출
        if (ReqModule && ReqModule->bEnabled)
        {
            // RequiredModule은 일반적으로 파티클별 페이로드가 없다고 가정 (-1 전달)
            ReqModule->SpawnParticle(this, -1, InStartTime, NewParticle);
        }

        // 2.2. TypeDataModule의 SpawnParticle 호출
        UParticleModuleTypeDataBase* TypeDataMod = CurrentLODLevel->TypeDataModule;

        if (TypeDataMod && TypeDataMod->bEnabled)
        {
            TypeDataMod->SpawnParticle(this, this->TypeDataOffset, InStartTime, NewParticle);

        }

        // 2.3. 일반 모듈들의 SpawnParticle 호출
        for (UParticleModule* Mod : CurrentLODLevel->Modules)
        {
            if (Mod && Mod->bEnabled)
            {
                int32 RelativePayloadOffset = -1;
                int32* OffsetPtr = SpriteTemplate->ModuleOffsetMap_Cached.Find(Mod);
                if (OffsetPtr)
                {
                    RelativePayloadOffset = *OffsetPtr;
                }
                Mod->SpawnParticle(this, RelativePayloadOffset, InStartTime, NewParticle);
            }
        }

        // --- 3. 스폰된 파티클 활성화 ---
        ActiveParticles++;
        SpawnCount++;
    }
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
    bEnabled = true;

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
    ReqModule->bUseLocalSpace = true; // 로컬 스페이스 사용 여부 현재 일단 true

    // --- UParticleEmitter에서 직접 캐시된 플래그 가져오기 ---
    bRequiresLoopNotification = InEmitterTemplate->bRequiresLoopNotification_Cached;

    // = InEmitterTemplate->bAxisLockEnabled_Cached; // 미지원
    bAxisLockEnabled = false;

    // --- TypeDataModule 기반 정보 ---
    bIsBeam = false; // 미지원

    // --- 런타임 상태 변수 초기화 ---
    MaxActiveParticles = 5; // 임시로 5로 설정 추후 삭제
    ActiveParticles = 5; // 임시로 5로 설정 추후 0으로 변경
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

void FParticleEmitterInstance::Tick(float DeltaTime)
{
    FParticleEmitterInstance* Owner = this;

    if (!CurrentLODLevel || !CurrentLODLevel->RequiredModule || !SpriteTemplate)
    {
        if (bKillOnDeactivate && ActiveParticles > 0)
        {
            UE_LOG(ELogLevel::Warning, "  Tick: Killing %d particles due to bKillOnDeactivate.", ActiveParticles);

            ActiveParticles = 0; // 간단히 처리
        }
        bEmitterIsDone = true;
        return;
    }

    // 이전 프레임의 DeltaTime 저장
    LastDeltaTime = DeltaTime;
    // 이미터 전체 경과 시간 업데이트
    SecondsSinceCreation += DeltaTime;

    UParticleModuleRequired* ReqModule = CurrentLODLevel->RequiredModule;
    bool bIsLoopingEmitter = (ReqModule->EmitterLoops == 0);
    bool bIsFiniteDuration = (ReqModule->EmitterDuration > 0.0f);

    // --- 1. 이미터 시간 (EmitterTime) 업데이트 ---
    if (!bEmitterIsDone) // 이미터가 아직 완료되지 않았다면 시간 업데이트
    {
        float OldEmitterTime = EmitterTime;
        EmitterTime += DeltaTime;
        
        //UE_LOG(ELogLevel::Warning, "  EmitterTime updated: %.4f -> %.4f", OldEmitterTime, EmitterTime);


        if (bIsFiniteDuration && EmitterTime >= ReqModule->EmitterDuration)
        {
            if (bIsLoopingEmitter || (LoopCount + 1 < ReqModule->EmitterLoops))
            {
                LoopCount++;
                EmitterTime -= ReqModule->EmitterDuration; // 루프 시작 시간으로 리셋
                UE_LOG(ELogLevel::Display, "  Emitter '%s' looped. LoopCount: %d, New EmitterTime: %.4f", *(SpriteTemplate->EmitterName), LoopCount, EmitterTime);
                if (bRequiresLoopNotification)
                {
                    // for (UParticleModule* Mod : CurrentLODLevel->Modules) {
                    //     if (Mod && Mod->bEnabled && Mod->RequiresLoopingNotification()) {
                    //         // Mod->OnEmitterLoop(this); // 가상의 함수
                    //     }
                    // }
                }
            }
            else
            {
                // 모든 루프 완료
                bEmitterIsDone = true; // 이미터 완료 플래그 설정
                // bKillOnCompleted 옵션은 아래 파티클 업데이트 후 처리
            }
        }
    }

    // --- 2. 파티클 스폰 (Spawn) ---
    if (!bEmitterIsDone && !bHaltSpawning && !bHaltSpawningExternal && ActiveParticles < MaxActiveParticles)
    {
        UParticleModuleSpawn* SpawnModule = nullptr;
        for (UParticleModule* Mod : CurrentLODLevel->Modules)
        {
            if (Mod && Mod->bEnabled)
            {
                SpawnModule = Cast<UParticleModuleSpawn>(Mod);
                if (SpawnModule) break;
            }
        }

        if (SpawnModule)
        {
            // 2.1. 지속 스폰 (Rate)
            if (SpawnModule->bProcessSpawnRate)
            {
                float CurrentRate = SpawnModule->GetEffectiveSpawnRate(EmitterTime);
                float ParticlesToSpawnExact = CurrentRate * DeltaTime + SpawnFraction;
                int32 NumToSpawnThisTick = FMath::FloorToInt(ParticlesToSpawnExact);
                SpawnFraction = ParticlesToSpawnExact - NumToSpawnThisTick;
                
                if (NumToSpawnThisTick > 0) UE_LOG(ELogLevel::Warning, "  Spawn: NumFromRate = %d (Rate: %.2f)", NumToSpawnThisTick, CurrentRate);

                if (NumToSpawnThisTick > 0)
                {
                    int32 ActualSpawnCount = FMath::Min(NumToSpawnThisTick, MaxActiveParticles - ActiveParticles);
                    if (ActualSpawnCount > 0)
                    {
                        SpawnParticles(ActualSpawnCount, EmitterTime, 0.0f /*Increment*/, Location /*InitialLocation*/, FVector::ZeroVector /*InitialVelocity*/, nullptr /*EventPayload*/);
                        ParticleCounter += ActualSpawnCount;
                    }
                }
            }

            // 2.2. 버스트 스폰 (Burst)
            if (SpawnModule->bProcessBurstList)
            {
                int32 NumBurstParticles = SpawnModule->GetBurstAmountForThisTick(EmitterTime, DeltaTime);
                if (NumBurstParticles > 0)
                {
                    int32 ActualSpawnCount = FMath::Min(NumBurstParticles, MaxActiveParticles - ActiveParticles);
                    if (ActualSpawnCount > 0)
                    {
                        SpawnParticles(ActualSpawnCount, EmitterTime, 0.0f, Location, FVector::ZeroVector, nullptr);
                        ParticleCounter += ActualSpawnCount;
                    }
                }
            }
        }
    }
    else if (bEmitterIsDone && bFakeBurstsWhenSpawningSupressed && !bHaltSpawningExternal)
    {
        // 이미터는 끝났지만, 스폰 억제 시 페이크 버스트 처리 (시간만 흐르게)

    }


    // --- 3. 파티클 업데이트 ---
    if (ActiveParticles > 0)
    {
        // Offset 파라미터는 이 Update 함수가 호출될 때 모듈의 인스턴스 데이터에 대한 오프셋을 의미할 수 있습니다.
        // BEGIN_UPDATE_LOOP 매크로는 Owner(FParticleEmitterInstance*)를 사용하므로,
        // 이 함수의 Offset 파라미터는 매크로에 직접 전달되지 않습니다.
        // 만약 모듈의 Update 함수가 자신의 인스턴스 데이터에 접근해야 한다면,
        // 그 데이터의 시작 주소는 Owner->InstanceData + ModuleSpecificOffset 형태로 얻어야 합니다.
        // (여기서는 ModuleSpecificOffset이 함수 파라미터 Offset이라고 가정)

        BEGIN_UPDATE_LOOP;
        // Owner는 현재 FParticleEmitterInstance (this)
        // 이 루프 내에서 'Particle' (FBaseParticle&) 변수 사용 가능
        // 'ParticleIndex' (int32) 사용 가능 (ParticleIndices 배열의 인덱스)
        // 'CurrentIndex' (int32) 사용 가능 (ParticleData 배열의 실제 인덱스)
        // 'ActiveParticles' (int32&) 사용 및 수정 가능 (KILL_CURRENT_PARTICLE에서 사용)

        // 3.1. 파티클 수명 및 상대 시간 업데이트
        float OldRelativeTime = Particle.RelativeTime;
        Particle.RelativeTime += DeltaTime * Particle.OneOverMaxLifetime;

        
        if (Particle.RelativeTime >= 1.0f)
        {

            // 파티클 수명 종료
            Particle.Flags |= (uint32)EParticleFlags::Death; // 죽음 플래그 설정
            UE_LOG(ELogLevel::Error, "    Particle %d (Idx:%d) KILLED (Lifetime %.2f -> %.2f)", i, CurrentIndex, OldRelativeTime, Particle.RelativeTime);
            KILL_CURRENT_PARTICLE; // 파티클 제거 (ActiveParticles 감소)
            CONTINUE_UPDATE_LOOP;  // 다음 파티클로 (역순 루프이므로 안전)
        }

        uint32 CurrentModulePayloadOffset = 0; // FBaseParticle 이후의 상대 오프셋
        for (UParticleModule* Mod : CurrentLODLevel->Modules)
        {
            if (Mod && Mod->bEnabled)
            {
                int32 RelativePayloadOffset = -1; // 기본값: 페이로드 없음
                if (SpriteTemplate) // UParticleEmitter* SpriteTemplate
                {
                    int32* OffsetPtr = SpriteTemplate->ModuleOffsetMap_Cached.Find(Mod);
                    if (OffsetPtr)
                    {
                        RelativePayloadOffset = *OffsetPtr;
                    }
                }
                // ParticleBase은 BEGIN_UPDATE_LOOP 매크로 내부 변수
                Mod->UpdateParticle(this, Particle, ParticleBase, RelativePayloadOffset, DeltaTime);
            }
        }

        // 3.3. 파티클 물리/이동 업데이트 (기본 로직)
        Particle.OldLocation = Particle.Location;
        Particle.Location += Particle.Velocity * DeltaTime;
        // (필요시 중력, 저항 등 다른 물리 효과 적용)

        END_UPDATE_LOOP;
    }

    // --- 4. 이미터 완료 및 정리 ---
    if (bEmitterIsDone && ActiveParticles == 0)
    {
        // 이미터가 모든 루프를 마치고 모든 파티클이 소멸됨
        // (여기서 특별한 정리 작업이 필요하다면 수행)
        // UE_LOG(LogTemp, Log, TEXT("Emitter '%s' has completed and all particles are dead."), *SpriteTemplate->EmitterName);
    }
    else if (bEmitterIsDone && bKillOnCompleted && ActiveParticles > 0)
    {
        // 이미터는 완료되었지만, bKillOnCompleted가 true이고 아직 파티클이 남아있다면 모두 제거
        // UE_LOG(LogTemp, Log, TEXT("Emitter '%s' completed, killing remaining %d particles due to bKillOnCompleted."), *SpriteTemplate->EmitterName, ActiveParticles);
        ActiveParticles = 0; // 간단히 처리
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
    , PivotOffset(-0.5f, -0.5f)
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

        ParticleData = (uint8*)(FPlatformMemory::Malloc<EAllocationType::EAT_Container>(MemBlockSize));
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
        FPlatformMemory::Free<EAllocationType::EAT_Container>(ParticleData, MemBlockSize);
    }
    MemBlockSize = 0;
    ParticleDataNumBytes = 0;
    ParticleIndicesNumShorts = 0;
    ParticleData = nullptr;
    ParticleIndices = nullptr;
}
