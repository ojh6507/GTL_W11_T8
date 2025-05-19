#include "ParticleRenderPass.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"
#include "UnrealEd/EditorViewportClient.h"
#include <UnrealClient.h>

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
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    PrepareMeshParticleShader();
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

        PrepareSpriteParticleRender(Viewport);
        for (const auto& SpriteRenderData : SpriteRenderDatas)
        {
            RenderSpriteParticle(Viewport, SpriteRenderData);
        }

        PrepareMeshParticleRender(Viewport);
        for (const auto& MeshRenderData : MeshRenderDatas)
        {
            RenderMeshParticle(Viewport, MeshRenderData);
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
    Graphics->DeviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

    //IndexCount SpriteEmitter에서 가져와야 함
    int32 IndexCount = SpriteEmitter->GetSource().ActiveParticleCount * 6;

    //TODO: Texture랑 Sampler 가져와야 함

    Graphics->DeviceContext->DrawIndexed(IndexCount, SpriteEmitter->IndexAllocation.FirstIndex, 0);
}

void FParticleRenderPass::RenderMeshParticle(const std::shared_ptr<FEditorViewportClient>& Viewport, const FDynamicMeshEmitterData* MeshEmitter)
{
    //MeshEmitter에서 VB IB 정보 가져와서 바인딩 하고 DrawIndexedInstanced();
}

void FParticleRenderPass::ClearRenderArr()
{
    ParticleComps.Empty();
}

void FParticleRenderPass::PrepareRender(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    const EResourceType ResourceType = EResourceType::ERT_Scene;
    FViewportResource* ViewportResource = Viewport->GetViewportResource();
    FRenderTargetRHI* RenderTargetRHI = ViewportResource->GetRenderTarget(ResourceType);
    FDepthStencilRHI* DepthStencilRHI = ViewportResource->GetDepthStencil(ResourceType);

    Graphics->DeviceContext->OMSetRenderTargets(1, &RenderTargetRHI->RTV, DepthStencilRHI->DSV);
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

void FParticleRenderPass::PrepareMeshParticleShader() const
{
    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;

    VertexShader = ShaderManager->GetVertexShaderByKey(L"MeshParticleVertexShader");
    InputLayout = ShaderManager->GetInputLayoutByKey(L"MeshParticleVertexShader");
    PixelShader = ShaderManager->GetPixelShaderByKey(L"MeshParticlePixelShader");

    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
}

void FParticleRenderPass::CreateShader()
{
    D3D11_INPUT_ELEMENT_DESC SpriteParticleLayoutDesc[] =
    {
        // Slot 0: 정점 (쿼드 고정)
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA,   0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0 },
                                                                                          
        // Slot 1: 인스턴스 데이터                                                         
        { "CENTER",   0, DXGI_FORMAT_R32G32B32_FLOAT,    1, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,       1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "ROTATION", 0, DXGI_FORMAT_R32_FLOAT,          1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
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
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0} //UV
    };
    hr = ShaderManager->AddVertexShaderAndInputLayout
    (L"MeshParticleVertexShader", L"Shaders/MeshParticleVertexShader.hlsl", "mainVS", MeshParticleLayoutDesc, ARRAYSIZE(MeshParticleLayoutDesc));
    if (FAILED(hr))
    {
        return;
    }
    hr = ShaderManager->AddPixelShader(L"MeshParticlePixelShader", L"Shaders/MeshParticlePixelShader.hlsl", "mainPS");
    if (FAILED(hr))
    {
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
