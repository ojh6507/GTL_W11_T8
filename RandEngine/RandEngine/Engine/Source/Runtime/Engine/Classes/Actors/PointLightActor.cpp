#include "PointLightActor.h"
#include "Components/Light/PointLightComponent.h"
#include "Components/BillboardComponent.h"

APointLight::APointLight()
{
    PointLightComponent = AddComponent<UPointLightComponent>("PointLightComponent_0");
    BillboardComponent = AddComponent<UBillboardComponent>("UBillboardComponent_0");

    RootComponent = BillboardComponent;

    BillboardComponent->SetTexture(L"Assets/Editor/Icon/S_LightPoint.PNG");
    BillboardComponent->bIsEditorBillboard = true;

    PointLightComponent->AttachToComponent(RootComponent);
}

APointLight::~APointLight()
{
}
