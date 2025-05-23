#include "SubRenderer.h"

#include "AnimationSubEngine.h"
#include "RendererHelpers.h"
#include "SkeletalSubEngine.h"
#include "ParticleSystemSubEngine.h"
#include "StaticMeshRenderPass.h"
#include "Actors/Cube.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "UnrealEd/EditorViewportClient.h"
#include "SkeletalMeshDebugger.h"
#include "PropertyEditor/ShowFlags.h"
#include "LineRenderPass.h"
#include "ParticleRenderPass.h"

FSubRenderer::~FSubRenderer()
{
    Release();
}

void FSubRenderer::Initialize(FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, USubEngine* InEngine)
{
    Engine = InEngine;
    Graphics = InGraphics;
    BufferManager = InBufferManager;

    ShaderManager = new FDXDShaderManager(Graphics->Device);
    LineRenderPass = new FLineRenderPass();
    ParticleRenderPass = new FParticleRenderPass();

    UINT MaterialBufferSize = sizeof(FMaterialConstants);
    BufferManager->CreateBufferGeneric<FMaterialConstants>("FMaterialConstants", nullptr, MaterialBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT LitUnlitBufferSize = sizeof(FLitUnlitConstants);
    BufferManager->CreateBufferGeneric<FLitUnlitConstants>("FLitUnlitConstants", nullptr, LitUnlitBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT LightInfoBufferSize = sizeof(FLightInfoBuffer);
    BufferManager->CreateBufferGeneric<FLightInfoBuffer>("FLightInfoBuffer", nullptr, LightInfoBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT ObjectBufferSize = sizeof(FObjectConstantBuffer);
    BufferManager->CreateBufferGeneric<FObjectConstantBuffer>("FObjectConstantBuffer", nullptr, ObjectBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT CameraConstantBufferSize = sizeof(FCameraConstantBuffer);
    BufferManager->CreateBufferGeneric<FCameraConstantBuffer>("FCameraConstantBuffer", nullptr, CameraConstantBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    UINT BonesBufferSize = sizeof(FBonesConstants);
    BufferManager->CreateBufferGeneric<FBonesConstants>("FBonesConstants", nullptr, BonesBufferSize, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

    LineRenderPass->Initialize(BufferManager, Graphics, ShaderManager);
    ParticleRenderPass->Initialize(BufferManager, Graphics, ShaderManager);

    ID3D11Buffer* ObjectBuffer = BufferManager->GetConstantBuffer(TEXT("FObjectConstantBuffer"));
    ID3D11Buffer* CameraConstantBuffer = BufferManager->GetConstantBuffer(TEXT("FCameraConstantBuffer"));
    Graphics->DeviceContext->VSSetConstantBuffers(12, 1, &ObjectBuffer);
    Graphics->DeviceContext->VSSetConstantBuffers(13, 1, &CameraConstantBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(12, 1, &ObjectBuffer);
    Graphics->DeviceContext->PSSetConstantBuffers(13, 1, &CameraConstantBuffer);

    D3D11_INPUT_ELEMENT_DESC StaticMeshLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"MATERIAL_INDEX", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    D3D11_INPUT_ELEMENT_DESC SkeletalMeshLayoutDesc[] = {
        // StaticMesh 요소
        { "POSITION",        0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0,                             D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",           0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",          0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TANGENT",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",        0, DXGI_FORMAT_R32G32_FLOAT,       0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "MATERIAL_INDEX",  0, DXGI_FORMAT_R32_UINT,           0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },

        // SkeletalMesh용 본 인덱스/가중치
        { "BONEINDICES",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BONEWEIGHTS",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"StaticMeshVertexShader", L"Shaders/StaticMeshVertexShader.hlsl", "mainVS", StaticMeshLayoutDesc, ARRAYSIZE(StaticMeshLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }
    hr = ShaderManager->AddVertexShaderAndInputLayout(L"SkeletalMeshVertexShader", L"Shaders/SkeletalMeshVertexShader.hlsl", "mainVS", SkeletalMeshLayoutDesc, ARRAYSIZE(SkeletalMeshLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }
    hr = ShaderManager->AddPixelShader(L"StaticMeshPixelShader", L"Shaders/StaticMeshPixelShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        // TODO: 적절한 오류 처리
        return;
    }
    D3D_SHADER_MACRO DefinesBlinnPhong[] =
    {
        { PHONG, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"PHONG_StaticMeshPixelShader", L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefinesBlinnPhong);
    if (FAILED(hr))
    {
        // TODO: 적절한 오류 처리
        return;
    }
    D3D_SHADER_MACRO DefinesPBR[] =
    {
        { PBR, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddPixelShader(L"PBR_StaticMeshPixelShader", L"Shaders/StaticMeshPixelShader.hlsl", "mainPS", DefinesPBR);
    if (FAILED(hr))
    {
        // TODO: 적절한 오류 처리
        return;
    }
    hr = ShaderManager->AddPixelShader(L"StaticMeshPixelShaderWorldNormal", L"Shaders/StaticMeshPixelShaderWorldNormal.hlsl", "mainPS", DefinesPBR);
    if (FAILED(hr))
    {
        // TODO: 적절한 오류 처리
        return;
    }
}

void FSubRenderer::Release()
{
    if (ShaderManager)
    {
        delete ShaderManager;
        ShaderManager = nullptr;
    }
    if (LineRenderPass)
    {
        delete LineRenderPass;
        LineRenderPass = nullptr;
    }
    if (ParticleRenderPass)
    {
        delete  ParticleRenderPass;
        ParticleRenderPass = nullptr;
    }
}

void FSubRenderer::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const EViewModeIndex ViewMode = Viewport->GetViewMode();
    TargetViewport = Viewport.get();
    UpdateViewCamera(Viewport);
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_SubScene);
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_SubScene);
    // Set RTV + DSV

    // Clear RenderTarget

    //========= FOR ParticleViewr Test =============
    // 파티클 렌더패스 생겼을 시 제거
    Graphics->DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
    if (UParticleSystemSubEngine* ParticleSubEngine = Cast<UParticleSystemSubEngine>(Engine))
    {
    }
    else
    {
        Graphics->DeviceContext->ClearRenderTargetView(Graphics->BackBufferRTV, Graphics->ClearColor);
    }
    Graphics->DeviceContext->ClearDepthStencilView(Graphics->DeviceDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    if (Cast<UParticleSystemSubEngine>(Engine))
    {
    }
    else
    {
        Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->BackBufferRTV, Graphics->DeviceDSV);
    }

    // -----------------------------파티클 렌더패스 생겼을 때 RenderTargetRHI->RTV 다 주석처리 
     // Set Viewport
    if (ViewMode == EViewModeIndex::VMI_Wireframe)
    {
        Graphics->DeviceContext->RSSetState(Graphics->RasterizerWireframeBack);
    }
    else
    {
        Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
    }
    // Set Rasterizer + DSS
    Graphics->DeviceContext->OMSetDepthStencilState(Graphics->DepthStencilState, 0);
    FEngineLoop::PrimitiveDrawBatch.InitializeGrid(0, 0, 0);
    PrepareStaticRenderArr(Viewport);

}
void FSubRenderer::Render()
{
    if (UParticleSystemSubEngine* ParticleSubEngine = Cast<UParticleSystemSubEngine>(Engine))
    {
        D3D11_RECT scissorRect;
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = 1024;  // 현재 렌더 타겟의 너비
        scissorRect.bottom = 1024; // 현재 렌더 타겟의 높이

        Graphics->DeviceContext->RSSetScissorRects(1, &scissorRect);


        ParticleRenderPass->RenderSingleParticle(ParticleSubEngine->ViewportClient, ParticleSubEngine->ParticleComponent);
    }

    if (PreviewSkeletalMesh == nullptr)
    {
        ////========= FOR ParticleViewr Test =============
        //// 파티클 렌더패스 생겼을 시 제거
        //D3D11_RECT scissorRect;
        //scissorRect.left = 0;
        //scissorRect.top = 0;
        //scissorRect.right = 1024;  // 현재 렌더 타겟의 너비
        //scissorRect.bottom = 1024; // 현재 렌더 타겟의 높이
        //
        //Graphics->DeviceContext->RSSetScissorRects(1, &scissorRect);

        //ID3D11VertexShader* vertexShader = nullptr;
        //ID3D11InputLayout* inputLayout = nullptr;
        //
        //vertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
        //inputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader"); // VS와 함께 생성했으므로 같은 키 사용
        //
        //Graphics->DeviceContext->VSSetShader(vertexShader, nullptr, 0);

        //ID3D11PixelShader* pixelShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShader");

        //Graphics->DeviceContext->PSSetShader(pixelShader, nullptr, 0);

        //Graphics->DeviceContext->IASetInputLayout(inputLayout);

        //Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        //UpdateObjectConstant(FMatrix::Identity, FVector4(), false);

        //UpdateConstants();

        //RenderStaticMesh();

        Graphics->DeviceContext->OMSetRenderTargets(1, &Graphics->BackBufferRTV, Graphics->DeviceDSV);

        //=================================================
        return;
    }
    // 셰이더 설정

    // GPU SKinning 
    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
    if (GEngineLoop.GetSkinningType() == ST_GPU)
    {
        vertexShader = ShaderManager->GetVertexShaderByKey(L"SkeletalMeshVertexShader");
        inputLayout = ShaderManager->GetInputLayoutByKey(L"SkeletalMeshVertexShader"); // VS와 함께 생성했으므로 같은 키 사용
        UpdateBoneConstants();
    }
    else if (GEngineLoop.GetSkinningType() == ST_CPU)
    {
        vertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
        inputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader"); // VS와 함께 생성했으므로 같은 키 사용
    }
    ID3D11PixelShader* pixelShader = ShaderManager->GetPixelShaderByKey(L"PHONG_StaticMeshPixelShader");

    if (!vertexShader || !inputLayout || !pixelShader)
    {
        return;
    }

    Graphics->DeviceContext->VSSetShader(vertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(pixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(inputLayout);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UpdateObjectConstant(FMatrix::Identity, FVector4(), false);
    UpdateConstants();
    RenderMesh();
    if (GEngineLoop.GetSkinningType() == ST_GPU)
    {
        vertexShader = ShaderManager->GetVertexShaderByKey(L"StaticMeshVertexShader");
        inputLayout = ShaderManager->GetInputLayoutByKey(L"StaticMeshVertexShader"); // VS와 함께 생성했으므로 같은 키 사용

        Graphics->DeviceContext->VSSetShader(vertexShader, nullptr, 0);
        Graphics->DeviceContext->PSSetShader(pixelShader, nullptr, 0);
        Graphics->DeviceContext->IASetInputLayout(inputLayout);
    }
    RenderStaticMesh();

    LineRenderPass->ProcessLineRendering();
}

void FSubRenderer::RenderMesh()
{
    FSkeletalMeshRenderData* RenderData = PreviewSkeletalMesh->GetRenderData();

    TArray<FStaticMaterial*> RenderMaterial = PreviewSkeletalMesh->GetMaterials();
    const uint64 ShowFlag = TargetViewport->GetShowFlag();
    if (ShowFlag & EEngineShowFlags::SF_Bone) {
        FSkeletalMeshDebugger::DrawSkeleton(PreviewSkeletalMeshComp);
        FSkeletalMeshDebugger::DrawSkeletonAABBs(PreviewSkeletalMeshComp);
    }


    UINT Stride = sizeof(FSkeletalMeshVertex);
    UINT Offset = 0;
    FVertexInfo VertexInfo;
    BufferManager->CreateDynamicVertexBuffer(RenderData->MeshName, RenderData->Vertices, VertexInfo);

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->MeshName, RenderData->Indices, IndexInfo);

    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }

    if (RenderData->Subsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
        return;
    }

    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->Subsets.Num(); SubMeshIndex++)
    {
        uint32 MaterialIndex = RenderData->Subsets[SubMeshIndex].MaterialIndex;

        MaterialUtils::UpdateMaterial(BufferManager, Graphics, RenderMaterial[MaterialIndex]->Material->GetMaterialInfo());

        uint32 StartIndex = RenderData->Subsets[SubMeshIndex].IndexStart;
        uint32 IndexCount = RenderData->Subsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
    }
}

void FSubRenderer::PrepareStaticRenderArr(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    // if (Engine->EditorPlayer->GetControlMode() == CM_TRANSLATION)
    // {
    //     for (auto iter : Viewport->GetGizmoActor()->GetArrowArr())
    //         StaticMeshComponents.Add(iter);
    // }
    // else if (Engine->EditorPlayer->GetControlMode() == CM_ROTATION)
    // {
    //     for (auto iter : Viewport->GetGizmoActor()->GetDiscArr())
    //         StaticMeshComponents.Add(iter);
    // }
    // else
    // {
    //     for (auto iter : Viewport->GetGizmoActor()->GetScaleArr())
    //         StaticMeshComponents.Add(iter);
    // }
    if (Cast<USkeletalSubEngine>(Engine))
    {
        StaticMeshComponents.Add(Cast<USkeletalSubEngine>(Engine)->BasePlane->GetStaticMeshComponent());
        StaticMeshComponents.Add(Cast<USkeletalSubEngine>(Engine)->UnrealSphereComponent);
    }
    else if (Cast<UAnimationSubEngine>(Engine))
        StaticMeshComponents.Add(Cast<UAnimationSubEngine>(Engine)->BasePlane->GetStaticMeshComponent());
    /*else if (Cast<UParticleSystemSubEngine>(Engine))
        StaticMeshComponents.Add(Cast<UParticleSystemSubEngine>(Engine)->UnrealSphereComponent);*/
}

void FSubRenderer::RenderStaticMesh()
{
    for (UStaticMeshComponent* Comp : StaticMeshComponents)
    {
        if (!Comp || !Comp->GetStaticMesh())
        {
            continue;
        }
        FStaticMeshRenderData* RenderData = Comp->GetStaticMesh()->GetRenderData();
        if (RenderData == nullptr)
        {
            continue;
        }

        FMatrix WorldMatrix = Comp->GetWorldMatrix();
        bool bSelecet = false;
        EViewModeIndex ViewMode = TargetViewport->GetViewMode();
        if (Cast<UGizmoBaseComponent>(Comp))
        {
            USceneComponent* Gizmo = TargetViewport->GetPickedGizmoComponent();
            bSelecet = (Gizmo == Comp);
            Graphics->DeviceContext->RSSetState(Graphics->RasterizerSolidBack);
        }
        UpdateObjectConstant(WorldMatrix, FVector4(), bSelecet);

        RenderPrimitive(RenderData, Comp->GetStaticMesh()->GetMaterials(), Comp->GetOverrideMaterials(), Comp->GetselectedSubMeshIndex());
        if (ViewMode == EViewModeIndex::VMI_Wireframe)
        {
            Graphics->DeviceContext->RSSetState(Graphics->RasterizerWireframeBack);
        }
    }
}

void FSubRenderer::ClearRender()
{
    ShaderManager->ReloadAllShaders();
    StaticMeshComponents.Empty();
}

void FSubRenderer::UpdateObjectConstant(const FMatrix& WorldMatrix, const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    ObjectData.WorldMatrix = WorldMatrix;
    ObjectData.InverseTransposedWorld = FMatrix::Transpose(FMatrix::Inverse(WorldMatrix));
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;

    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

void FSubRenderer::UpdateLightConstant() const
{
    BufferManager->BindConstantBuffer(TEXT("FLightInfoBuffer"), 0, EShaderStage::Vertex);

    FLightInfoBuffer LightBufferData = {};
    LightBufferData.DirectionalLightsCount = 0;
    LightBufferData.PointLightsCount = 0;
    LightBufferData.SpotLightsCount = 0;

    FAmbientLightInfo AmbientLightInfo;
    AmbientLightInfo.AmbientColor = FLinearColor(.1f, .1f, .1f, 1.0f);

    LightBufferData.AmbientLightsCount = 1;
    LightBufferData.Ambient[0] = AmbientLightInfo;

    BufferManager->UpdateConstantBuffer(TEXT("FLightInfoBuffer"), LightBufferData);
}

void FSubRenderer::UpdateConstants() const
{
    TArray<FString> PSBufferKeys = {
        TEXT("FLightInfoBuffer"),
        TEXT("FMaterialConstants"),
        TEXT("FLitUnlitConstants")
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Pixel);

    /** Lit Flag */
    FLitUnlitConstants Data;
    Data.bIsLit = false;
    Data.bUseInputColor = 0;
    BufferManager->UpdateConstantBuffer(TEXT("FLitUnlitConstants"), Data);

    /** Light */
    UpdateLightConstant();
}

void FSubRenderer::UpdateBoneConstants() const
{
    BufferManager->BindConstantBuffer(TEXT("FBonesConstants"), 2, EShaderStage::Vertex);

    FBonesConstants Data;
    for (int32 i = 0; i < PreviewSkeletalMesh->Skeleton->CurrentPose.SkinningMatrices.Num(); i++)
        Data.BoneMatrices[i] = PreviewSkeletalMesh->Skeleton->CurrentPose.SkinningMatrices[i];
    BufferManager->UpdateConstantBuffer(TEXT("FBonesConstants"), Data);
}


void FSubRenderer::UpdateViewCamera(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    FCameraConstantBuffer CameraConstantBuffer;
    CameraConstantBuffer.ViewMatrix = Viewport->GetViewMatrix();
    CameraConstantBuffer.InvViewMatrix = FMatrix::Inverse(CameraConstantBuffer.ViewMatrix);
    CameraConstantBuffer.ProjectionMatrix = Viewport->GetProjectionMatrix();
    CameraConstantBuffer.InvProjectionMatrix = FMatrix::Inverse(CameraConstantBuffer.ProjectionMatrix);
    CameraConstantBuffer.ViewLocation = Viewport->GetCameraLocation();
    CameraConstantBuffer.NearClip = Viewport->GetCameraNearClip();
    CameraConstantBuffer.FarClip = Viewport->GetCameraFarClip();
    BufferManager->UpdateConstantBuffer("FCameraConstantBuffer", CameraConstantBuffer);
}

void FSubRenderer::SetPreviewSkeletalMesh(USkeletalMesh* InPreviewSkeletalMesh)
{
    PreviewSkeletalMesh = InPreviewSkeletalMesh;
    bOnlyOnce = false;
}

void FSubRenderer::SetPreviewSkeletalMeshComponent(USkeletalMeshComponent* InPreviewSkeletalMeshComp)
{
    PreviewSkeletalMeshComp = InPreviewSkeletalMeshComp;
}

void FSubRenderer::RenderPrimitive(FStaticMeshRenderData* RenderData, TArray<FStaticMaterial*> Materials, TArray<UMaterial*> OverrideMaterials,
    int SelectedSubMeshIndex) const
{
    UINT Stride = sizeof(FStaticMeshVertex);
    UINT Offset = 0;

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &Stride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }

    if (RenderData->MaterialSubsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexed(RenderData->Indices.Num(), 0, 0);
        return;
    }

    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        uint32 MaterialIndex = RenderData->MaterialSubsets[SubMeshIndex].MaterialIndex;

        FSubMeshConstants SubMeshData = (SubMeshIndex == SelectedSubMeshIndex) ? FSubMeshConstants(true) : FSubMeshConstants(false);

        BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);

        if (OverrideMaterials[MaterialIndex] != nullptr)
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, OverrideMaterials[MaterialIndex]->GetMaterialInfo());
        }
        else
        {
            MaterialUtils::UpdateMaterial(BufferManager, Graphics, Materials[MaterialIndex]->Material->GetMaterialInfo());
        }

        uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexed(IndexCount, StartIndex, 0);
    }
}
