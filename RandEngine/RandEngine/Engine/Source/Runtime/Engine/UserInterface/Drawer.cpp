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
            UParticleSystem* NewSystem = FObjectFactory::ConstructObject<UParticleSystem>(nullptr); // 임시 Outer 사용
            if (NewSystem)
            {
                // 기본 이미터 추가 (헬퍼 함수 사용)
                // UParticleEmitter* DefaultEmitter = CreateDefaultEmitterForSystem(NewSystem, EParticleType::Sprite);
                // if (DefaultEmitter) NewSystem->Emitters.Add(DefaultEmitter);

                NewSystem->InitializeSystem(); // 초기 빌드

                // (선택적) 바로 저장 UI를 띄우거나, 임시 에셋으로 두고 나중에 저장하도록 유도
                // SaveParticleSystemAsset(NewSystem);

                // 생성된 시스템을 뷰어에서 열도록 요청
                if (GEngineLoop.ParticleSystemViewerSubEngine)
                {
                    static_cast<UParticleSystemSubEngine*>(GEngineLoop.ParticleSystemViewerSubEngine)->OpenParticleSystemForEditing(NewSystem);
                    GEngineLoop.ParticleSystemViewerSubEngine->RequestShowWindow(true);
                }
            }
        }
        ImGui::EndPopup();
    }

    //// 2. 기존 파티클 시스템 에셋 목록 표시 (TObjectRange 또는 에셋 매니저 사용)
    //// 여기서는 TObjectRange를 사용한다고 가정 (실제로는 에셋 레지스트리나 특정 경로 스캔 방식이 더 일반적)
    //ImGui::Text("Existing Particle Systems:");
    //for (UParticleSystem* PSA : TObjectIterator<UParticleSystem>()) // 모든 UParticleSystem 객체 순회
    //{
    //    if (!PSA || PSA->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) // CDO나 Archetype은 제외
    //        continue;

    //    // 에셋 경로 또는 이름을 가져와서 표시 (사용자 시스템의 방식에 따라)
    //    // FString AssetName = PSA->GetPathName(); // 또는 PSA->GetName()
    //    FString AssetName = PSA->GetFName().ToString(); // FName 사용 시
    //    if (ImGui::Selectable(AssetName.ToAnsiString().c_str())) // FString -> const char* 변환
    //    {
    //        // 선택 시 아무것도 안 함 (더블클릭으로 열기)
    //    }

    //    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    //    {
    //        if (GEngineLoop.ParticleSystemViewerSubEngine)
    //        {
    //            static_cast<UParticleSystemSubEngine*>(GEngineLoop.ParticleSystemViewerSubEngine)->OpenParticleSystemForEditing(PSA);
    //            GEngineLoop.ParticleSystemViewerSubEngine->RequestShowWindow(true);
    //            // 더블 클릭된 에셋이 ParticleSystemViewerPanel의 CurrentEditedSystem으로 설정되어야 함
    //            // ParticleSystemViewerPanel* Panel = static_cast<ParticleSystemViewerPanel*>(GEngineLoop.ParticleSystemViewerSubEngine->GetEditorPanel());
    //            // if (Panel) Panel->SetEditedParticleSystem(PSA);
    //        }
    //    }
    //}
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
