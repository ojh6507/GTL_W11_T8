#pragma once

#ifndef __ICON_FONT_INDEX__

#define __ICON_FONT_INDEX__
#define DEFAULT_FONT        0
#define FEATHER_FONT        1

#endif // !__ICON_FONT_INDEX__

enum EWindowType : uint8
{
    WT_Main,
    WT_SkeletalSubWindow,
    WT_AnimationSubWindow,
};

class UEditorPanel
{
public:
    virtual ~UEditorPanel() = default;
    virtual void Render() = 0;
    virtual void OnResize(HWND hWnd) = 0;

    EWindowType WindowType = WT_Main;
protected:
};
