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
    /*
    //테스트용 코드
    ID3D11Buffer* VB = nullptr;
    ID3D11Buffer* IB = nullptr;

    TArray<FParticleSpriteVertex> Vertices;
    TArray<uint32> Indices;

    for (int i = 0; i < 10; ++i)
    {
        FVector Center = FVector(0, i * 5.f, 0);
        for (int j = 0; j < 4; ++j)
        {
            FParticleSpriteVertex Vertex =
            {
                Center,
                0.f,
                FVector(0, 0, 0),
                i,
                FVector2D(i+1, i+1),
                FMath::DegreesToRadians(i * 10.f),
                0.f,
                FLinearColor(i/10.f, j/10.f, 1, 1)
            };
            Vertices.Add(Vertex);
        }
        Indices.Add(0 + i * 4);
        Indices.Add(1 + i * 4);
        Indices.Add(2 + i * 4);
        Indices.Add(0 + i * 4);
        Indices.Add(2 + i * 4);
        Indices.Add(3 + i * 4);
    }

    UINT Stride = sizeof(FParticleSpriteVertex);
    UINT Offset = 0;

    D3D11_BUFFER_DESC VBDesc = {};
    VBDesc.ByteWidth = Vertices.Num() * sizeof(FParticleSpriteVertex);
    VBDesc.Usage = D3D11_USAGE_DEFAULT;
    VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VBDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA VBInitData = {};
    VBInitData.pSysMem = Vertices.GetData();

    HRESULT hr = Graphics->Device->CreateBuffer(&VBDesc, &VBInitData, &VB);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create VertexBuffer"));
    }

    D3D11_BUFFER_DESC IBDesc = {};
    IBDesc.ByteWidth = Indices.Num() * sizeof(uint32);
    IBDesc.Usage = D3D11_USAGE_DEFAULT;
    IBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    IBDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA IBInitData = {};
    IBInitData.pSysMem = Indices.GetData();

    hr = Graphics->Device->CreateBuffer(&IBDesc, &IBInitData, &IB);
    if (FAILED(hr))
    {
        UE_LOG(ELogLevel::Error, TEXT("Failed to create IndexBuffer"));
    }

    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &VB, &Stride, &Offset);
    Graphics->DeviceContext->IASetIndexBuffer(IB, DXGI_FORMAT_R32_UINT, 0);

    Graphics->DeviceContext->DrawIndexed(Indices.Num(), 0, 0);
    */
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
    ID3D11Buffer* InstanceBuffer = MeshEmitter->VertexAllocation.VertexBuffer;
    UINT VBStride = sizeof(FStaticMeshVertex);
    UINT Offset = 0;

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

    Graphics->DeviceContext->DrawIndexedInstanced(IndexInfo.NumIndices, MeshEmitter->GetSourceData()->ActiveParticleCount, 
                                                  0, 0, 0);
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
        { "COLOR",     0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Transform[0]
        { "TEXCOORD",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Transform[1]
        { "TEXCOORD",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Transform[2]
        { "TEXCOORD",  3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Velocity
        { "TEXCOORD",  4, DXGI_FORMAT_R16G16B16A16_SINT,  0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // SUBUV Params
        { "TEXCOORD",  5, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // SUBUV Lerp
        { "TEXCOORD",  6, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 }, // Relative Time
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
