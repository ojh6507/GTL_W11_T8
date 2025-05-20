#pragma once
#include "Container/Array.h"
#include "Math/Matrix.h"
#include "Math/Vector4.h"

class UMaterial;
struct FStaticMaterial;
struct FStaticMeshRenderData;
class USubEngine;
class ACubeActor;
class AStaticMeshActor;
class UStaticMeshComponent;
class FEditorViewportClient;
class USkeletalMesh;
class FSubCamera;
class FDXDShaderManager;
class FStaticMeshRenderPass;
class FDXDBufferManager;
class FGraphicsDevice;
class FLineRenderPass;
class USkeletalMeshComponent;
class FParticleRenderPass;

class FSubRenderer
{
public:
    /** Initialize */
    FSubRenderer() = default;
    ~FSubRenderer();
    
    void Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, USubEngine* InEngine);
    void Release();

    /** Render */
    // void PrepareRender(const FSubCamera& Camera) const;
    // void Render(FSubCamera& Camera);
    // void RenderMesh(FSubCamera& Camera);
    void PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void Render();
    void RenderMesh();
    void PrepareStaticRenderArr(const std::shared_ptr<FEditorViewportClient>& Viewport);
    void RenderStaticMesh();
    void ClearRender();

    void UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const;

    void UpdateLightConstant() const;
    void UpdateConstants() const;
    void UpdateBoneConstants() const;
    /** Update Buffer */
    void UpdateViewCamera(const std::shared_ptr<FEditorViewportClient>& Viewport) const;
    /** Set */
    void SetPreviewSkeletalMesh(USkeletalMesh* InPreviewSkeletalMesh);
    void SetPreviewSkeletalMeshComponent(USkeletalMeshComponent* InPreviewSkeletalMeshComp);
    void RenderPrimitive(FStaticMeshRenderData* RenderData, TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials, int SelectedSubMeshIndex) const;
private:
    FGraphicsDevice* Graphics;
    FDXDBufferManager* BufferManager;
    FDXDShaderManager* ShaderManager = nullptr;
    USubEngine* Engine = nullptr;
    TArray<UStaticMeshComponent*> StaticMeshComponents;
    USkeletalMesh* PreviewSkeletalMesh = nullptr;
    USkeletalMeshComponent* PreviewSkeletalMeshComp = nullptr;
    FEditorViewportClient* TargetViewport;
    FLineRenderPass* LineRenderPass = nullptr;

    FParticleRenderPass* ParticleRenderPass = nullptr;
private:
    /** TargetPos & MaxZ Offset */
    bool bOnlyOnce = false;
};
