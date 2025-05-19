#include "Drawer.h"

// #include "Engine/FbxLoader.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "ImGui/imgui_internal.h"
#include "SubWindow/AnimationSubEngine.h"
#include "SubWindow/SkeletalSubEngine.h"
#include "SubWindow/ParticleSystemSubEngine.h"
#include "SubWindow/SubEngine.h"
#include "SubWindow/SubRenderer.h"
#include "UObject/UObjectIterator.h"

 #include "Particle/ParticleSystem.h"

void FDrawer::Toggle()
{
    bIsOpen = !bIsOpen;
    if (!bFirstOpenFrame)
    {
        bFirstOpenFrame = true;
    }
}

void FDrawer::Render()
{
}

void FDrawer::Render(float DeltaTime)
{
    if (!bIsOpen)
        return;

    ImVec2 WinSize = ImVec2(Width, Height);

    // 목표 위치
    float TargetY = WinSize.y * 0.75f;
    float StartY = WinSize.y; // 아래에서 올라오게
    float CurrentY = TargetY;

    if (bFirstOpenFrame)
    {
        AnimationTime = 0.0f;
        bFirstOpenFrame = false;
    }

    // 애니메이션 진행
    if (AnimationTime < AnimationDuration)
    {
        AnimationTime += DeltaTime;
        float T = AnimationTime / AnimationDuration;
        T = ImClamp(T, 0.0f, 1.0f);
        T = ImGui::GetStyle().Alpha * T; // 곡선 보간을 원한다면 여기서 ease 적용
        CurrentY = ImLerp(StartY, TargetY, T);
    }

    ImGui::SetNextWindowPos(ImVec2(5, CurrentY));
    ImGui::SetNextWindowSize(ImVec2(WinSize.x - 10.0f, WinSize.y * 0.25f));
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    ImGui::Begin("Content Drawer", nullptr, PanelFlags);
    if (ImGui::BeginTabBar("DetailsTabBar"))
    {
        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.05f, 0.05f, 0.08f, 0.80f));         // 비활성 탭
        ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.35f, 0.35f, 0.40f, 1.00f));  // 호버 탭
        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.20f, 0.20f, 0.25f, 1.00f));   // 활성 탭

        if (ImGui::BeginTabItem("Skeletal Mesh"))
        {
            RenderSkeletalMeshContentDrawer();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Particle System"))
        {
            RenderParticleSytemContentDrawer();
            ImGui::EndTabItem();
        }
        ImGui::PopStyleColor(3);
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void FDrawer::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}

void FDrawer::RenderSkeletalMeshContentDrawer()
{
    for (auto Obj : TObjectRange<USkeletalMesh>())
    {
        if (Obj->GetOuter() != nullptr)
            continue;
        ImGui::Selectable(GetData(Obj->GetRenderData()->FilePath));

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            UE_LOG(ELogLevel::Display, TEXT("Double Clicked"));
            static_cast<USkeletalSubEngine*>(GEngineLoop.SkeletalViewerSubEngine)->SetSkeletalMesh(Obj);
            GEngineLoop.SkeletalViewerSubEngine->RequestShowWindow(true);

            static_cast<UAnimationSubEngine*>(GEngineLoop.AnimationViewerSubEngine)->SetSkeletalMesh(Obj);
            GEngineLoop.AnimationViewerSubEngine->RequestShowWindow(true);

            break;
        }
    }
}

void FDrawer::RenderParticleSytemContentDrawer()
{
    // 1. "새 파티클 시스템" 생성 컨텍스트 메뉴
    if (ImGui::BeginPopupContextWindow("ParticleSystemContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::Selectable("Create New Particle System"))
        {
            UParticleSystem* NewSystem = FObjectFactory::ConstructObject<UParticleSystem>(nullptr);
            if (NewSystem)
            {
                NewSystem->InitializeSystem(); 
            }
        }
        ImGui::EndPopup();
    }


    for (UParticleSystem* PSA : TObjectRange<UParticleSystem>()) // 모든 UParticleSystem 객체 순회
    {
        if (!PSA) 
            continue;

        FString AssetName = PSA->GetFName().ToString();
        ImGui::Selectable(AssetName.ToAnsiString().c_str());

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (GEngineLoop.ParticleSystemViewerSubEngine)
            {
                static_cast<UParticleSystemSubEngine*>(GEngineLoop.ParticleSystemViewerSubEngine)->OpenParticleSystemForEditing(PSA);
                GEngineLoop.ParticleSystemViewerSubEngine->RequestShowWindow(true);
              
            }
        }
    }
    //// 만약 특정 경로의 에셋만 표시하고 싶다면, 에셋 매니저를 통해 해당 경로의 에셋 목록을 가져와야 합니다.
    //// 예: TArray<FAssetData> ParticleSystemAssets;
    ////     UAssetManager::Get().GetAssetsByPath(TEXT("/Game/Particles"), ParticleSystemAssets, true);
    ////     for (const FAssetData& AssetData : ParticleSystemAssets) {
    ////         if (AssetData.AssetClassPath == UParticleSystem::StaticClass()->GetClassPathName()) { // 클래스 경로 이름으로 비교
    ////             UParticleSystem* PSA = Cast<UParticleSystem>(AssetData.GetAsset());
    ////             if (PSA) { /* ImGui::Selectable 등 */ }
    ////         }
    ////     }
}
