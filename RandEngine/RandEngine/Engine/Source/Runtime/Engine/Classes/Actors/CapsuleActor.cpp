#include "CapsuleActor.h"
#include "Components/CapsuleComponent.h"
#include "Particle/ParticleSystemComponent.h"

ACapsuleActor::ACapsuleActor()
{
    CapsuleComponent = AddComponent<UCapsuleComponent>();
    RootComponent = CapsuleComponent;
    UParticleSystemComponent* ParticleSystem = AddComponent<UParticleSystemComponent>("ParticleSystemComponent_0");
    ParticleSystem->SetupAttachment(RootComponent);
}

UCapsuleComponent* ACapsuleActor::GetShapeComponent() const
{
    return CapsuleComponent;
}
