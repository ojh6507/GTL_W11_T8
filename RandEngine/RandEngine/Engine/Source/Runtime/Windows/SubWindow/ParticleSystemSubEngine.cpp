#include "ParticleSystemSubEngine.h"
#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubRenderer.h"
#include "UnrealClient.h"
#include "Actors/Cube.h"
#include "Animation/Skeleton.h"

#include "Particle/ParticleSystem.h"
#include "Particle/ParticleSystemComponent.h"

#include "UserInterface/Drawer.h"
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

    ViewportClient->ViewFOV = 60.f;
    ParticleComponent = FObjectFactory::ConstructObject<UParticleSystemComponent>(this);
   
    FViewportCamera& ViewTransform = ViewportClient->PerspectiveCamera;
    ViewportClient->PerspectiveCamera.SetLocation(FVector(0, 0, 0));
    ViewportClient->PerspectiveCamera.SetRotation(FVector(0, 0, 0));
    ViewTransform.SetLocation(
        FVector(0,0,0) - (ViewTransform.GetForwardVector() * 100.0f)
    );
    ViewportClient->CameraRotateYaw(20);
    ViewportClient->CameraRotatePitch(15);
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
        if (bCtrlPressed)
        {
            if (GetAsyncKeyState('S') & 0x8000)
            {
                if (ParticleSystem)
                {
                    ParticleSystem->SaveParticleSystemToBinary();
                }
            }
            if (GetAsyncKeyState('O') & 0x8000)
            {
                static_cast<FDrawer*>(GEngineLoop.GetUnrealEditor()->GetSubParticlePanel("Drawer").get())->Toggle();
            }
        }
        if (GetAsyncKeyState('F') & 0x8000)
        {
            FViewportCamera& ViewTransform = ViewportClient->PerspectiveCamera;

            ViewportClient->PerspectiveCamera.SetLocation(FVector(0, 0, 0));
            ViewportClient->PerspectiveCamera.SetRotation(FVector(0, 0, 0));

            ViewTransform.SetLocation(
                FVector(0, 0, 0) - (ViewTransform.GetForwardVector() * 100.0f)
            );
            ViewportClient->CameraRotateYaw(20);
            ViewportClient->CameraRotatePitch(15);

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

void UParticleSystemSubEngine::RefreshParticleComponent()
{
    if(ParticleComponent)
        ParticleComponent->InitParticles();
}

void UParticleSystemSubEngine::OpenParticleSystemForEditing(UParticleSystem* InParticleSystem)
{
    ParticleSystem = InParticleSystem;
    ParticleComponent->SetParticleTemplate(ParticleSystem);
    ParticleComponent->InitParticles();
    ParticleSystemViewerPanel* particlePanel = reinterpret_cast<ParticleSystemViewerPanel*>(UnrealEditor->GetSubParticlePanel("SubParticleViewerPanel").get());
    particlePanel->SetEditedParticleSystem(ParticleSystem);
 
    FViewportCamera& ViewTransform = ViewportClient->PerspectiveCamera;
    ViewportClient->PerspectiveCamera.SetLocation(FVector(0, 0, 0));
    ViewportClient->PerspectiveCamera.SetRotation(FVector(0, 0, 0));
    ViewTransform.SetLocation(
        FVector(0,0,0) - (ViewTransform.GetForwardVector() * 180.0f)
    );
    ViewportClient->CameraRotateYaw(20);
    ViewportClient->CameraRotatePitch(15);

}
