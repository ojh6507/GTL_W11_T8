#include "UnrealEd.h"
#include "EditorPanel.h"

#include "PropertyEditor/ControlEditorPanel.h"
#include "PropertyEditor/OutlinerEditorPanel.h"
#include "PropertyEditor/PropertyEditorPanel.h"
#include "PropertyEditor/Sub/SkeletalViewerPanel.h"
#include "PropertyEditor/Sub/AnimationTimelinePanel.h"
#include "PropertyEditor/Sub/ViewerControlPanel.h"
#include "UserInterface/Drawer.h"

void UnrealEd::Initialize()
{
    auto ControlPanel = std::make_shared<ControlEditorPanel>();
    AddEditorPanel("ControlPanel", ControlPanel);

    auto OutlinerPanel = std::make_shared<OutlinerEditorPanel>();
    AddEditorPanel("OutlinerPanel", OutlinerPanel);

    auto PropertyPanel = std::make_shared<PropertyEditorPanel>();
    AddEditorPanel("PropertyPanel", PropertyPanel);

    auto Drawer = std::make_shared<FDrawer>();
    AddEditorPanel("Drawer", Drawer);
    
    auto SubSkeletalViewerPanel = std::make_shared<SkeletalViewerPanel>();
    SubSkeletalViewerPanel->WindowType = WT_SkeletalSubWindow;
    AddEditorPanel("SubSkeletalViewerPanel", SubSkeletalViewerPanel, EWindowType::WT_SkeletalSubWindow);
    auto SubSkeletalViewerControlPanel = std::make_shared<ViewerControlPanel>();
    SubSkeletalViewerControlPanel->WindowType = WT_SkeletalSubWindow;
    AddEditorPanel("SubSkeletalViewerControlPanel", SubSkeletalViewerControlPanel, EWindowType::WT_SkeletalSubWindow);
    auto SkeletalDrawer = std::make_shared<FDrawer>();
    SkeletalDrawer->WindowType = WT_SkeletalSubWindow;
    AddEditorPanel("Drawer", SkeletalDrawer, EWindowType::WT_SkeletalSubWindow);
    
    auto SubAnimationViewerPanel = std::make_shared<SAnimationTimelinePanel>();
    SubAnimationViewerPanel->WindowType = WT_AnimationSubWindow;
    auto SubSkeletalViewerPanel2 = std::make_shared<SkeletalViewerPanel>();
    SubSkeletalViewerPanel2->WindowType = WT_AnimationSubWindow;
    auto SubAnimationViewerControlPanel = std::make_shared<ViewerControlPanel>();
    SubAnimationViewerControlPanel->WindowType = WT_AnimationSubWindow;
    AddEditorPanel("SubAnimationViewerControlPanel", SubAnimationViewerControlPanel, EWindowType::WT_AnimationSubWindow);
    AddEditorPanel("SubSkeletalViewerPanel", SubSkeletalViewerPanel2, EWindowType::WT_AnimationSubWindow);
    AddEditorPanel("SubAnimationViewerPanel", SubAnimationViewerPanel, EWindowType::WT_AnimationSubWindow);
    auto AnimationDrawer = std::make_shared<FDrawer>();
    AnimationDrawer->WindowType = WT_AnimationSubWindow;
    AddEditorPanel("Drawer", AnimationDrawer, EWindowType::WT_AnimationSubWindow);
}

void UnrealEd::Render(EWindowType WindowType) const
{
    TMap<FString, std::shared_ptr<UEditorPanel>> TargetPanels;

    switch (WindowType)
    {
    case EWindowType::WT_Main:
        TargetPanels = Panels;
        break;
    case EWindowType::WT_SkeletalSubWindow:
        TargetPanels = SkeletalSubPanels;
        break;
    case EWindowType::WT_AnimationSubWindow:
        TargetPanels = AnimationSubPanels;
        break;
    }

    for (const auto& Panel : TargetPanels)
    {
        Panel.Value->Render();
        if (Panel.Key == "Drawer")
            static_cast<FDrawer*>(Panel.Value.get())->Render(GEngineLoop.GElapsedTime);
    }
}

void UnrealEd::AddEditorPanel(const FString& PanelId, const std::shared_ptr<UEditorPanel>& EditorPanel, EWindowType WindowType)
{
    switch (WindowType)
    {
    case EWindowType::WT_Main:
        Panels[PanelId] = EditorPanel;
        break;
    case EWindowType::WT_SkeletalSubWindow:
        SkeletalSubPanels[PanelId] = EditorPanel;
        break;
    case EWindowType::WT_AnimationSubWindow:
        AnimationSubPanels[PanelId] = EditorPanel;
        break;
    default:
        break;
    }

}

void UnrealEd::OnResize(HWND hWnd, EWindowType WindowType) const
{
    
    TMap<FString, std::shared_ptr<UEditorPanel>> TargetPanels;

    switch (WindowType)
    {
    case EWindowType::WT_Main:
        TargetPanels = Panels;
        break;
    case EWindowType::WT_SkeletalSubWindow:
        TargetPanels = SkeletalSubPanels;
        break;
    case EWindowType::WT_AnimationSubWindow:
        TargetPanels = AnimationSubPanels;
        break;
    }

    for (auto& PanelPair : TargetPanels)
    {
        if (PanelPair.Value)
        {
            PanelPair.Value->OnResize(hWnd);
        }
    }
}

std::shared_ptr<UEditorPanel> UnrealEd::GetEditorPanel(const FString& PanelId)
{
    return Panels[PanelId];
}


std::shared_ptr<UEditorPanel> UnrealEd::GetSubSkeletalPanel(const FString& PanelId)
{
    return SkeletalSubPanels[PanelId];
}

std::shared_ptr<UEditorPanel> UnrealEd::GetSubAnimationPanel(const FString& PanelId)
{
    return AnimationSubPanels[PanelId];
}
