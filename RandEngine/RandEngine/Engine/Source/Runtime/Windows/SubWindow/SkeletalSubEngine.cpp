#include "SkeletalSubEngine.h"

#include "ImGuiManager.h"
#include "ImGuiSubWindow.h"
#include "SubRenderer.h"
#include "UnrealClient.h"
#include "Engine/SkeletalMeshActor.h"
#include "Actors/Cube.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Engine/AssetManager.h"

USkeletalSubEngine::USkeletalSubEngine()
{
}

USkeletalSubEngine::~USkeletalSubEngine()
{
}

void USkeletalSubEngine::Initialize(HWND& hWnd, FGraphicsDevice* InGraphics, FDXDBufferManager* InBufferManager, UImGuiManager* InSubWindow,
                                    UnrealEd* InUnrealEd)
{
    SubRenderer = new FSubRenderer;
    SubRenderer->Initialize(InGraphics, InBufferManager, this);

    Super::Initialize(hWnd, InGraphics, InBufferManager, InSubWindow, InUnrealEd);
    
    EditorPlayer = FObjectFactory::ConstructObject<AEditorPlayer>(this);
    EditorPlayer->SetCoordMode(CDM_LOCAL); 
    SkeletalMeshActor = FObjectFactory::ConstructObject<ASkeletalMeshActor>(this);

    BasePlane = FObjectFactory::ConstructObject<ACube>(this);
    BasePlane->SetActorScale(FVector(500,500,1));
    BasePlane->SetActorLocation(FVector(0,0,-1));
    SelectedBoneComponent = FObjectFactory::ConstructObject<USceneComponent>(this);

    UnrealSphereComponent = FObjectFactory::ConstructObject<UStaticMeshComponent>(this);
    UnrealSphereComponent->SetStaticMesh(UAssetManager::Get().GetStaticMesh(L"Contents/UnrealSphere.obj"));
    UnrealSphereComponent->SetRelativeScale3D(FVector(-10,-10,-10));
    UnrealSphereComponent->SetRelativeLocation(FVector(0,0,2000));
    UnrealSphereComponent->SetRelativeRotation(FRotator(0,0,180));
    
    SelectedActor = SkeletalMeshActor;
}

void USkeletalSubEngine::Tick(float DeltaTime)
{
    ViewportClient->Tick(DeltaTime);
    Input(DeltaTime);
    // EditorPlayer->Tick(DeltaTime);
    Render();    
}

void USkeletalSubEngine::Input(float DeltaTime)
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
        ViewportClient->CameraRotatePitch(DeltaY*0.1f);
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

void USkeletalSubEngine::Render()
{
    if (Wnd && IsWindowVisible(*Wnd) && Graphics->Device)
    {
        SubRenderer->PrepareRender(ViewportClient);
        SubRenderer->Render();
        SubRenderer->ClearRender();
        
        // Sub window rendering
        SubUI->BeginFrame();

        UnrealEditor->Render(EWindowType::WT_SkeletalSubWindow);
        
        SubUI->EndFrame();
        // Sub swap
        Graphics->SwapBuffer();
    }
}

void USkeletalSubEngine::Release()
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

void USkeletalSubEngine::SetSkeletalMesh(USkeletalMesh* InSkeletalMesh)
{
    OriginSkeletalMesh = InSkeletalMesh;
    if (SelectedSkeletalMesh != nullptr)
    {
        GUObjectArray.MarkRemoveObject(SelectedSkeletalMesh);
        SelectedSkeletalMesh = nullptr;
    }
    SelectedSkeletalMesh = Cast<USkeletalMesh>(InSkeletalMesh->Duplicate(this));
    SkeletalMeshActor->SetSkeletalMesh(SelectedSkeletalMesh);
    if (SubRenderer)
    {
        SubRenderer->SetPreviewSkeletalMesh(SelectedSkeletalMesh);
    }
    SelectedActor = SkeletalMeshActor;
    
    if (SubRenderer)
    {
        SubRenderer->SetPreviewSkeletalMeshComponent(SelectedActor->GetComponentByClass<USkeletalMeshComponent>());
    }
}

void USkeletalSubEngine::SaveSkeletalMesh()
{
    OriginSkeletalMesh->Skeleton->BoneTree = SelectedSkeletalMesh->Skeleton->BoneTree;
    OriginSkeletalMesh->Skeleton->BoneParentMap = SelectedSkeletalMesh->Skeleton->BoneParentMap;
    OriginSkeletalMesh->Skeleton->ReferenceSkeleton = SelectedSkeletalMesh->Skeleton->ReferenceSkeleton;
    OriginSkeletalMesh->Skeleton->LinkupCache = SelectedSkeletalMesh->Skeleton->LinkupCache;
    OriginSkeletalMesh->Skeleton->CurrentPose = SelectedSkeletalMesh->Skeleton->CurrentPose;
    OriginSkeletalMesh->Skeleton->CachedProcessingOrder = SelectedSkeletalMesh->Skeleton->CachedProcessingOrder;
}
