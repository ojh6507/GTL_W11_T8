#include "ParticleRenderPass.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

FParticleRenderPass::FParticleRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
{
}

FParticleRenderPass::~FParticleRenderPass()
{
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

void FParticleRenderPass::UpdateCameraConstant(const FVector& CameraUp, const FVector& CameraRight) const
{
    FSpriteParticleCameraConstants CameraData = {};
    CameraData.CameraUp = CameraUp;
    CameraData.CameraRight = CameraRight;

    BufferManager->UpdateConstantBuffer(TEXT("FSpriteParticleCameraConstantBuffer"), CameraData);
}

void FParticleRenderPass::PrepareSpriteParticleRender()
{
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    PrepareSpriteParticleShader();
    BufferManager->BindConstantBuffer(TEXT("FSpriteParticleCameraConstantBuffer"), 0, EShaderStage::Vertex);
}

void FParticleRenderPass::PrepareMeshParticleRender()
{
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    PrepareMeshParticleShader();
}

void FParticleRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    PrepareSpriteParticleRender();
    for (const auto& Particle : ParticleComps)
    {
        TArray<FDynamicMeshEmitterData*> MeshRenderDatas;
        TArray<FDynamicSpriteEmitterData*> SpriteRenderDatas;
        //for (const auto& RenderData : Particle->GetEmitterRenderData())
        //{
        //    if (FDynamicSpriteEmitterData* SpriteRenderData = dynamic_cast<FDynamicSpriteEmitterData*>(RenderData))
        //    {
        //        SpriteRenderDatas.Add(SpriteRenderData);
        //    }
        //    else if (FDynamicMeshEmitterData* MeshRenderData = dynamic_cast<FDynamicMeshEmitterData*>(RenderData))
        //    {
        //        MeshRenderDatas.Add(MeshRenderData);
        //    }
        //}
        for (const auto& SpriteRenderData : SpriteRenderDatas)
        {
            //RenderSpriteParticle();
        }
        for (const auto& MeshRenderData : MeshRenderDatas)
        {
            //RenderMeshParticle();
        }
    }
}

void FParticleRenderPass::ClearRenderArr()
{
    ParticleComps.Empty();
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
        // Center (Position)
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        // Size
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        // UV
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        // Color
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        //Rotation
        { "TEXCOORD", 2, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
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
