#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModuleRequired.h"

UParticleSystem::UParticleSystem()
    : Delay(0.0f)
    , bAutoActivate(true) // 기본적으로 자동 활성화
{
}

// 시스템 내 모든 이미터 빌드
void UParticleSystem::BuildEmitters()
{
    // Emitters 배열에 있는 각 UParticleEmitter에 대해 Build() 함수를 호출합니다.
    for (UParticleEmitter* Emitter : Emitters)
    {
        if (Emitter)
        {
            Emitter->Build(); // 각 이미터의 Build 함수 호출
        }
    }
}

void UParticleSystem::InitializeSystem()
{
    BuildEmitters();
    UpdateComputedFlags();
}

void UParticleSystem::UpdateComputedFlags() // 이 함수는 private 멤버로 선언하는 것이 좋음
{
    bIsLooping_Computed = false; // 기본값으로 시작

    for (const UParticleEmitter* Emitter : Emitters)
    {
        if (Emitter)
        {
            UParticleLODLevel* HighLOD = Emitter->GetHighestLODLevel(); // UParticleEmitter에 GetHighestLODLevel() 구현 필요
            if (HighLOD && HighLOD->RequiredModule)
            {
                // UParticleModuleRequired에 EmitterLoops 멤버 필요
                if (HighLOD->RequiredModule->EmitterLoops == 0) // 0이면 무한 루프로 간주
                {
                    bIsLooping_Computed = true;
                    break; // 하나라도 무한 루프면 전체 시스템도 루핑
                }
            }
        }
    }
}
void UParticleSystem::PostLoad()
{
    InitializeSystem();
}
