#pragma once
#include "GameFramework/Actor.h"
#include "UnrealEd/EditorPanel.h"

class ViewerControlPanel : public UEditorPanel
{
public:
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;
    void CreateMenuButton(ImVec2 ButtonSize, ImFont* IconFont);
    void CraeteViewerButton(ImVec2 ButtonSize);
    void CreateFlagButton();
    void CreateSRTButton(ImVec2 ButtonSize);
private:
    float Width = 300, Height = 100;
    bool bOpenModal = false;
    bool bShowImGuiDemoWindow = false; // 데모 창 표시 여부를 관리하는 변수
    float* FOV = nullptr;
    float CameraSpeed = 0.0f;
    float GridScale = 1.0f;
    ImVec4 PendingRect;
    bool bRepositionAnimWindow = false;
};
