#pragma once
#include "Components/ActorComponent.h"
#include "UnrealEd/EditorPanel.h"
#include "UnrealEd/EditorViewportClient.h"

// 예시 이미터 데이터 구조체
struct MyModuleData {
    std::string name;
    // ... 모듈 타입별 다양한 속성들 ...
    float value_param1;
    ImVec4 color_param;
    // 어떤 타입의 모듈인지 식별자 추가 가능 (예: enum ModuleType)

    MyModuleData(std::string n, float v1, ImVec4 c) : name(n), value_param1(v1), color_param(c) {}
};
struct MyEmitterData {
    std::string name;
    bool is_enabled;
    TArray<MyModuleData> modules;
    // ... 기타 이미터 속성 ...
};
struct FRenderTargetRHI;
struct FDepthStencilRHI;

class ParticleSystemViewerPanel : public UEditorPanel // UEditorPanel이 정의되어 있어야 함
{
public:
    void PrepareRender(FEditorViewportClient* ViewportClient);
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override; // HWND는 Windows 특정 타입

private:
    // UI 상태 및 비율 변수
    static float LeftAreaTotalRatio;
    static float ViewportInLeftRatio;
    static int SelectedEmitterIndex;
    static int SelectedModuleIndex;
    static TArray<MyEmitterData> EmittersData; // 데이터

    // 레이아웃 관리 헬퍼 함수
    void RenderMainLayout(const ImVec2& canvasContentSize);
    void RenderLeftPane(const ImVec2& panelSize, float splitterThickness, float minPanelSize);
    void RenderVerticalSplitter(const ImVec2& canvasContentSize, float splitterThickness);
    void RenderHorizontalSplitter(const ImVec2& leftAreaContentSize, float splitterThickness);
    void RenderRightPane(const ImVec2& panelSize);

    // 기존 패널 렌더링 함수들 (static 유지)
    void RenderViewportPanel(const ImVec2& panelSize, ImTextureID textureId, float originalImageWidth, float originalImageHeight);
    void RenderPropertiesPanel(const ImVec2& panelSize, const MyEmitterData* emitterPtr, MyModuleData* modulePtr);
    void RenderEmitterBlockContents(int emitterIdx, MyEmitterData& emitterData, int& currentSelectedEmitterIdx, int& currentSelectedModuleIdx);
    void RenderEmitterStrip(const ImVec2& panelSize, int& currentSelectedEmitterIdx, int& currentSelectedModuleIdx, TArray<MyEmitterData>& localEmittersData);
    void AddEmitterStrip(TArray<MyEmitterData>& localEmittersData);
    void AddModule(TArray<MyEmitterData>& localEmittersData);
    // HWND 크기 (OnResize에서 설정)
    float Width = 800.0f;  // 적절한 초기값 설정
    float Height = 600.0f; // 적절한 초기값 설정
    FRenderTargetRHI* RenderTargetRHI;
    FDepthStencilRHI* DepthStencilRHI;
};
