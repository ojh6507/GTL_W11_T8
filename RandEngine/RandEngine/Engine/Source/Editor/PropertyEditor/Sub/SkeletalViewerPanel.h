#pragma once
#include "SkeletalDetailPanel.h"
#include "Components/ActorComponent.h"
#include "UnrealEd/EditorPanel.h"

struct FBoneNode;

class USkeleton;

class SkeletalViewerPanel : public UEditorPanel
{
public:
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override;

private:
    USkeletalMesh* SkeletalMesh =nullptr;

    void CreateSkeletalTreeNode();
    void CreateFlagButton();
    void RenderBoneHierarchy(int32 CurrentBoneIdx, const TArray<FBoneNode>& BoneNodes, const TMap<int32, TArray<int32>>& BoneHierarchy);
    uint64 ConvertSelectionToFlags(const bool Selected[]);

    SkeletalDetailPanel DetailPanel;
    int32 SelectedBoneIdx = 0;
    EViewModeIndex CurrentViewModeIndex = EViewModeIndex::VMI_Unlit;
private:
    float Width = 800, Height = 600;
};
