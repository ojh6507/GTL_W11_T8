#include "ParticleLODLevel.h"
#include "ParticleEmitter.h"
#include "ParticleModule.h"
#include "UObject/Casts.h"

#include "ParticleModuleSpawn.h"
#include "ParticleModuleLifetime.h"
#include "ParticleModuleRequired.h"
#include "ParticleModuleTypeDataBase.h"

#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"
#include "ParticleModuleTypeDataMesh.h"
#include "ParticleModuleTypeDataSprite.h"
int32 UParticleLODLevel::CalculateMaxActiveParticleCount()
{
    if (!RequiredModule)
    {
        return 0;
    }

    float ParticleMaxLifetime = 0.0f;
    float EffectiveSpawnRate = 0.0f;
    int32 TotalBurstAmountFromSpawnModule = 0;

    // Modules 배열에서 Spawn 및 Lifetime 모듈 찾기
    UParticleModuleSpawn* SpawnModule = nullptr;
    UParticleModuleLifetime* LifetimeModule = nullptr;

    for (UParticleModule* mod : Modules)
    {
        if (mod && mod->bEnabled) // 유효하고 활성화된 모듈만 고려
        {
            if (!SpawnModule) // 아직 스폰 모듈 못 찾음
            {
                SpawnModule = Cast<UParticleModuleSpawn>(mod);
            }
            if (!LifetimeModule) // 아직 라이프타임 모듈 못 찾음
            {
                LifetimeModule = Cast<UParticleModuleLifetime>(mod);
            }
            if (SpawnModule && LifetimeModule) break; // 둘 다 찾았으면 종료
        }
    }

    // 스폰 모듈에서 정보 가져오기
    if (SpawnModule)
    {
        EffectiveSpawnRate = SpawnModule->GetEffectiveSpawnRate(0.0f); // 이미터 시간 0에서의 스폰율 (또는 분포의 최대/평균)
        TotalBurstAmountFromSpawnModule = SpawnModule->GetTotalBurstAmountSimple();
    }
    else
    {
        UE_LOG(ELogLevel::Warning, TEXT("CalculateMaxActiveParticleCount: SpawnModule not found or disabled."));
    }


    // 라이프타임 모듈에서 정보 가져오기
    if (LifetimeModule) // Cast 성공 및 활성화는 위에서 체크됨
    {
        ParticleMaxLifetime = LifetimeModule->GetMaxLifetime();
    }
    else
    {
        ParticleMaxLifetime = 0.0f;
    }

    // --- 실제 계산 로직 ---

    // 수명이 0이거나 음수이고 버스트도 없으면 활성 파티클 없음
    if (ParticleMaxLifetime <= 0.0f && TotalBurstAmountFromSpawnModule == 0)
    {
        return 0;
    }
    // 수명이 0이거나 음수이지만 버스트가 있다면, 버스트 파티클만 존재
    if (ParticleMaxLifetime <= 0.0f && TotalBurstAmountFromSpawnModule > 0)
    {
        return TotalBurstAmountFromSpawnModule;
    }
    // 스폰율이 0이고 버스트만 있다면, 버스트 파티클만 존재
    if (EffectiveSpawnRate <= 0.0f && TotalBurstAmountFromSpawnModule > 0)
    {
        return TotalBurstAmountFromSpawnModule;
    }
    // 스폰율도 없고 버스트도 없으면 활성 파티클 없음
    if (EffectiveSpawnRate <= 0.0f && TotalBurstAmountFromSpawnModule == 0)
    {
        return 0;
    }


    float emitterDuration = RequiredModule->EmitterDuration;
    int32 emitterLoops = RequiredModule->EmitterLoops;
    int32 calculatedMaxActiveParticles = 0;

    // 1. 지속 스폰으로 인한 최대 파티클 수
    if (EffectiveSpawnRate > 0.0f)
    {
        bool bInfiniteDurationOrLoops = (emitterLoops == 0 || emitterDuration <= 0.0f);
        if (bInfiniteDurationOrLoops)
        {
            calculatedMaxActiveParticles = FMath::CeilToInt(EffectiveSpawnRate * ParticleMaxLifetime);
        }
        else // 유한 루프 및 유효한 지속 시간
        {
            float totalPlayTime = emitterDuration * emitterLoops;
            if (ParticleMaxLifetime >= totalPlayTime)
            {
                calculatedMaxActiveParticles = FMath::CeilToInt(EffectiveSpawnRate * totalPlayTime);
            }
            else
            {
                calculatedMaxActiveParticles = FMath::CeilToInt(EffectiveSpawnRate * ParticleMaxLifetime);
            }
        }
    }

    // 2. 버스트로 인한 파티클 수 추가
    //    - 가장 단순한 방법: 모든 버스트 파티클이 한 번에 추가된다고 가정.
    //    - 더 정확하게 하려면: 이미터 루프와 파티클 수명을 고려하여 버스트가 중첩될 수 있는지 판단.
    //      (예: 루프 N번, 각 루프 시작 시 버스트, 파티클 수명이 루프 길이보다 짧으면 중첩 가능)
    //      여기서는 단순 합산.
    calculatedMaxActiveParticles += TotalBurstAmountFromSpawnModule;

    // 3. 최종 값 보정
    if (calculatedMaxActiveParticles > 0)
    {
        return FMath::Max(1, calculatedMaxActiveParticles); // 최소 1개 보장
    }

    return 0; // 모든 조건에 해당하지 않으면 0
}

void UParticleLODLevel::CompileModules(FParticleEmitterBuildInfo& OutEmitterBuildInfo) // 파라미터 이름을 Out으로 변경하여 의도 명확화
{
    // 0. 방어 코드
    if (!RequiredModule)
    {
        OutEmitterBuildInfo.EstimatedMaxActiveParticleCount = 0;
        return;
    }

    // 1. FParticleEmitterBuildInfo의 기본 참조 설정
    OutEmitterBuildInfo.OwningLODLevel = this;
    OutEmitterBuildInfo.RequiredModule = this->RequiredModule;
    OutEmitterBuildInfo.CurrentTypeDataModule = this->TypeDataModule; // TypeDataModule이 null일 수도 있음

    // 2. SpawnModule 찾아서 BuildInfo에 설정 (Modules 배열 내에서)
    OutEmitterBuildInfo.SpawnModule = nullptr; // 초기화
    for (UParticleModule* Mod : this->Modules)
    {
        if (Mod && Mod->bEnabled) // 유효하고 활성화된 모듈만
        {
            if (UParticleModuleSpawn* SpawnMod = Cast<UParticleModuleSpawn>(Mod))
            {
                OutEmitterBuildInfo.SpawnModule = SpawnMod;
                break;
            }
        }
    }

    // 3. 예상 최대 활성 파티클 수 계산 및 설정
    OutEmitterBuildInfo.EstimatedMaxActiveParticleCount = CalculateMaxActiveParticleCount();

    // 4. 각 중요 모듈 및 일반 모듈의 CompileModule 함수 호출
    //    호출 순서는 모듈 간 의존성에 따라 중요할 수 있습니다.
    //    일반적으로 Required -> Spawn -> 일반 Modules -> TypeData 순서 또는
    //    Required -> 일반 Modules (Spawn 포함) -> TypeData 순서가 될 수 있습니다.
    //    여기서는 Required -> Modules (Spawn 포함) -> TypeData 순서로 가정합니다.

    // 4.1. RequiredModule 컴파일
    //      (RequiredModule은 항상 존재하고 활성화되어 있다고 가정, 또는 여기서 bEnabled 체크)
    this->RequiredModule->CompileModule(OutEmitterBuildInfo);

    // 4.2. Modules 배열에 있는 모든 활성화된 일반 모듈들 컴파일
    //      (여기에 SpawnModule도 포함되어 한 번만 호출됨)
    for (UParticleModule* Mod : this->Modules)
    {
        if (Mod && Mod->bEnabled)
        {
            // TypeDataModule은 Modules 배열에 없다고 가정하고, 별도로 처리합니다.
            Mod->CompileModule(OutEmitterBuildInfo);
        }
    }

    // 4.3. TypeDataModule 컴파일 (존재하고 활성화된 경우)
    //      다른 모든 모듈의 정보가 BuildInfo에 어느 정도 채워진 후 호출될 수 있습니다.
    if (this->TypeDataModule && this->TypeDataModule->bEnabled)
    {
        this->TypeDataModule->CompileModule(OutEmitterBuildInfo);
    }
}
static UParticleModule* CreateModuleByClassName(const FString& ClassName, UObject* Outer)
{
    // 참고: 실제 엔진에서는 FClassFinder나 UClass::TryFindTypeSlow 등을 사용할 수 있습니다.
    // 여기서는 간단한 if-else 체인으로 예시를 듭니다.
    // 실제로는 모든 가능한 모듈 타입을 여기에 등록해야 합니다.

    if (ClassName == TEXT("UParticleModuleRequired")) return FObjectFactory::ConstructObject<UParticleModuleRequired>(Outer);
    if (ClassName == TEXT("UParticleModuleSpawn")) return FObjectFactory::ConstructObject<UParticleModuleSpawn>(Outer); // 예시
    if (ClassName == TEXT("UParticleModuleLifetime")) return FObjectFactory::ConstructObject<UParticleModuleLifetime>(Outer); // 예시
    // ... 기타 모든 UParticleModule 파생 클래스들 ...
    if (ClassName == TEXT("UParticleModuleTypeDataSprite")) return FObjectFactory::ConstructObject<UParticleModuleTypeDataSprite>(Outer); // 예시
    if (ClassName == TEXT("UParticleModuleTypeDataMesh")) return FObjectFactory::ConstructObject<UParticleModuleTypeDataMesh>(Outer); // 예시
    // ... 기타 모든 UParticleModuleTypeDataBase 파생 클래스들 ...

    // UE_LOG(LogSerialization, Error, TEXT("Unknown module class name for deserialization: %s"), *ClassName);
    return nullptr;
}

FArchive& operator<<(FArchive& Ar, UParticleLODLevel& LOD)
{

    if (Ar.IsLoading())
    {
        uint8 bEnable = (LOD.bEnabled) ? 1 : 0;
        Ar << bEnable;


        uint8 bHasRequiredModule = 0;
        Ar << bHasRequiredModule;
        if (bHasRequiredModule)
        {
            LOD.RequiredModule = FObjectFactory::ConstructObject<UParticleModuleRequired>(&LOD);
            if (LOD.RequiredModule)
            {
                Ar << (*LOD.RequiredModule);
            }
        }
        else
        {
            LOD.RequiredModule = nullptr;
        }
    }
    else // Saving
    {
        uint8 bEnable = (LOD.bEnabled) ? 1 : 0;
        Ar << bEnable;


        uint8 bHasRequiredModule = (LOD.RequiredModule ? 1 : 0);
        Ar << bHasRequiredModule;
        if (bHasRequiredModule)
        {
            Ar << (*LOD.RequiredModule);
        }
    }

    // --- 3. TypeDataModule 직렬화 (다형성 처리) ---
    if (Ar.IsLoading())
    {
        uint8 bHasTypeDataModule = false;
        Ar << bHasTypeDataModule;
        if (bHasTypeDataModule)
        {
            FString TypeDataClassName;
            Ar << TypeDataClassName; // 타입 이름 읽기
            LOD.TypeDataModule = static_cast<UParticleModuleTypeDataBase*>(CreateModuleByClassName(TypeDataClassName, &LOD));

            if (LOD.TypeDataModule)
            {
                Ar << (*LOD.TypeDataModule); // 해당 타입의 operator<< 호출
            }

        }
        else
        {
            LOD.TypeDataModule = nullptr;
        }
    }
    else // Saving
    {
        uint8 bHasTypeDataModule = (LOD.TypeDataModule ? 1 : 0);
        Ar << bHasTypeDataModule;
        if (bHasTypeDataModule)
        {
            FString TypeDataClassName = LOD.TypeDataModule->GetClass()->GetName();

            Ar << TypeDataClassName;
            Ar << (*LOD.TypeDataModule);
        }
    }

    if (Ar.IsLoading())
    {

        int32 NumModules = 0;
        Ar << NumModules;

        LOD.Modules.Empty();
        LOD.Modules.Reserve(NumModules);
        for (int32 i = 0; i < NumModules; ++i)
        {
            FString ModuleClassName;
            Ar << ModuleClassName; // 모듈의 실제 클래스 이름 읽기
            UParticleModule* NewModule = CreateModuleByClassName(ModuleClassName, &LOD);

            if (NewModule)
            {
                Ar << (*NewModule); // 해당 타입의 operator<< 호출
                LOD.Modules.Add(NewModule);
            }
        }
    }
    else // Saving
    {
        int32 NumModules = LOD.Modules.Num();
        Ar << NumModules;

        for (UParticleModule* Module : LOD.Modules)
        {
            if (Module)
            {
                FString ModuleClassName = Module->GetClass()->GetName();
                Ar << ModuleClassName;
                Ar << (*Module);
            }
        }
    }

    return Ar;
}
