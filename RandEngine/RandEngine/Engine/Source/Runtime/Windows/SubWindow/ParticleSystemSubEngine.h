#pragma once

#include "SubEngine.h"
class UStaticMeshComponent;
class UParticleSystemSubEngine : public USubEngine
{
    DECLARE_CLASS(UParticleSystemSubEngine, USubEngine)
public:
    UParticleSystemSubEngine();
    ~UParticleSystemSubEngine();
public:
    virtual void Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,UnrealEd* InUnrealEd);
    virtual void Tick(float DeltaTime);
    virtual void Input(float DeltaTime);
    virtual void Render();
    virtual void Release();
    UStaticMeshComponent* UnrealSphereComponent = nullptr;
};
