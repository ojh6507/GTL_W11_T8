#include "ParticleSystemSubEngine.h"
#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubRenderer.h"
#include "UnrealClient.h"
#include "Actors/Cube.h"
#include "Animation/Skeleton.h"
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


    EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>(this);
    EditorPlayer->SetCoordMode(CDM_LOCAL);

}

void UParticleSystemSubEngine::Tick(float DeltaTime)
{
    ViewportClient->Tick(DeltaTime);
    Input(DeltaTime);
    Render();
}

void UParticleSystemSubEngine::Input(float DeltaTime)
{
    if (::GetForegroundWindow() != *Wnd)
        return;
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
        if (GetAsyncKeyState('W') & 0x8000)
        {
            EditorPlayer->SetMode(CM_TRANSLATION);
        }
        if (GetAsyncKeyState('E') & 0x8000)
        {
            EditorPlayer->SetMode(CM_ROTATION);
        }
        if (GetAsyncKeyState('R') & 0x8000)
        {
            EditorPlayer->SetMode(CM_SCALE);
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

        reinterpret_cast<ParticleSystemViewerPanel*>(UnrealEditor->GetSubParticlePanel("SubParticleViewerPanel").get())->PrepareRender(ViewportClient);

        UnrealEditor->Render(EWindowType::WT_ParticleSubWindow);
        SubUI->EndFrame();

        // Sub swap
        SubRenderer->ClearRender();
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
