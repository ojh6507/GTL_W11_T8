#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleModuleTypeDataBase.h"
#include "ParticleModuleRequired.h"
#include "ParticleDefines.h"

// UParticleEmitter 생성자
UParticleEmitter::UParticleEmitter()
    : bRequiresLoopNotification(false)
    , bMeshRotationActive(false)
    , DynamicParameterDataOffset(0)
    , ParticleSize(0)
    , ReqInstanceBytes(0)
    , TypeDataOffset(0)
    , TypeDataInstanceOffset(-1)
{
    // 필요한 경우 멤버 변수들의 기본값 추가 설정
}

// CacheEmitterModuleInfo 
void UParticleEmitter::CacheEmitterModuleInfo()
{
    // --- 멤버 변수 초기화 ---
    bRequiresLoopNotification = false;
    bMeshRotationActive = false;

    ModuleOffsetMap.Empty();
    ModuleInstanceOffsetMap.Empty();
    ModulesNeedingInstanceData.Empty();

    DynamicParameterDataOffset = 0;
    ParticleSize = sizeof(FBaseParticle);
    ReqInstanceBytes = 0;
    TypeDataOffset = 0;
    TypeDataInstanceOffset = -1;

    // --- LOD 0 처리 ---
    if (LODLevels.IsEmpty()) { /*UE_LOG(LogTemp, Warning, TEXT("UParticleEmitter::CacheEmitterModuleInfo - LODLevels is empty."));*/ return; }
    UParticleLODLevel* HighLODLevel = LODLevels[0];
    if (!HighLODLevel) { /*UE_LOG(LogTemp, Warning, TEXT("UParticleEmitter::CacheEmitterModuleInfo - HighLODLevel is null."));*/ return; }

    // --- TypeDataModule 처리 ---
    UParticleModuleTypeDataBase* HighTypeData = HighLODLevel->TypeDataModule;
    if (HighTypeData)
    {
        int32 ReqBytes = HighTypeData->RequiredBytes(nullptr);
        if (ReqBytes > 0)
        {
            TypeDataOffset = ParticleSize;
            ParticleSize += ReqBytes;
        }
        int32 TempInstanceBytes = HighTypeData->RequiredBytesPerInstance();
        if (TempInstanceBytes > 0)
        {
            TypeDataInstanceOffset = ReqInstanceBytes;
            ReqInstanceBytes += TempInstanceBytes;
        }
    }

    // --- RequiredModule 처리 ---
    UParticleModuleRequired* RequiredModule = HighLODLevel->RequiredModule;
    if (!RequiredModule) { /*UE_LOG(LogTemp, Error, TEXT("UParticleEmitter::CacheEmitterModuleInfo - RequiredModule is null."));*/ return; }

    // ScreenAlignment 관련 로직이 없으므로 bMeshRotationActive는 다른 요인으로 결정
    bMeshRotationActive = false; // 기본값

    // --- 일반 모듈들 순회 (LOD 0의 모듈들) ---
    for (int32 ModuleIdx = 0; ModuleIdx < HighLODLevel->Modules.Num(); ModuleIdx++)
    {
        UParticleModule* ParticleModule = HighLODLevel->Modules[ModuleIdx];
        if (!ParticleModule) continue;

        bRequiresLoopNotification |= (ParticleModule->bEnabled && ParticleModule->RequiresLoopingNotification());

        if (ParticleModule->GetModuleType() != EModuleType::TypeDataBase)
        {
            int32 ReqBytes = ParticleModule->RequiredBytes(HighTypeData);
            if (ReqBytes > 0)
            {
                ModuleOffsetMap.Add(ParticleModule, ParticleSize);
                // UParticleModuleParameterDynamic 지원 시 로직 (현재는 주석 처리)
                // if (ParticleModule->GetModuleType() == EModuleType::ParameterDynamic && (DynamicParameterDataOffset == 0))
                // {
                //     DynamicParameterDataOffset = ParticleSize;
                // }
                ParticleSize += ReqBytes;
            }

            int32 TempInstanceBytes = ParticleModule->RequiredBytesPerInstance();
            if (TempInstanceBytes > 0)
            {
                ModuleInstanceOffsetMap.Add(ParticleModule, ReqInstanceBytes);
                ModulesNeedingInstanceData.Add(ParticleModule);
                ReqInstanceBytes += TempInstanceBytes;
            }
        }

        if (!bMeshRotationActive && ParticleModule->TouchesMeshRotation())
        {
            bMeshRotationActive = true;
        }
    }
    // UE_LOG(LogTemp, Log, TEXT("UParticleEmitter::CacheEmitterModuleInfo completed. ParticleSize: %d, ReqInstanceBytes: %d"), ParticleSize, ReqInstanceBytes);
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
        return;
    }

    if (HighLODLevel->TypeDataModule != nullptr)
    {
        if (HighLODLevel->TypeDataModule->RequiresBuild())
        {
            FParticleEmitterBuildInfo EmitterBuildInfo;

            if (true) // 전역 플래그 사용
            {
                EmitterBuildInfo.bIsEditorBuild = true;
                HighLODLevel->CompileModules(EmitterBuildInfo); // LODLevel에서 정보 수집
            }
            else
            {
                EmitterBuildInfo.bIsEditorBuild = false;
            }

            HighLODLevel->TypeDataModule->Build(EmitterBuildInfo); // TypeData가 정보 사용
        }
        HighLODLevel->TypeDataModule->CacheModuleInfo(this);
    }

    // Build가 완료된 후, 최종적으로 데이터 레이아웃을 캐싱합니다.
    // TypeDataModule의 Build 과정에서 ParticleSize 등에 영향을 줄 수 있는
    // 정보가 변경되었을 수도 있기 때문입니다 (예: TypeData가 특정 페이로드를 추가하는 경우).
    // 하지만 일반적인 흐름은 CacheEmitterModuleInfo가 먼저 호출되어 ParticleSize의 기본틀을 잡고,
    // TypeDataModule::Build는 그 정보를 참고하는 것입니다.
    // 만약 TypeDataModule::Build가 ParticleSize에 영향을 준다면 순서 조정이 필요할 수 있습니다.
    // 언리얼 엔진에서는 CacheEmitterModuleInfo가 Build의 마지막 부분에 호출됩니다.
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
