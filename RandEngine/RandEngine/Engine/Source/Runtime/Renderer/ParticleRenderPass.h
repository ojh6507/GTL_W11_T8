#pragma once

#include "Define.h"
#include "IRenderPass.h"
#include "EngineBaseTypes.h"
#include "Classes/Particle/ParticleSystemComponent.h"
#include "Classes/Particle/ParticleHelper.h"

struct FSpriteParticleConstants
{
    FVector CameraUp;
    int32 bHasTexture;
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

    void UpdateSpriteParticleConstant(const std::shared_ptr<FEditorViewportClient>& Viewport, int32 hasTexture = 1) const;

    void UpdateObjectConstant(const FVector4& UUIDColor, bool bIsSelected) const;

    void UpdateLitUnlitConstant(int32 isLit) const;

    void PrepareSpriteParticleRender(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void PrepareMeshParticleRender(const std::shared_ptr<FEditorViewportClient>& Viewport);

    virtual void Render(const std::shared_ptr<FEditorViewportClient>& Viewport) override;

    void RenderSpriteParticle(const std::shared_ptr<FEditorViewportClient>& Viewport, const FDynamicSpriteEmitterData* SpriteEmitter);

    void RenderMeshParticle(const std::shared_ptr<FEditorViewportClient>& Viewport, const FDynamicMeshEmitterData* MeshEmitter, const UParticleSystemComponent* Particle);

    virtual void ClearRenderArr() override;

    void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport);

    void PrepareSpriteParticleShader() const;

    void CreateShader();

    void CreateBlendStates();

    void ChangeViewMode(EViewModeIndex ViewMode); // MeshParticle Prepare Shader

    //for Viewer
    void PrepareRenderSingleParticle(const std::shared_ptr<FEditorViewportClient>& Viewport, UParticleSystemComponent* InParticleComponent);
    void RenderSingleParticle(const std::shared_ptr<FEditorViewportClient>& Viewport, UParticleSystemComponent* InParticleComponent);
protected:
    TArray<UParticleSystemComponent*> ParticleComps;

private:
    FDXDBufferManager* BufferManager;

    FGraphicsDevice* Graphics;

    FDXDShaderManager* ShaderManager;

    ID3D11BlendState* BlendStates[3];
};
