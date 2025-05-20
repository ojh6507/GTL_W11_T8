#include "ParticleRenderPass.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"
#include "UnrealEd/EditorViewportClient.h"
#include <UnrealClient.h>
#include "Components/Mesh/StaticMeshRenderData.h"
#include "Engine/Asset/StaticMeshAsset.h"
#include "Engine/EditorEngine.h"
#include <RendererHelpers.h>

FParticleRenderPass::FParticleRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FParticleRenderPass::~FParticleRenderPass()
{
    for (int i = 0; i < 3; ++i)
    {
        if (BlendStates[i])
        {
            BlendStates[i]->Release();
            BlendStates[i] = nullptr;
        }
    }
}

void FParticleRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;  
    ShaderManager = InShaderManager;
    CreateShader();
    CreateBlendStates();
}

void FParticleRenderPass::PrepareRenderArr()
{
    ParticleComps.Empty();
    for (const auto iter : TObjectRange<UParticleSystemComponent>())
    {
        if (iter->GetWorld() == GEngine->ActiveWorld) //TODO: Sprite 조건 추가
        {
            ParticleComps.Add(iter);
        }
    }
}

void FParticleRenderPass::UpdateCameraConstant(const std::shared_ptr<FEditorViewportClient>& Viewport) const
{
    FSpriteParticleCameraConstants CameraData = {};
    FVector CameraUp, CameraRight;
    if (Viewport->IsPerspective())
    {
        CameraUp = Viewport->PerspectiveCamera.GetUpVector();
        CameraRight = Viewport->PerspectiveCamera.GetRightVector();
    }
    else
    {
        CameraUp = Viewport->OrthogonalCamera.GetUpVector();
        CameraRight = Viewport->OrthogonalCamera.GetRightVector();
    }
    CameraData.CameraUp = CameraUp;
    CameraData.CameraRight = CameraRight;

    BufferManager->UpdateConstantBuffer(TEXT("FSpriteParticleCameraConstantBuffer"), CameraData);
}

void FParticleRenderPass::UpdateObjectConstant(const FVector4& UUIDColor, bool bIsSelected) const
{
    FObjectConstantBuffer ObjectData = {};
    //Instance buffer에 있는 Transform 사용으로 필요없음
    ObjectData.WorldMatrix = FMatrix::Identity;
    ObjectData.InverseTransposedWorld = FMatrix::Identity;
    ObjectData.UUIDColor = UUIDColor;
    ObjectData.bIsSelected = bIsSelected;

    BufferManager->UpdateConstantBuffer(TEXT("FObjectConstantBuffer"), ObjectData);
}

void FParticleRenderPass::UpdateLitUnlitConstant(int32 isLit) const
{
    FLitUnlitConstants Data;
    Data.bIsLit = isLit;
    BufferManager->UpdateConstantBuffer(TEXT("FLitUnlitConstants"), Data);
}

void FParticleRenderPass::PrepareSpriteParticleRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    PrepareSpriteParticleShader();
    UpdateCameraConstant(Viewport);
    BufferManager->BindConstantBuffer(TEXT("FSpriteParticleCameraConstantBuffer"), 0, EShaderStage::Vertex);
    const float BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    UINT SampleMask = 0xffffffff;
    Graphics->DeviceContext->OMSetBlendState(BlendStates[0], BlendFactor, SampleMask);
}

void FParticleRenderPass::PrepareMeshParticleRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const EViewModeIndex ViewMode = Viewport->GetViewMode();
    ChangeViewMode(ViewMode);

    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    TArray<FString> PSBufferKeys = {
        TEXT("FLightInfoBuffer"),
        TEXT("FMaterialConstants"),
        TEXT("FLitUnlitConstants"),
        TEXT("FSubMeshConstants"),
        TEXT("FTextureConstants"),
    };

    BufferManager->BindConstantBuffers(PSBufferKeys, 0, EShaderStage::Pixel);
    BufferManager->BindConstantBuffer(TEXT("FDiffuseMultiplier"), 6, EShaderStage::Pixel);
    BufferManager->BindConstantBuffer(TEXT("FLightInfoBuffer"), 0, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FMaterialConstants"), 1, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FObjectConstantBuffer"), 12, EShaderStage::Vertex);

    const float BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
    UINT SampleMask = 0xffffffff;
    Graphics->DeviceContext->OMSetBlendState(BlendStates[0], BlendFactor, SampleMask);
}

void FParticleRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareRender(Viewport);
    for (const auto& Particle : ParticleComps)
    {
        TArray<FDynamicMeshEmitterData*> MeshRenderDatas;
        TArray<FDynamicSpriteEmitterData*> SpriteRenderDatas;
        for (const auto& RenderData : Particle->EmitterRenderData)   
        {
            if (FDynamicSpriteEmitterData* SpriteRenderData = dynamic_cast<FDynamicSpriteEmitterData*>(RenderData))
            {
                SpriteRenderDatas.Add(SpriteRenderData);
            }
            else if (FDynamicMeshEmitterData* MeshRenderData = dynamic_cast<FDynamicMeshEmitterData*>(RenderData))
            {
                MeshRenderDatas.Add(MeshRenderData);
            }
        }

        if (SpriteRenderDatas.Num() > 0)
        {
            PrepareSpriteParticleRender(Viewport);
            for (const auto& SpriteRenderData : SpriteRenderDatas)
            {
                RenderSpriteParticle(Viewport, SpriteRenderData);
            }
        }

        if (MeshRenderDatas.Num() > 0)
        {
            PrepareMeshParticleRender(Viewport);
            for (const auto& MeshRenderData : MeshRenderDatas)
            {
                RenderMeshParticle(Viewport, MeshRenderData, Particle);
            }
        }
    }
}

void FParticleRenderPass::RenderSpriteParticle(const std::shared_ptr<FEditorViewportClient>& Viewport, const FDynamicSpriteEmitterData* SpriteEmitter)
{
    if (!SpriteEmitter || !SpriteEmitter->VertexAllocation.IsValid() || !SpriteEmitter->IndexAllocation.IsValid())
        return;
    
    ID3D11Buffer* VertexBuffer = SpriteEmitter->VertexAllocation.VertexBuffer;
    ID3D11Buffer* IndexBuffer = SpriteEmitter->IndexAllocation.IndexBuffer;
    UINT Stride = SpriteEmitter->GetDynamicVertexStride();
    
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &SpriteEmitter->VertexAllocation.VertexOffset);
    //포맷 확인 필요
    Graphics->DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    
    //IndexCount SpriteEmitter에서 가져와야 함
    int32 IndexCount = SpriteEmitter->GetSource().ActiveParticleCount * 6;
    
    //TODO: Texture랑 Sampler 가져와야 함
    //Graphics->DeviceContext->PSSetShaderResources(0, 1, SpriteEmitter->MaterialResource->SRV);
    //Graphics->DeviceContext->PSSetSamplers(0, 1, SpriteEmitter->MaterialResource->SamplerState);

    Graphics->DeviceContext->DrawIndexed(IndexCount, SpriteEmitter->IndexAllocation.FirstIndex, 0);
}

void FParticleRenderPass::RenderMeshParticle(const std::shared_ptr<FEditorViewportClient>& Viewport, const FDynamicMeshEmitterData* MeshEmitter, const UParticleSystemComponent* Particle)
{
    UEditorEngine* Engine = Cast<UEditorEngine>(GEngine);

    USceneComponent* SelectedComponent = Engine->GetSelectedComponent();
    AActor* SelectedActor = Engine->GetSelectedActor();

    USceneComponent* TargetComponent = nullptr;

    if (SelectedComponent != nullptr)
    {
        TargetComponent = SelectedComponent;
    }
    else if (SelectedActor != nullptr)
    {
        TargetComponent = SelectedActor->GetRootComponent();
    }

    FVector4 UUIDColor = Particle->EncodeUUID() / 255.0f;
    const bool bIsSelected = (Engine && TargetComponent == Particle);

    UpdateObjectConstant(UUIDColor, bIsSelected);

    ID3D11Buffer* InstanceBuffer = MeshEmitter->VertexAllocation.VertexBuffer;
    UINT VBStride = sizeof(FStaticMeshVertex);
    UINT Offset = 0;

    if (!MeshEmitter->StaticMesh)
    {
        UE_LOG(ELogLevel::Error, "No Static Mesh in Mesh Emitter!");
        return;
    }

    FStaticMeshRenderData* RenderData = MeshEmitter->StaticMesh->GetRenderData();

    FVertexInfo VertexInfo;
    BufferManager->CreateVertexBuffer(RenderData->ObjectName, RenderData->Vertices, VertexInfo);

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VertexInfo.VertexBuffer, &VBStride, &Offset);

    FIndexInfo IndexInfo;
    BufferManager->CreateIndexBuffer(RenderData->ObjectName, RenderData->Indices, IndexInfo);
    if (IndexInfo.IndexBuffer)
    {
        Graphics->DeviceContext->IASetIndexBuffer(IndexInfo.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    }
    UINT Stride = MeshEmitter->GetDynamicVertexStride();

    Graphics->DeviceContext->IASetVertexBuffers(1, 1, &InstanceBuffer, &Stride, &MeshEmitter->VertexAllocation.VertexOffset);

    if (RenderData->MaterialSubsets.Num() == 0)
    {
        Graphics->DeviceContext->DrawIndexedInstanced(IndexInfo.NumIndices, MeshEmitter->GetSourceData()->ActiveParticleCount,
            0, 0, 0);
        return;
    }
    
    for (int SubMeshIndex = 0; SubMeshIndex < RenderData->MaterialSubsets.Num(); SubMeshIndex++)
    {
        uint32 MaterialIndex = RenderData->MaterialSubsets[SubMeshIndex].MaterialIndex;

        FSubMeshConstants SubMeshData = FSubMeshConstants(false);

        BufferManager->UpdateConstantBuffer(TEXT("FSubMeshConstants"), SubMeshData);

        MaterialUtils::UpdateMaterial(BufferManager, Graphics, MeshEmitter->MeshMaterials[MaterialIndex]->GetMaterialInfo());

        uint32 StartIndex = RenderData->MaterialSubsets[SubMeshIndex].IndexStart;
        uint32 IndexCount = RenderData->MaterialSubsets[SubMeshIndex].IndexCount;
        Graphics->DeviceContext->DrawIndexedInstanced(IndexCount, MeshEmitter->GetSourceData()->ActiveParticleCount,
            StartIndex, 0, 0);
    }
}

void FParticleRenderPass::ClearRenderArr()
{
    ParticleComps.Empty();
}

void FParticleRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    Graphics->DeviceContext->RSSetViewports(1, &Viewport->GetViewportResource()->GetD3DViewport());

    const EResourceType ResourceType = EResourceType::ERT_Scene;
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(ResourceType);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);
    for (UParticleSystemComponent* ParticleComp : ParticleComps)
    {
        ParticleComp->PrepareRenderData();
        ParticleComp->FillRenderData(Viewport);
    }
}

void FParticleRenderPass::PrepareSpriteParticleShader() const
{
    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;

    VertexShader = ShaderManager->GetVertexShaderByKey(L"SpriteParticleVertexShader");
    InputLayout = ShaderManager->GetInputLayoutByKey(L"SpriteParticleVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"SpriteParticlePixelShader");

    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
}


void FParticleRenderPass::CreateShader()
{
    D3D11_INPUT_ELEMENT_DESC SpriteParticleLayoutDesc[] =
    {
        {"TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,      0, 0,                            D3D11_INPUT_PER_VERTEX_DATA, 0 }, // UV
        { "POSITION",    0, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Position
        { "TEXCOORD",    1, DXGI_FORMAT_R32_FLOAT,         0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // RelativeTime
        { "TEXCOORD",    2, DXGI_FORMAT_R32G32B32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // OldPosition
        { "TEXCOORD",    3, DXGI_FORMAT_R32_FLOAT,         0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // ParticleId
        { "TEXCOORD",    4, DXGI_FORMAT_R32G32_FLOAT,      0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Size
        { "TEXCOORD",    5, DXGI_FORMAT_R32_FLOAT,         0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Rotation
        { "TEXCOORD",    6, DXGI_FORMAT_R32_FLOAT,         0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // SubImageIndex
        { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // Color
    };
    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout
    (L"SpriteParticleVertexShader", L"Shaders/SpriteParticleVertexShader.hlsl", "mainVS", SpriteParticleLayoutDesc, ARRAYSIZE(SpriteParticleLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }
    hr = ShaderManager->AddPixelShader(L"SpriteParticlePixelShader", L"Shaders/SpriteParticlePixelShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
        return;
    }

    D3D11_INPUT_ELEMENT_DESC MeshParticleLayoutDesc[] =
    {  
        { "POSITION",        0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "COLOR",           0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "NORMAL",          0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TANGENT",         0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD",        0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "MATERIAL_INDEX",  0, DXGI_FORMAT_R32_UINT,           0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },

        { "COLOR",           1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEXCOORD",        1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Transform[0]
        { "TEXCOORD",        2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Transform[1]
        { "TEXCOORD",        3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Transform[2]
        { "TEXCOORD",        4, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Velocity
        { "TEXCOORD",        5, DXGI_FORMAT_R16G16B16A16_SINT,  1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // SUBUV Params
        { "TEXCOORD",        6, DXGI_FORMAT_R32_FLOAT,          1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // SUBUV Lerp
        { "TEXCOORD",        7, DXGI_FORMAT_R32_FLOAT,          1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Relative Time
    };
    D3D_SHADER_MACRO DefinesGouraud[] =
    {
        { GOURAUD, "1" },
        { nullptr, nullptr }
    };
    hr = ShaderManager->AddVertexShaderAndInputLayout(L"GOURAUD_MeshParticleVertexShader", L"Shaders/MeshParticleVertexShader.hlsl", "mainVS", MeshParticleLayoutDesc, ARRAYSIZE(MeshParticleLayoutDesc), DefinesGouraud);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, "Gouraud Mesh Particle Vertex Shader Add Failed");
        return;
    }
    hr = ShaderManager->AddVertexShaderAndInputLayout
    (L"MeshParticleVertexShader", L"Shaders/MeshParticleVertexShader.hlsl", "mainVS", MeshParticleLayoutDesc, ARRAYSIZE(MeshParticleLayoutDesc));
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, "Mesh Particle Vertex Shader Add Failed");
        return;
    }
}

void FParticleRenderPass::CreateBlendStates()
{
    D3D11_BLEND_DESC BlendDesc = {};
    BlendDesc.AlphaToCoverageEnable = FALSE;
    BlendDesc.IndependentBlendEnable = FALSE;
    BlendDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    BlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HRESULT hr = Graphics->Device->CreateBlendState(&BlendDesc, &BlendStates[0]);
    if (FAILED(hr))
    {
        return;
    }

    BlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;

    hr = Graphics->Device->CreateBlendState(&BlendDesc, &BlendStates[1]);
    if (FAILED(hr))
    {
        return;
    }

    BlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;

    hr = Graphics->Device->CreateBlendState(&BlendDesc, &BlendStates[2]);
    if (FAILED(hr))
    {
        return;
    }
}

void FParticleRenderPass::ChangeViewMode(EViewModeIndex ViewMode)
{
    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;

    switch (ViewMode)
    {
    case EViewModeIndex::VMI_Lit_Gouraud:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"GOURAUD_MeshParticleVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"GOURAUD_MeshParticleVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"GOURAUD_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Lit_Lambert:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"MeshParticleVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"MeshParticleVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"LAMBERT_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Lit_BlinnPhong:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"MeshParticleVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"MeshParticleVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"PHONG_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_LIT_PBR:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"MeshParticleVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"MeshParticleVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"PBR_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    case EViewModeIndex::VMI_Wireframe:
    case EViewModeIndex::VMI_Unlit:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"MeshParticleVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"MeshParticleVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"LAMBERT_StaticMeshPixelShader");
        UpdateLitUnlitConstant(0);
        break;
    case EViewModeIndex::VMI_SceneDepth:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"MeshParticleVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"MeshParticleVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderDepth");
        UpdateLitUnlitConstant(0);
        break;
    case EViewModeIndex::VMI_WorldNormal:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"MeshParticleVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"MeshParticleVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderWorldNormal");
        UpdateLitUnlitConstant(0);
        break;
    case EViewModeIndex::VMI_WorldTangent:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"MeshParticleVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"MeshParticleVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"StaticMeshPixelShaderWorldTangent");
        UpdateLitUnlitConstant(0);
        break;
        // HeatMap ViewMode 등
    default:
        VertexShader = ShaderManager->GetVertexShaderByKey(L"MeshParticleVertexShader");
        InputLayout = ShaderManager->GetInputLayoutByKey(L"MeshParticleVertexShader");
        PixelShader = ShaderManager->GetPixelShaderByKey(L"LAMBERT_StaticMeshPixelShader");
        UpdateLitUnlitConstant(1);
        break;
    }

    // Rasterizer
    Graphics->ChangeRasterizer(ViewMode);

    // Setup
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
}
