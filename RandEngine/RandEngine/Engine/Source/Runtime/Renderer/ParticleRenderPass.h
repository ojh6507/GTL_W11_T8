#pragma once

#include "Define.h"
#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Classes/Particle/ParticleSystemComponent.h"
#include "Classes/Particle/ParticleHelper.h"

struct FSpriteParticleCameraConstants
{
    FVector CameraUp;
    float pad;
    FVector CameraRight;
    float pad1;
};

class FParticleRenderPass : public IRenderPass
{
public:
    FParticleRenderPass();
    virtual ~FParticleRenderPass();

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;

    virtual void PrepareRenderArr() override;

    void UpdateCameraConstant(const FVector& CameraUp, const FVector& CameraRight) const;

    void PrepareSpriteParticleRender();

    void PrepareMeshParticleRender();

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

    void PrepareSpriteParticleShader() const;

    void PrepareMeshParticleShader() const;

    void CreateShader();

protected:
    TArray<UParticleSystemComponent*> ParticleComps;

private:
    FDXDBufferManager* BufferManager;

    FGraphicsDevice* Graphics;

    FDXDShaderManager* ShaderManager;
};
