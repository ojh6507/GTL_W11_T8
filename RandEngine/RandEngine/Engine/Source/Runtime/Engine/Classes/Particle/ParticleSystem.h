#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UParticleEmitter;
class UParticleSystem : public UObject
{
    DECLARE_CLASS(UParticleSystem, UObject);

public:
    UParticleSystem();
    ~UParticleSystem() override = default;

    TArray<UParticleEmitter*> Emitters;
};

