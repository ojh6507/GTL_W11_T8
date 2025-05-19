#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleModuleRequired.h"
#include "ParticleModuleSpawn.h"
#include "ParticleDefines.h"
#include "Runtime/Windows/SubWindow/ParticleSystemSubEngine.h"
#include "Runtime/Launch/EngineLoop.h"
#include "Engine/Engine.h"
#include "Uobject/Casts.h"

// UParticleEmitter 생성자
UParticleEmitter::UParticleEmitter()
    : EmitterName(TEXT("DefaultEmitter"))
    , bRequiresLoopNotification_Cached(false)
    , bMeshRotationActive_Cached(false)
    , ParticleSize_Cached(sizeof(FBaseParticle))
    , ReqInstanceBytes_Cached(0)
    , TypeDataOffset_Cached(0)
    , TypeDataInstanceOffset_Cached(-1)
    , EstimatedMaxActiveParticles_Cached(0)
{
    // 필요한 경우 멤버 변수들의 기본값 추가 설정
}

// CacheEmitterModuleInfo 
void UParticleEmitter::CacheEmitterModuleInfo()
{
    // --- 멤버 변수 초기화 ---
    bRequiresLoopNotification_Cached = false;
    bMeshRotationActive_Cached = false;

    ModuleOffsetMap_Cached.Empty();
    ModuleInstanceOffsetMap_Cached.Empty();
    ModulesNeedingInstanceData_Cached.Empty();

    ParticleSize_Cached = sizeof(FBaseParticle); // FBaseParticle 크기로 시작
    ReqInstanceBytes_Cached = 0;
    TypeDataOffset_Cached = 0; // TypeDataModule 페이로드가 없다면 0 유지
    TypeDataInstanceOffset_Cached = -1;

    // --- LOD 0 처리 ---
    if (LODLevels.IsEmpty())
    {
        UE_LOG(ELogLevel::Warning, TEXT("UParticleEmitter::CacheEmitterModuleInfo - LODLevels is empty."));
        return;
    }

    UParticleLODLevel* HighLODLevel = LODLevels[0];
    if (!HighLODLevel)
    {
        UE_LOG(ELogLevel::Warning, TEXT("UParticleEmitter::CacheEmitterModuleInfo - HighLODLevel is null."));
        return;
    }

    // --- TypeDataModule 처리 ---
    UParticleModuleTypeDataBase* HighTypeData = HighLODLevel->TypeDataModule;
    if (HighTypeData)
    {
        int32 ReqBytes = HighTypeData->RequiredBytes(nullptr); // TypeDataModule이 파티클당 필요로 하는 추가 바이트
        if (ReqBytes > 0)
        {
            TypeDataOffset_Cached = ParticleSize_Cached; // 현재까지의 ParticleSize가 TypeData의 시작 오프셋
            ParticleSize_Cached += ReqBytes;  // ParticleSize에 TypeData 크기 추가
        }
        int32 TempInstanceBytes = HighTypeData->RequiredBytesPerInstance();
        if (TempInstanceBytes > 0)
        {
            TypeDataInstanceOffset_Cached = ReqInstanceBytes_Cached; // 현재까지의 ReqInstanceBytes가 TypeData의 인스턴스 오프셋
            ReqInstanceBytes_Cached += TempInstanceBytes; // ReqInstanceBytes에 TypeData 인스턴스 크기 추가
        }
    }

    // --- RequiredModule 처리 ---
    UParticleModuleRequired* RequiredModule = HighLODLevel->RequiredModule;
    if (!RequiredModule)
    {
        UE_LOG(ELogLevel::Error, TEXT("UParticleEmitter::CacheEmitterModuleInfo - RequiredModule is null."));
        return;
    }

    // ScreenAlignment 관련 로직이 없으므로 bMeshRotationActive는 다른 요인으로 결정
    bMeshRotationActive_Cached = false; // 기본값

    // --- 일반 모듈들 순회 (LOD 0의 모듈들) ---
    for (int32 ModuleIdx = 0; ModuleIdx < HighLODLevel->Modules.Num(); ModuleIdx++)
    {
        UParticleModule* ParticleModule = HighLODLevel->Modules[ModuleIdx];
        if (!ParticleModule)
        {
            continue;
        }
        bRequiresLoopNotification_Cached |= (ParticleModule->bEnabled && ParticleModule->RequiresLoopingNotification());

        // TypeDataModule은 이미 위에서 처리했으므로, 일반 모듈만 고려
        if (ParticleModule->GetModuleType() != EModuleType::TypeDataBase)
        {
            int32 ReqBytes = ParticleModule->RequiredBytes(HighTypeData);
            if (ReqBytes > 0)
            {
                ModuleOffsetMap_Cached.Add(ParticleModule, ParticleSize_Cached);
                ParticleSize_Cached += ReqBytes;
            }

            int32 TempInstanceBytes = ParticleModule->RequiredBytesPerInstance();
            if (TempInstanceBytes > 0)
            {
                ModuleInstanceOffsetMap_Cached.Add(ParticleModule, ReqInstanceBytes_Cached);
                ModulesNeedingInstanceData_Cached.Add(ParticleModule);
                ReqInstanceBytes_Cached += TempInstanceBytes;
            }
        }

        if (!bMeshRotationActive_Cached && ParticleModule->TouchesMeshRotation())
        {
            bMeshRotationActive_Cached = true;
        }
    }
}


void UParticleEmitter::Build()
{
    if (LODLevels.IsEmpty())
    {
        UE_LOG(ELogLevel::Warning, TEXT("UParticleEmitter::Build - LODLevels is empty."));
        return;
    }
    UParticleLODLevel* HighLODLevel = LODLevels[0];
    if (!HighLODLevel)
    {
        UE_LOG(ELogLevel::Warning, TEXT("UParticleEmitter::Build - HighLODLevel is null."));
        EstimatedMaxActiveParticles_Cached = 0; // 안전을 위해 0으로 설정
        CacheEmitterModuleInfo();
        return;
    }


    FParticleEmitterBuildInfo EmitterBuildInfo;

    EmitterBuildInfo.bIsEditorBuild = true; // 항상 에디터 컨텍스트로 빌드 정보 준비
    EmitterBuildInfo.OwningLODLevel = HighLODLevel;
    EmitterBuildInfo.RequiredModule = HighLODLevel->RequiredModule;
    EmitterBuildInfo.CurrentTypeDataModule = HighLODLevel->TypeDataModule;
    EmitterBuildInfo.SpawnModule = nullptr;

    for (UParticleModule* Mod : HighLODLevel->Modules)
    {
        if (Mod && Mod->bEnabled) // 유효하고 활성화된 모듈만
        {
            if (UParticleModuleSpawn* SpawnMod = Cast<UParticleModuleSpawn>(Mod))
            {
                EmitterBuildInfo.SpawnModule = SpawnMod;
                break;
            }
        }
    }
    if (HighLODLevel->RequiredModule) // RequiredModule이 있어야 CompileModules 의미 있음
    {
        HighLODLevel->CompileModules(EmitterBuildInfo);
        EstimatedMaxActiveParticles_Cached = EmitterBuildInfo.EstimatedMaxActiveParticleCount;
    }
    else
    {
        // RequiredModule이 없다면 예상 파티클 수를 0으로 설정하거나 기본값 사용
        this->EstimatedMaxActiveParticles_Cached = 0;
        EmitterBuildInfo.EstimatedMaxActiveParticleCount = 0; // BuildInfo에도 반영
        UE_LOG(ELogLevel::Error, TEXT("UParticleEmitter '%s'::Build - RequiredModule is null in HighLODLevel. Cannot fully compile modules."), *EmitterName);
    }

    if (HighLODLevel->TypeDataModule != nullptr)
    {
        if (HighLODLevel->TypeDataModule->RequiresBuild())
        {
            HighLODLevel->TypeDataModule->Build(EmitterBuildInfo); // TypeData가 정보 사용
        }
         else
         {
             UE_LOG(ELogLevel::Warning, TEXT("UParticleEmitter '%s'::Build - TypeDataModule does not require build."), *EmitterName);
         }
        HighLODLevel->TypeDataModule->CacheModuleInfo(this);
    }
    else
    {
        UE_LOG(ELogLevel::Warning, TEXT("UParticleEmitter '%s'::Build - TypeDataModule is null."), *EmitterName);
    }

    // 모든 빌드 관련 작업 후 최종적으로 데이터 레이아웃 캐싱
    CacheEmitterModuleInfo();
    UE_LOG(ELogLevel::Display, TEXT("UParticleEmitter::Build completed for emitter."));
}

// GetHighestLODLevel (ParticleEmitter.h에 선언, .cpp에 구현)
UParticleLODLevel* UParticleEmitter::GetHighestLODLevel() const
{
    if (!LODLevels.IsEmpty())
    {
        return LODLevels[0];
    }
    return nullptr;
}

FArchive& operator<<(FArchive& Ar, UParticleEmitter& E)
{
    Ar << E.EmitterName;
    int32 NumLODLevels = E.LODLevels.Num();
    Ar << NumLODLevels;

    if (Ar.IsLoading())
    {
        E.LODLevels.Empty();
        E.LODLevels.Reserve(NumLODLevels);
        for (int32 i = 0; i < NumLODLevels; ++i)
        {
            UParticleLODLevel* NewLOD = nullptr;
            NewLOD = FObjectFactory::ConstructObject<UParticleLODLevel>(&E);
            if (NewLOD)
            {
                Ar << (*NewLOD);
                E.LODLevels.Add(NewLOD);
            }

        }
        FParticleEmitterBuildInfo EmitterBuildInfo; // Emitter 레벨에서 BuildInfo 생성 및 기본값 설정
        EmitterBuildInfo.bIsEditorBuild = true;
        for (UParticleLODLevel* LoadedLOD : E.LODLevels)
        {
            if (LoadedLOD)
            {
                FParticleEmitterBuildInfo LODBuildInfo = EmitterBuildInfo;
                LoadedLOD->CompileModules(LODBuildInfo);
            }
        }
    }
    else // Saving
    {
        for (UParticleLODLevel* LOD : E.LODLevels)
        {
            if (LOD)
            {
                Ar << (*LOD);
            }
        }
    }
    return Ar;
}
