#pragma once

#include "Define.h"
#include "IRenderPass.h"
#include "EngineBaseTypes.h"

struct FSpriteParticleCameraConstants
{
    FVector CameraUp;
    float pad;
    FVector CameraRight;
    float pad1;
};

class FSpriteParticleRenderPass : public IRenderPass
{
public:
    FSpriteParticleRenderPass();
    virtual ~FSpriteParticleRenderPass();

    virtual void Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager) override;

    virtual void PrepareRenderArr() override;

    void UpdateCameraConstant(const FVector& CameraUp, const FVector& CameraRight) const;

    void PrepareRender();

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    virtual void ClearRenderArr() override;

    void PrepareShader() const;

    void CreateShader();

protected:
    //TArray<UParticleSystemComponent*> ParticleComps;

private:
    FDXDBufferManager* BufferManager;

    FGraphicsDevice* Graphics;

    FDXDShaderManager* ShaderManager;
};
