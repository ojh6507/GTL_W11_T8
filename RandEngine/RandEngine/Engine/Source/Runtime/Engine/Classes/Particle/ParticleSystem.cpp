#include "ParticleSystem.h"
#include "ParticleEmitter.h"

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
