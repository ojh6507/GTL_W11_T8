#pragma once
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

class UParticleModule : public UObject
{
    DECLARE_CLASS(UParticleModule, UObject);

public:
    UParticleModule();
    ~UParticleModule() override = default;
};

