#pragma once

#include "SubEngine.h"
class UStaticMeshComponent;
class UParticleSystemComponent;
class UParticleSystem;
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
    void OpenParticleSystemForEditing(UParticleSystem* InParticleSystem);
    UStaticMeshComponent* UnrealSphereComponent = nullptr;
    UParticleSystem* ParticleSystem = nullptr;
    UParticleSystemComponent* ParticleComponent = nullptr;
};
