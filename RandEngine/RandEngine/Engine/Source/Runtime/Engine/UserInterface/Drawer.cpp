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
#include "Engine/AssetManager.h"

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
    if (ImGui::BeginPopupContextWindow("ParticleSystemList_ContextMenu", ImGuiPopupFlags_MouseButtonRight /*| ImGuiPopupFlags_NoOpenOverItems*/))
    {
        if (ImGui::Selectable("Create New Particle System..."))
        {
            bShowCreateParticleSystemPopup = true; // 이름 입력 팝업을 띄우도록 플래그 설정
            memset(NewParticleSystemNameBuffer, 0, sizeof(NewParticleSystemNameBuffer)); // 입력 버퍼 초기화
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 새 파티클 시스템 이름 입력 팝업 모달
    if (bShowCreateParticleSystemPopup)
    {
        // 모달을 화면 중앙에 위치시키도록 설정 (처음 나타날 때만)
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::OpenPopup("Create New Particle System"); // 팝업 열기 요청
    }

    // BeginPopupModal은 OpenPopup이 호출된 프레임부터 true를 반환하기 시작합니다.
    // &GbShowCreateParticleSystemPopup을 통해 팝업의 X 버튼이나 ESC로 닫힐 때 플래그가 false로 설정됩니다.
    if (ImGui::BeginPopupModal("Create New Particle System", &bShowCreateParticleSystemPopup, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Enter a name for the new Particle System:");
        ImGui::InputText("##ParticleSystemNameInput", NewParticleSystemNameBuffer, IM_ARRAYSIZE(NewParticleSystemNameBuffer));
        ImGui::Separator();

        if (ImGui::Button("Create", ImVec2(120, 0)))
        {
            if (strlen(NewParticleSystemNameBuffer) > 0)
            {
                FString AssetNameString = FString(NewParticleSystemNameBuffer);
                FName AssetFName(*AssetNameString);

                // Outer를 nullptr로 하면 TransientPackage에 생성됩니다.
                // 실제 에셋으로 저장하려면 UPackage를 생성하고 Outer로 지정해야 합니다.
                UParticleSystem* NewSystem = FObjectFactory::ConstructObject<UParticleSystem>(nullptr);

                if (NewSystem)
                {
                    NewSystem->InitializeSystem();
                    NewSystem->ParticleSystemFileName = AssetNameString;
                    NewSystem->SaveParticleSystemToBinary();
                   
                    FString SaveFilePath = FString(TEXT("Contents/Particle/")) + NewSystem->ParticleSystemFileName +".myparticle";
                    UAssetManager::Get().RegisterNewlyCreatedParticleSystem(SaveFilePath, NewSystem);
                }
                else
                {
                    UE_LOG(ELogLevel::Error, TEXT("Failed to create Particle System with name: %s. It might already exist or be invalid."), *AssetNameString);
                }
                bShowCreateParticleSystemPopup = false;
                ImGui::CloseCurrentPopup();
            }
            else
                ImGui::TextColored;
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            bShowCreateParticleSystemPopup = false; // 팝업 닫기
            ImGui::CloseCurrentPopup(); // 모달 닫기
        }
        ImGui::EndPopup();
    }


    // 기존 파티클 시스템 목록 표시
    for (UParticleSystem* PSA : TObjectRange<UParticleSystem>())
    {
        if (!PSA)
        {
            continue;
        }
        ImGui::PushID(PSA);
        FString AssetName = PSA->ParticleSystemFileName;
        ImGui::Selectable(GetData(AssetName));
        ImGui::PopID();
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (GEngineLoop.ParticleSystemViewerSubEngine)
            {
                static_cast<UParticleSystemSubEngine*>(GEngineLoop.ParticleSystemViewerSubEngine)->OpenParticleSystemForEditing(PSA);
                GEngineLoop.ParticleSystemViewerSubEngine->RequestShowWindow(true);
            }
        }
    }
}
