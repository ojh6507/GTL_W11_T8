#include "SkeletalViewerPanel.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "SubWindow/SkeletalSubEngine.h"
#include "SubWindow/SubEngine.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Engine/SkeletalMeshActor.h"
#include "PropertyEditor/ShowFlags.h"

void SkeletalViewerPanel::Render()
{
    CreateFlagButton();
    ImVec2 WinSize = ImVec2(Width, Height);
    // ImGui::SetNextWindowPos(ImVec2(WinSize.x * 0.75f + 2.0f, 2));
    ImGui::SetNextWindowPos(ImVec2(0, 50));
    ImGui::SetNextWindowSize(ImVec2(WinSize.x * 0.2f - 5.0f, WinSize.y - 50));
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;

    ImGui::Begin("Skeletal Tree", nullptr, PanelFlags);

    CreateSkeletalTreeNode();

    ImGui::End();
    DetailPanel.Render(SkeletalMesh, SelectedBoneIdx);
}

void SkeletalViewerPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
    DetailPanel.OnResize(hWnd);
}

void SkeletalViewerPanel::CreateSkeletalTreeNode()
{
    USkeletalSubEngine* SubEngine = nullptr;
    if (WindowType == WT_SkeletalSubWindow)
        SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.SkeletalViewerSubEngine);
    else if (WindowType == WT_AnimationSubWindow)
        SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.AnimationViewerSubEngine);

    SkeletalMesh = SubEngine->SelectedSkeletalMesh;

    // if (Skeleton == Selected->Skeleton)
    //     return;
    // else

    const FReferenceSkeleton& RefSkeleton = SkeletalMesh->Skeleton->ReferenceSkeleton;
    const TArray<FBoneNode>& BoneNodes = RefSkeleton.BoneInfo;

    // 1. 뼈 계층 구조 분석
    TMap<int32, TArray<int32>> BoneHierarchy;
    for (int32 BoneIdx = 0; BoneIdx < BoneNodes.Num(); ++BoneIdx)
    {
        const int32 ParentIdx = BoneNodes[BoneIdx].ParentIndex;
        BoneHierarchy.FindOrAdd(ParentIdx).Add(BoneIdx);
    }

    // 2. 루트 본 찾기
    const TArray<int32>& RootBones = BoneHierarchy.FindOrAdd(INDEX_NONE);

    // 3. 계층 구조 렌더링
    for (int32 RootBoneIdx : RootBones)
    {
        RenderBoneHierarchy(RootBoneIdx, BoneNodes, BoneHierarchy);
    }
}

void SkeletalViewerPanel::CreateFlagButton()
{
    const ImGuiIO& IO = ImGui::GetIO();
    ImFont* IconFont = IO.Fonts->Fonts.size() == 1 ? IO.FontDefault : IO.Fonts->Fonts[FEATHER_FONT];
    constexpr ImVec2 IconSize = ImVec2(32, 32);
    ImVec2 WinSize = ImVec2(Width, Height);
    float treeWidth = WinSize.x * 0.2f - 5.0f;   // 트리 패널의 실제 너비
    float margin = 5.0f;                       // 트리와 버튼 사이 간격
    float panelX = treeWidth + margin;         // X 위치
    float panelY = 25.0f;

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(300,40), ImGuiCond_Always);

    constexpr ImVec2 MinSize(300, 72);
    constexpr ImVec2 MaxSize(FLT_MAX, 72);

    /* Min, Max Size */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

    /* Panel Flags */
    constexpr ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus;

    /* Render Start */
    if (!ImGui::Begin("Control Panel1", nullptr, PanelFlags))
    {
        ImGui::End();
        return;
    }

    const char* ViewModeNames[] = {
        //  "Lit_Gouraud", "Lit_Lambert", "Lit_Blinn-Phong", "Lit_PBR",
          "Unlit", "Wireframe",
          //"Scene Depth", "World Normal", "World Tangent","Light Heat Map"
    };

    USkeletalSubEngine* SubEngine = nullptr;
    if (WindowType == WT_SkeletalSubWindow)
        SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.SkeletalViewerSubEngine);
    else if (WindowType == WT_AnimationSubWindow)
        SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.AnimationViewerSubEngine);

    constexpr uint32 ViewModeCount = std::size(ViewModeNames);

    FString ViewModeControl = ViewModeNames[static_cast<int>(CurrentViewModeIndex) - 4];

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2, 0.2, 0.2, 1));
    
    const ImVec2 ViewModeTextSize = ImGui::CalcTextSize(GetData(ViewModeControl));
    if (ImGui::Button(GetData(ViewModeControl), ImVec2(30 + ViewModeTextSize.x, 32)))
    {
        ImGui::OpenPopup("ViewModeControl");
    }
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (ImGui::BeginPopup("ViewModeControl"))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ViewModeNames); i++)
        {
            bool bIsSelected = (static_cast<int>(CurrentViewModeIndex) - 4) == i;
            if (ImGui::Selectable(ViewModeNames[i], bIsSelected))
            {
                CurrentViewModeIndex = static_cast<EViewModeIndex>(i + 4);
                SubEngine->ViewportClient->SetViewMode(CurrentViewModeIndex);
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    
    
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2, 0.2, 0.2, 1));

    if (ImGui::Button("Show", ImVec2(60, 32)))
    {
        ImGui::OpenPopup("ShowFlags");
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    const char* Items[] = { "Bone" };
    const uint64 CurFlag = SubEngine->ViewportClient->GetShowFlag();

    if (ImGui::BeginPopup("ShowFlags"))
    {
        bool Selected[IM_ARRAYSIZE(Items)] =
        {
            static_cast<bool>(CurFlag & EEngineShowFlags::SF_Bone)
        }; // 각 항목의 체크 상태 저장

        for (int i = 0; i < IM_ARRAYSIZE(Items); i++)
        {
            ImGui::Checkbox(Items[i], &Selected[i]);
        }

        SubEngine->ViewportClient->SetShowFlag(ConvertSelectionToFlags(Selected));
        ImGui::EndPopup();
    }
    
    
    // ShowFlags::GetInstance().Draw(SubEngine->ViewportClient);
    ImGui::End();
}

// 재귀적 뼈 계층 구조 렌더링 함수
void SkeletalViewerPanel::RenderBoneHierarchy(
    int32 CurrentBoneIdx,
    const TArray<FBoneNode>& BoneNodes,
    const TMap<int32, TArray<int32>>& BoneHierarchy)
{
    const FBoneNode& CurrentBone = BoneNodes[CurrentBoneIdx];
    const FString BoneName = CurrentBone.Name.ToString().IsEmpty() ? TEXT("Unnamed Bone") : CurrentBone.Name.ToString();

    // selectable 트리 노드 플래그 설정
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow
        | ImGuiTreeNodeFlags_SpanAvailWidth
        | ImGuiTreeNodeFlags_Leaf * !BoneHierarchy.Contains(CurrentBoneIdx)
        | ImGuiTreeNodeFlags_Selected * (CurrentBoneIdx == SelectedBoneIdx)
        | ImGuiTreeNodeFlags_AllowItemOverlap
        | ImGuiTreeNodeFlags_DefaultOpen; // 원하는 경우 기본 열기

    // TreeNodeEx 호출 → 반환값이 열림 여부
    bool nodeOpen = ImGui::TreeNodeEx(
        (void*)(intptr_t)CurrentBoneIdx, // 고유 ID
        nodeFlags,
        "%s",
        *BoneName
    );

    // 클릭 시 선택 처리
    if (ImGui::IsItemClicked())
    {
        SelectedBoneIdx = CurrentBoneIdx;
        // 필요하다면, 이름도 저장
        if (WindowType == WT_SkeletalSubWindow)
        {
            USkeletalSubEngine* SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.SkeletalViewerSubEngine);
            SubEngine->SelectedComponent = SubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBoneIdx];
        }
        else if (WindowType == WT_AnimationSubWindow)
        {
            USkeletalSubEngine* SubEngine = static_cast<USkeletalSubEngine*>(GEngineLoop.AnimationViewerSubEngine);
            SubEngine->SelectedComponent = SubEngine->SkeletalMeshActor->BoneGizmoSceneComponents[SelectedBoneIdx];
        }
    }

    if (nodeOpen)
    {
        if (BoneHierarchy.Contains(CurrentBoneIdx))
        {
            for (int32 ChildIdx : BoneHierarchy[CurrentBoneIdx])
            {
                RenderBoneHierarchy(ChildIdx, BoneNodes, BoneHierarchy);
            }
        }
        ImGui::TreePop();
    }
}

uint64 SkeletalViewerPanel::ConvertSelectionToFlags(const bool Selected[])
{
    uint64 Flags = EEngineShowFlags::None;
    if (Selected[0])
    {
        Flags |= static_cast<uint64>(EEngineShowFlags::SF_Bone);
    }

    return Flags;
}
