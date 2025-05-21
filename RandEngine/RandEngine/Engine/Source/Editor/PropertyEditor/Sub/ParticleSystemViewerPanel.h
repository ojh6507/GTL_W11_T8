#pragma once
#include "Components/ActorComponent.h"
#include "UnrealEd/EditorPanel.h"
#include "UnrealEd/EditorViewportClient.h"

struct FRenderTargetRHI;
struct FDepthStencilRHI;

class UParticleSystem;
class UParticleModule;
class UParticleEmitter;
class UParticleLODLevel;

class UParticleSystemSubEngine;

struct FDraggedModuleInfo
{
    int EmitterIndex = -1;
    int OriginalModuleIndex = -1;
    UParticleModule* ModulePtr = nullptr;

    void Reset() { EmitterIndex = -1; OriginalModuleIndex = -1; ModulePtr = nullptr; }
    bool IsValid() const { return ModulePtr != nullptr && EmitterIndex != -1 && OriginalModuleIndex != -1; }
};

class ParticleSystemViewerPanel : public UEditorPanel // UEditorPanel이 정의되어 있어야 함
{
public:
    ParticleSystemViewerPanel();
    void PrepareRender(FEditorViewportClient* ViewportClient);
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override; // HWND는 Windows 특정 타입

    void SetEditedParticleSystem(UParticleSystem* System);
public:
    UParticleSystem* CurrentEditedSystem;
private:
    // UI 상태 및 비율 변수
    static float LeftAreaTotalRatio;
    static float ViewportInLeftRatio;

    int SelectedEmitterIndex_Internal;
    int SelectedModuleIndex_Internal;
    FDraggedModuleInfo DraggedModuleInfo_Internal;

    void RenderMainLayout(const ImVec2& canvasContentSize);
    void RenderLeftPane(const ImVec2& panelSize, float splitterThickness, float minPanelSize);
    void RenderVerticalSplitter(const ImVec2& canvasContentSize, float splitterThickness);
    void RenderHorizontalSplitter(const ImVec2& leftAreaContentSize, float splitterThickness);
    void RenderRightPane(const ImVec2& panelSize);

    // 개별 UI 패널 렌더링 함수들
    void RenderViewportPanel(const ImVec2& panelSize, ImTextureID textureId, float originalImageWidth, float originalImageHeight);
    // RenderPropertiesPanel은 실제 UObject 포인터를 받도록 시그니처 변경
    void RenderPropertiesPanel(const ImVec2& panelSize, UParticleEmitter* SelectedEmitter, UParticleModule* SelectedModule);
    // RenderEmitterStrip은 UParticleSystem 포인터를 받거나 멤버 CurrentEditedSystem을 직접 사용
    void RenderEmitterStrip(const ImVec2& panelSize);
    
    void RenderEmitterHeader(int emitterIdx, UParticleEmitter* Emitter, UParticleLODLevel* LOD);
    
    
    // RenderEmitterBlockContents는 UParticleEmitter 참조/포인터를 받음
    void RenderEmitterBlockContents(int emitterIdx, UParticleEmitter* Emitter);

    // 이미터/모듈 추가/삭제를 위한 메뉴 핸들러 함수들
    void HandleAddEmitterMenu(); // AddEmitterStrip에서 이름 변경 및 기능 분리
    void HandleAddModuleMenu(UParticleEmitter* TargetEmitter); // AddModule에서 이름 변경 및 기능 분리
    void HandleDeleteSelectedEmitter();
    void HandleDeleteSelectedModule(UParticleEmitter* TargetEmitter);
    void RenderModuleEntry(UParticleModule* Module, const FString& DisplayName, int emitterIdx, int moduleIdentifier);
    // HWND 크기 (OnResize에서 설정)
    float Width = 800.0f;  // 적절한 초기값 설정
    float Height = 600.0f; // 적절한 초기값 설정
    FRenderTargetRHI* RenderTargetRHI;
    FDepthStencilRHI* DepthStencilRHI;
    UParticleSystemSubEngine* SubEngine;
};
