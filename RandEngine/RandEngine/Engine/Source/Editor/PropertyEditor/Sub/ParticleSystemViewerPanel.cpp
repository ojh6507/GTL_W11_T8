#include "ParticleSystemViewerPanel.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "SubWindow/SkeletalSubEngine.h"
#include "SubWindow/SubEngine.h"
#include "Components/Mesh/SkeletalMesh.h"
#include "Engine/SkeletalMeshActor.h"
#include "PropertyEditor/ShowFlags.h"
#include "Renderer/RendererHelpers.h"
#include "Engine/UnrealClient.h"
#include "UnrealEd/EditorViewportClient.h"
// ParticleSystemViewerPanel.cpp


float ParticleSystemViewerPanel::LeftAreaTotalRatio = 0.7f;
float ParticleSystemViewerPanel::ViewportInLeftRatio = 0.6f;
int ParticleSystemViewerPanel::SelectedEmitterIndex = -1;
int ParticleSystemViewerPanel::SelectedModuleIndex = -1;

TArray<MyEmitterData> ParticleSystemViewerPanel::EmittersData = {
    MyEmitterData("Sparks", true, { MyModuleData("Spawn", 100.0f, ImVec4(1,0,0,1)), MyModuleData("Lifetime", 1.0f, ImVec4(0,1,0,1)) }),
    MyEmitterData("Smoke", true, { MyModuleData("Initial Location", 0.0f, ImVec4(0,0,1,1)), MyModuleData("Initial Velocity", 50.0f, ImVec4(1,1,0,1)) }),
    MyEmitterData("Rain", false, { MyModuleData("Gravity", 980.0f, ImVec4(0,1,1,1))})
};
// --- 메인 렌더링 함수 ---
void ParticleSystemViewerPanel::Render()
{
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(Width, Height)); // 클래스 멤버 Width, Height 사용

    ImGuiWindowFlags mainCanvasFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("CascadeModifiedCanvas", nullptr, mainCanvasFlags))
    {
        RenderMainLayout(ImGui::GetContentRegionAvail());
        ImGui::End();
    }
}
void ParticleSystemViewerPanel::PrepareRender(FEditorViewportClient* ViewportClient)
{
    const EViewModeIndex ViewMode = ViewportClient->GetViewMode();
    FViewportResource* ViewportResource = ViewportClient->GetViewportResource();
    RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_Scene);
    DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_Scene);

}

// --- 레이아웃 관리 함수 ---
void ParticleSystemViewerPanel::RenderMainLayout(const ImVec2& canvasContentSize)
{
    float splitterThickness = 5.0f;
    float minPanelSize = 50.0f;

    // 왼쪽 패널 너비 계산
    float leftPaneActualWidth = canvasContentSize.x * LeftAreaTotalRatio - splitterThickness / 2.0f;
    leftPaneActualWidth = FMath::Max(minPanelSize, leftPaneActualWidth);
    leftPaneActualWidth = FMath::Min(leftPaneActualWidth, canvasContentSize.x - splitterThickness - minPanelSize);

    // 왼쪽 패널 렌더링
    RenderLeftPane(ImVec2(leftPaneActualWidth, canvasContentSize.y), splitterThickness, minPanelSize);

    ImGui::SameLine(0, 0);

    // 수직 스플리터 렌더링 및 로직 처리
    RenderVerticalSplitter(canvasContentSize, splitterThickness);

    ImGui::SameLine(0, 0);

    // 오른쪽 패널 렌더링 (남은 공간 사용)
    float rightPaneWidth = ImGui::GetContentRegionAvail().x; // 남은 너비 자동 계산
    RenderRightPane(ImVec2(rightPaneWidth, canvasContentSize.y));
}

void ParticleSystemViewerPanel::RenderLeftPane(const ImVec2& panelSize, float splitterThickness, float minPanelSize)
{
    bool leftAreaContainerVisible = ImGui::BeginChild("LeftAreaContainer_Main", panelSize, ImGuiChildFlags_None);
    if (leftAreaContainerVisible)
    {
        ImVec2 leftAreaContentSize = ImGui::GetContentRegionAvail();

        // 뷰포트 패널 높이 계산
        float viewportActualHeight = leftAreaContentSize.y * ViewportInLeftRatio - splitterThickness / 2.0f;
        viewportActualHeight = FMath::Max(minPanelSize, viewportActualHeight);
        viewportActualHeight = FMath::Min(viewportActualHeight, leftAreaContentSize.y - splitterThickness - minPanelSize);

        ImTextureID texID = (ImTextureID)RenderTargetRHI->SRV;

        float origW = 1024;
        float origH = 1024;
        RenderViewportPanel(ImVec2(leftAreaContentSize.x, viewportActualHeight), texID, origW, origH);

        // 수평 스플리터 (왼쪽 패널 내부)
        RenderHorizontalSplitter(leftAreaContentSize, splitterThickness);

        // 속성 패널 (남은 공간 사용)
        MyEmitterData* currentEmitterPtr = nullptr;
        MyModuleData* currentModulePtr = nullptr;
        if (SelectedEmitterIndex != -1 && SelectedEmitterIndex < EmittersData.Num())
        {
            currentEmitterPtr = &EmittersData[SelectedEmitterIndex];
            if (SelectedModuleIndex != -1 && SelectedModuleIndex < currentEmitterPtr->modules.Num())
            {
                currentModulePtr = &currentEmitterPtr->modules[SelectedModuleIndex];
            }
        }
        // panelSize의 y를 0으로 주면 남은 공간을 모두 사용하도록 할 수 있지만, 여기서는 명시적으로 계산된 높이를 사용.
        // 만약 PropertiesPanel이 ViewportPanel 아래 남은 모든 공간을 차지하게 하려면
        // ImVec2(leftAreaContentSize.x, ImGui::GetContentRegionAvail().y) 를 사용.
        RenderPropertiesPanel(ImVec2(leftAreaContentSize.x, ImGui::GetContentRegionAvail().y), currentEmitterPtr, currentModulePtr);
    }
    ImGui::EndChild(); // LeftAreaContainer_Main
}

void ParticleSystemViewerPanel::RenderVerticalSplitter(const ImVec2& canvasContentSize, float splitterThickness)
{
    ImGui::InvisibleButton("##vsplitter", ImVec2(splitterThickness, canvasContentSize.y));
    if (ImGui::IsItemActive())
    {
        LeftAreaTotalRatio += ImGui::GetIO().MouseDelta.x / canvasContentSize.x;
        LeftAreaTotalRatio = FMath::Clamp(LeftAreaTotalRatio, 0.05f, 0.95f);
    }
    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW); }
}

void ParticleSystemViewerPanel::RenderHorizontalSplitter(const ImVec2& leftAreaContentSize, float splitterThickness)
{
    ImGui::InvisibleButton("##hsplitter", ImVec2(leftAreaContentSize.x, splitterThickness));
    if (ImGui::IsItemActive())
    {
        ViewportInLeftRatio += ImGui::GetIO().MouseDelta.y / leftAreaContentSize.y;
        ViewportInLeftRatio = FMath::Clamp(ViewportInLeftRatio, 0.05f, 0.95f);
    }
    if (ImGui::IsItemHovered()) { ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS); }
}

void ParticleSystemViewerPanel::RenderRightPane(const ImVec2& panelSize)
{
    // RenderEmitterStrip가 오른쪽 패널의 전체 내용을 그리도록 호출
    RenderEmitterStrip(panelSize, SelectedEmitterIndex, SelectedModuleIndex, EmittersData);
}


// --- 개별 UI 패널 렌더링 함수들 ---
void ParticleSystemViewerPanel::RenderViewportPanel(const ImVec2& panelSize, ImTextureID textureId, float originalImageWidth, float originalImageHeight)
{

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
    bool childVisible = ImGui::BeginChild("ViewportPanel_TopLeft", panelSize, ImGuiChildFlags_Border);
    if (childVisible)
    {
        if (textureId)
        {
            ImVec2 viewportContentSize = ImGui::GetContentRegionAvail();
            if (viewportContentSize.x > 0 && viewportContentSize.y > 0)
            {
                float viewportAspectRatio = viewportContentSize.x / viewportContentSize.y;
                float imageAspectRatio = originalImageWidth / originalImageHeight;
                ImVec2 uv0(0.0f, 0.0f), uv1(1.0f, 1.0f);

                if (imageAspectRatio > viewportAspectRatio) // 이미지가 뷰포트보다 넓으면, 너비에 맞추고 위아래 레터박스
                {
                    float uvWidth = viewportAspectRatio / imageAspectRatio;
                    uv0.x = (1.0f - uvWidth) * 0.5f;
                    uv1.x = uv0.x + uvWidth;
                }
                else if (imageAspectRatio < viewportAspectRatio) // 이미지가 뷰포트보다 좁으면 (길면), 높이에 맞추고 좌우 레터박스
                {
                    float uvHeight = imageAspectRatio / viewportAspectRatio;
                    uv0.y = (1.0f - uvHeight) * 0.5f;
                    uv1.y = uv0.y + uvHeight;
                }
                ImGui::Image(textureId, viewportContentSize, uv0, uv1);
            }
        }
        else
        {
            ImGui::Text("텍스처 ID 없음");
        }
    }
    ImGui::EndChild(); // ViewportPanel_TopLeft
    ImGui::PopStyleColor();
}

void ParticleSystemViewerPanel::RenderPropertiesPanel(const ImVec2& panelSize, const MyEmitterData* emitterPtr, MyModuleData* modulePtr)
{
    ImVec2 actualPanelSize = panelSize;
    if (actualPanelSize.y <= 0) { // 높이가 0 또는 음수이면 남은 공간 모두 사용
        actualPanelSize.y = ImGui::GetContentRegionAvail().y;
    }

    bool childVisible = ImGui::BeginChild("PropertiesPanel_BottomLeft", actualPanelSize, ImGuiChildFlags_Border);
    if (childVisible)
    {
        if (modulePtr)
        {
            ImGui::Text("모듈 속성: %s (이미터: %s)", modulePtr->name.c_str(), emitterPtr ? emitterPtr->name.c_str() : "N/A");
            ImGui::Separator();
            // modulePtr이 const MyModuleData* 이므로, 값을 변경하려면 const_cast 필요
            ImGui::InputFloat("Value Param 1", &const_cast<MyModuleData*>(modulePtr)->value_param1);
            ImGui::ColorEdit4("Color Param", &const_cast<MyModuleData*>(modulePtr)->color_param.x);
        }
        else if (emitterPtr)
        {
            ImGui::Text("이미터 속성: %s", emitterPtr->name.c_str());
            ImGui::Separator();
            ImGui::Checkbox("활성화", &const_cast<MyEmitterData*>(emitterPtr)->is_enabled);
        }
        else
        {
            ImGui::Text("선택된 항목 없음");
        }
    }
    ImGui::EndChild(); // PropertiesPanel_BottomLeft
}

void ParticleSystemViewerPanel::RenderEmitterStrip(const ImVec2& panelSize, int& currentSelectedEmitterIdx, int& currentSelectedModuleIdx, TArray<MyEmitterData>& localEmittersData)
{
    bool systemHierarchyVisible = ImGui::BeginChild("SystemHierarchyPanel_Right", panelSize, ImGuiChildFlags_None);
    if (systemHierarchyVisible)
    {
        if (ImGui::Button("+ 새 이미터 추가")) { localEmittersData.Add(MyEmitterData("새 이미터", true)); }
        ImGui::SameLine();
        if (ImGui::Button("선택 이미터 삭제") && currentSelectedEmitterIdx != -1 && currentSelectedEmitterIdx < localEmittersData.Num())
        {
            localEmittersData.RemoveAt(currentSelectedEmitterIdx);
            currentSelectedEmitterIdx = -1;
            currentSelectedModuleIdx = -1;
        }
        ImGui::Separator();

        ImVec2 emitterStripContentSize = ImGui::GetContentRegionAvail();
        ImGuiWindowFlags emitterStripFlags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));

        bool emitterStripVisible = ImGui::BeginChild("EmitterStrip", emitterStripContentSize, ImGuiChildFlags_None, emitterStripFlags);
        if (emitterStripVisible)
        {
            float fixedBlockWidth = 200.0f;
            float blockHeight = ImGui::GetContentRegionAvail().y;
            if (blockHeight < 50.0f) blockHeight = 50.0f; // 최소 높이 보장
            // 패딩을 고려한 실제 블록 높이 (필요시 조정)
            ImVec2 emitterBlockSize = ImVec2(fixedBlockWidth, blockHeight - ImGui::GetStyle().WindowPadding.y * 2.0f);
            if (emitterBlockSize.y < 50.0f) emitterBlockSize.y = 50.0f;


            for (int i = 0; i < localEmittersData.Num(); ++i)
            {
                if (i > 0) ImGui::SameLine();

                ImGui::PushID(i); // 각 이미터 블록에 고유 ID 부여

                bool isEmitterSelectedForFrame = (currentSelectedEmitterIdx == i && currentSelectedModuleIdx == -1);
                ImVec4 childBgColor = isEmitterSelectedForFrame ? ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered) : ImVec4(0.2f, 0.2f, 0.22f, 1.0f);
                ImU32 childBorderColorU32 = isEmitterSelectedForFrame ? ImGui::GetColorU32(ImVec4(0.9f, 0.9f, 0.2f, 1.0f)) : ImGui::GetColorU32(ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

                ImGui::PushStyleColor(ImGuiCol_ChildBg, childBgColor);
                ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(childBorderColorU32));
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));

                char child_id_str[64];
                snprintf(child_id_str, sizeof(child_id_str), "emitter_block_frame_%d", i);
                bool emitterBlockFrameVisible = ImGui::BeginChild(child_id_str, emitterBlockSize, ImGuiChildFlags_Border, ImGuiWindowFlags_None);
                if (emitterBlockFrameVisible)
                {
                    RenderEmitterBlockContents(i, localEmittersData[i], currentSelectedEmitterIdx, currentSelectedModuleIdx);
                }
                ImGui::EndChild(); // emitter_block_frame

                ImGui::PopStyleVar(2);
                ImGui::PopStyleColor(2);
                ImGui::PopID(); // PushID(i)에 대한 Pop
            }
        }
        ImGui::EndChild(); // EmitterStrip
        ImGui::PopStyleVar(); // WindowPadding
    }
    ImGui::EndChild(); // SystemHierarchyPanel_Right
}

void ParticleSystemViewerPanel::RenderEmitterBlockContents(int emitterIdx, MyEmitterData& emitterData, int& currentSelectedEmitterIdx, int& currentSelectedModuleIdx)
{
    ImVec2 nameStartPos = ImGui::GetCursorScreenPos();
    ImGui::Text("%s", emitterData.name.c_str());

    float checkboxWidth = ImGui::GetFrameHeight();
    // 체크박스를 이름 오른쪽에 정렬하기 위한 오프셋 계산
    // 현재 사용 가능한 너비에서 체크박스 너비와 약간의 간격을 뺌
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float desiredCheckboxX = ImGui::GetWindowContentRegionMax().x - checkboxWidth - ImGui::GetStyle().ItemInnerSpacing.x;
    float currentX = ImGui::GetCursorPosX(); // Text 다음에 오는 커서 X 위치

    if (currentX < desiredCheckboxX) {
        ImGui::SameLine(0.0f, desiredCheckboxX - currentX);
    }
    else { // 이름이 너무 길어서 공간이 없으면 그냥 옆에 붙임
        ImGui::SameLine();
    }

    ImGui::Checkbox("##Enable", &emitterData.is_enabled); // "##Enable"은 현재 ID 스택 내에서 고유 ID를 만듦

    ImVec2 checkboxEndPos = ImGui::GetItemRectMax();

    // 이름과 체크박스 영역 전체를 클릭 가능하게 만들기 위해 커서 위치 재설정
    ImGui::SetCursorScreenPos(nameStartPos);
    if (ImGui::InvisibleButton("##emitter_header_clickable", ImVec2(checkboxEndPos.x - nameStartPos.x, ImGui::GetTextLineHeightWithSpacing())))
    {
        currentSelectedEmitterIdx = emitterIdx;
        currentSelectedModuleIdx = -1;
    }
    ImGui::SetItemAllowOverlap();

    ImGui::Separator();
    ImGui::Text("모듈:");
    ImGui::Indent();
    for (int moduleIdxLoop = 0; moduleIdxLoop < emitterData.modules.Num(); ++moduleIdxLoop)
    {
        MyModuleData& module = emitterData.modules[moduleIdxLoop];
        ImGui::PushID(moduleIdxLoop); // 각 모듈에 대해 고유 ID 스택 생성

        bool isThisModuleSelected = (currentSelectedEmitterIdx == emitterIdx && currentSelectedModuleIdx == moduleIdxLoop);
        // Selectable의 레이블이 중복될 수 있으므로, PushID로 구분하거나 레이블 자체를 고유하게 만듦
        if (ImGui::Selectable(module.name.c_str(), isThisModuleSelected, ImGuiSelectableFlags_DontClosePopups))
        {
            currentSelectedEmitterIdx = emitterIdx;
            currentSelectedModuleIdx = moduleIdxLoop;
        }
        ImGui::PopID(); // moduleIdxLoop에 대한 Pop
    }
    ImGui::Unindent();
}

// --- 창 크기 변경 처리 ---
void ParticleSystemViewerPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    if (hWnd && GetClientRect(hWnd, &clientRect)) // hWnd 유효성 검사 추가
    {
        Width = static_cast<float>(clientRect.right - clientRect.left);
        Height = static_cast<float>(clientRect.bottom - clientRect.top);
    }

}
