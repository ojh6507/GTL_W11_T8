#pragma once
#include "LightActor.h"
class ASpotLight :
    public ALight
{
    DECLARE_CLASS(ASpotLight, ALight)
public:
    ASpotLight();
    virtual ~ASpotLight();
protected:
    UPROPERTY
    (USpotLightComponent*, SpotLightComponent, = nullptr);

    UPROPERTY
    (UBillboardComponent*, BillboardComponent, = nullptr);
};

