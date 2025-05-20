#include "ParticleSystemSubEngine.h"
#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubRenderer.h"
#include "UnrealClient.h"
#include "Actors/Cube.h"
#include "Animation/Skeleton.h"

#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemComponent.h"

#include "Engine/AssetManager.h"
#include "PropertyEditor/Sub/ParticleSystemViewerPanel.h"

UParticleSystemSubEngine::UParticleSystemSubEngine()
{
}

UParticleSystemSubEngine::~UParticleSystemSubEngine()
{
}

void UParticleSystemSubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,
    UnrealEd* InUnrealEd)
{
    Super::Initialize(hWnd, InGraphics, InBufferManager, InSubWindow, InUnrealEd);
    SubRenderer = new FSubRenderer;
    SubRenderer->Initialize(InGraphics, InBufferManager, this);


    EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>(this);
    EditorPlayer->SetCoordMode(CDM_LOCAL);


    //UnrealSphereComponent = FObjectFactory::ConstructObject<UStaticMeshComponent>(this);
    //UnrealSphereComponent->SetStaticMesh(UAssetManager::Get().GetStaticMesh(L"Contents/Sphere.obj"));
    //UnrealSphereComponent->SetRelativeScale3D(FVector(4.f, 4.f, 4.f));
    //UnrealSphereComponent->SetRelativeLocation(FVector(0, 0, 0));
    ViewportClient->ViewFOV = 60.f;
    ParticleComponent = FObjectFactory::ConstructObject<UParticleSystemComponent>(this);
    ParticleComponent->SetRelativeLocation(FVector(0, 0, 0));
    FVector compLoc=  ParticleComponent->GetRelativeLocation();

    FViewportCamera& ViewTransform = ViewportClient->PerspectiveCamera;
   ViewTransform.SetLocation(
       compLoc - (ViewTransform.GetForwardVector() * 50.0f)
   );

}

void UParticleSystemSubEngine::Tick(float DeltaTime)
{
    ViewportClient->Tick(DeltaTime);
    ParticleComponent->TickComponent(DeltaTime);
    Input(DeltaTime);
    Render();
}

void UParticleSystemSubEngine::Input(float DeltaTime)
{
    if (::GetFocus() != *Wnd)
        return;
    bool bCtrlPressed = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;

    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        if (!bRBClicked)
        {
            bRBClicked = true;
            GetCursorPos(&LastMousePos);
        }
        POINT CursorPos;
        GetCursorPos(&CursorPos);

        float DeltaX = CursorPos.x - LastMousePos.x;
        float DeltaY = CursorPos.y - LastMousePos.y;
        ViewportClient->CameraRotateYaw(DeltaX * 0.1f);
        ViewportClient->CameraRotatePitch(DeltaY * 0.1f);
        LastMousePos = CursorPos;
    }
    else
    {
        if (bRBClicked)
        {
            bRBClicked = false;
        }
    }
    if (bRBClicked)
    {
        if (GetAsyncKeyState('A') & 0x8000)
        {
            ViewportClient->CameraMoveRight(-100.f * DeltaTime);
        }
        if (GetAsyncKeyState('D') & 0x8000)
        {
            ViewportClient->CameraMoveRight(100.f * DeltaTime);
        }
        if (GetAsyncKeyState('W') & 0x8000)
        {
            ViewportClient->CameraMoveForward(100.f * DeltaTime);
        }
        if (GetAsyncKeyState('S') & 0x8000)
        {
            ViewportClient->CameraMoveForward(-100.f * DeltaTime);
        }
        if (GetAsyncKeyState('E') & 0x8000)
        {
            ViewportClient->CameraMoveUp(100.f * DeltaTime);
        }
        if (GetAsyncKeyState('Q') & 0x8000)
        {
            ViewportClient->CameraMoveUp(-100.f * DeltaTime);
        }
    }
    else
    {
        if (bCtrlPressed && (GetAsyncKeyState('S') & 0x8000))
        {
            if (ParticleSystem)
            {
                ParticleSystem->SaveParticleSystemToBinary();
            }
        }

    }
}

void UParticleSystemSubEngine::Render()
{
    if (Wnd && IsWindowVisible(*Wnd) && Graphics->Device)
    {
        Graphics->Prepare();

        SubRenderer->PrepareRender(ViewportClient);

        SubRenderer->Render();
        // Sub window rendering
        SubUI->BeginFrame();

        ParticleSystemViewerPanel* particlePanel = reinterpret_cast<ParticleSystemViewerPanel*>(UnrealEditor->GetSubParticlePanel("SubParticleViewerPanel").get());
        if (particlePanel) 
        {
            particlePanel->PrepareRender(ViewportClient.get()); // 내부적으로 멤버 변수 RenderTargetRHI 설정

        }
        UnrealEditor->Render(EWindowType::WT_ParticleSubWindow);
        SubUI->EndFrame();

        SubRenderer->ClearRender();
        // Sub swap
        Graphics->SwapBuffer();
    }
}

void UParticleSystemSubEngine::Release()
{
    USubEngine::Release();
    if (SubUI)
    {
        SubUI->Shutdown();
        delete SubUI;
        SubUI = nullptr;
    }
    if (SubRenderer)
    {
        SubRenderer->Release();
        delete SubRenderer;
        SubRenderer = nullptr;
    }
}

void UParticleSystemSubEngine::OpenParticleSystemForEditing(UParticleSystem* InParticleSystem)
{
    ParticleSystem = InParticleSystem;
    ParticleComponent->SetParticleTemplate(ParticleSystem);
    ParticleComponent->InitParticles();
    ParticleSystemViewerPanel* particlePanel = reinterpret_cast<ParticleSystemViewerPanel*>(UnrealEditor->GetSubParticlePanel("SubParticleViewerPanel").get());
    particlePanel->SetEditedParticleSystem(ParticleSystem);
    
}
