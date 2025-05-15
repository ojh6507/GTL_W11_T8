#include "ViewerControlPanel.h"

#include "tinyfiledialogs.h"
#include "Engine/AssetManager.h"
#include "LevelEditor/SLevelEditor.h"
#include "PropertyEditor/ShowFlags.h"
#include "SubWindow/SkeletalSubEngine.h"
#include "SubWindow/SubEngine.h"
#include "UserInterface/Drawer.h"

class FDrawer;

void ViewerControlPanel::Render()
{
    /* Pre Setup */
    const ImGuiIO& IO = ImGui::GetIO();
    ImFont* IconFont = IO.Fonts->Fonts.size() == 1 ? IO.FontDefault : IO.Fonts->Fonts[FEATHER_FONT];
    constexpr ImVec2 IconSize = ImVec2(32, 32);
    ImVec2 WinSize = ImVec2(Width, Height);
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::SetNextWindowSize(ImVec2(Width, 50));
    /* Panel Flags */
    ImGuiWindowFlags PanelFlags =ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar;
    USkeletalSubEngine* SkeletalSubEngine = nullptr;
    if (WindowType == WT_SkeletalSubWindow)
        SkeletalSubEngine = Cast<USkeletalSubEngine>(GEngineLoop.SkeletalViewerSubEngine);
    else if ( WindowType == WT_AnimationSubWindow)
        SkeletalSubEngine = Cast<USkeletalSubEngine>(GEngineLoop.AnimationViewerSubEngine);
    /* Render Start */
    if (ImGui::Begin("Control Panel", nullptr, PanelFlags))
    {
        ImGui::PushFont(IconFont);


        if (ImGui::Button("\ue9d6",IconSize))
        {
            SkeletalSubEngine->SaveSkeletalMesh();
        }
        ImGui::SameLine();
        if (ImGui::Button("\ue950",IconSize))
        {
            if (WindowType == WT_SkeletalSubWindow)
                static_cast<FDrawer*>(GEngineLoop.GetUnrealEditor()->GetSubSkeletalPanel("Drawer").get())->Toggle();
            else if (WindowType == WT_AnimationSubWindow)
                static_cast<FDrawer*>(GEngineLoop.GetUnrealEditor()->GetSubAnimationPanel("Drawer").get())->Toggle();
        }
        ImGui::PopFont();

        ImGui::SameLine(0,Width *0.75);
        constexpr ImVec4 ActiveColor = ImVec4(0.00f, 0.0f, 0.40f, 1.0f);
        if (WindowType == WT_SkeletalSubWindow)
            ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
        if (ImGui::Button("SkeletalMesh Viewer",ImVec2(150,30)))
        {
            if (WindowType == WT_AnimationSubWindow)
            {
                GEngineLoop.bRePositionSkeletalWindow = true;
            }
        }
        if (WindowType == WT_SkeletalSubWindow)
            ImGui::PopStyleColor();

        ImGui::SameLine();
        if (WindowType == WT_AnimationSubWindow)
            ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
        if (ImGui::Button("Animation Viewer",ImVec2(150,30)))
        {
            if (WindowType == WT_SkeletalSubWindow)
            {
                GEngineLoop.bRepositionAnimWindow = true;
            }
        }
        if (WindowType == WT_AnimationSubWindow)
            ImGui::PopStyleColor();
        ImGui::End();
    }
    // if (ImGui::Begin("Control Panel", nullptr, PanelFlags))
    // {
    //     ImGui::SameLine();
    //     CreateFlagButton();
    //     ImGui::SameLine();
    //     /* Get Window Content Region */
    //     const float ContentWidth = ImGui::GetWindowContentRegionMax().x;
    //     /* Move Cursor X Position */
    //     if (Width >= 880.f)
    //     {
    //         ImGui::SetCursorPosX(ContentWidth - (IconSize.x * 3.0f + 16.0f));
    //     }
    //     ImGui::PushFont(IconFont);
    //     CreateSRTButton(IconSize);
    //     ImGui::PopFont();
    //     ImGui::End();
    // }
}

void ViewerControlPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = ClientRect.right - ClientRect.left;
    Height = ClientRect.bottom - ClientRect.top;
}

void ViewerControlPanel::CreateMenuButton(ImVec2 ButtonSize, ImFont* IconFont)
{
        if (ImGui::MenuItem("New Level"))
    {
        if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
        {
            EditorEngine->NewLevel();
        }
    }

    if (ImGui::MenuItem("Load Level"))
    {
        char const* lFilterPatterns[1] = { "*.scene" };
        const char* FileName = tinyfd_openFileDialog("Open Scene File", "", 1, lFilterPatterns, "Scene(.scene) file", 0);

        if (FileName == nullptr)
        {
            tinyfd_messageBox("Error", "파일을 불러올 수 없습니다.", "ok", "error", 1);
            return;
        }
        if (UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
        {
            EditorEngine->NewLevel();
            EditorEngine->LoadLevel(FileName);
        }
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Save Level"))
    {
        char const* lFilterPatterns[1] = { "*.scene" };
        const char* FileName = tinyfd_saveFileDialog("Save Scene File", "", 1, lFilterPatterns, "Scene(.scene) file");

        if (FileName == nullptr)
        {
            return;
        }
        if (const UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
        {
            EditorEngine->SaveLevel(FileName);
        }

        tinyfd_messageBox("알림", "저장되었습니다.", "ok", "info", 1);
    }

    ImGui::Separator();

    if (ImGui::BeginMenu("Import"))
    {
        if (ImGui::MenuItem("Wavefront (.obj)"))
        {
            char const* lFilterPatterns[1] = { "*.obj" };
            const char* FileName = tinyfd_openFileDialog("Open OBJ File", "", 1, lFilterPatterns, "Wavefront(.obj) file", 0);

            if (FileName != nullptr)
            {
                std::cout << FileName << '\n';

                if (UAssetManager::Get().GetStaticMesh(FileName) == nullptr)
                {
                    tinyfd_messageBox("Error", "파일을 불러올 수 없습니다.", "ok", "error", 1);
                }
            }
        }

        ImGui::EndMenu();
    }
    
    ImGui::Separator();

    if (ImGui::MenuItem("Quit"))
    {
        bOpenModal = true;
    }
}


void ViewerControlPanel::CreateFlagButton()
{
    const std::shared_ptr<FEditorViewportClient> ActiveViewport = GEngineLoop.GetLevelEditor()->GetActiveViewportClient();

    const char* ViewTypeNames[] = { "Perspective", "Top", "Bottom", "Left", "Right", "Front", "Back" };
    const ELevelViewportType ActiveViewType = ActiveViewport->GetViewportType();
    FString TextViewType = ViewTypeNames[ActiveViewType];

    if (ImGui::Button(GetData(TextViewType), ImVec2(120, 32)))
    {
        // toggleViewState = !toggleViewState;
        ImGui::OpenPopup("ViewControl");
    }

    if (ImGui::BeginPopup("ViewControl"))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ViewTypeNames); i++)
        {
            bool bIsSelected = (static_cast<int>(ActiveViewport->GetViewportType()) == i);
            if (ImGui::Selectable(ViewTypeNames[i], bIsSelected))
            {
                ActiveViewport->SetViewportType(static_cast<ELevelViewportType>(i));
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    const char* ViewModeNames[] = { 
        "Lit_Gouraud", "Lit_Lambert", "Lit_Blinn-Phong", "Lit_PBR",
        "Unlit", "Wireframe",
        "Scene Depth", "World Normal", "World Tangent","Light Heat Map"
    };
    constexpr uint32 ViewModeCount = std::size(ViewModeNames);
    
    const int RawViewMode = static_cast<int>(ActiveViewport->GetViewMode());
    const int SafeIndex = (RawViewMode >= 0) ? (RawViewMode % ViewModeCount) : 0;
    FString ViewModeControl = ViewModeNames[SafeIndex];
    
    const ImVec2 ViewModeTextSize = ImGui::CalcTextSize(GetData(ViewModeControl));
    if (ImGui::Button(GetData(ViewModeControl), ImVec2(30 + ViewModeTextSize.x, 32)))
    {
        ImGui::OpenPopup("ViewModeControl");
    }

    if (ImGui::BeginPopup("ViewModeControl"))
    {
        for (int i = 0; i < IM_ARRAYSIZE(ViewModeNames); i++)
        {
            bool bIsSelected = (static_cast<int>(ActiveViewport->GetViewMode()) == i);
            if (ImGui::Selectable(ViewModeNames[i], bIsSelected))
            {
                ActiveViewport->SetViewMode(static_cast<EViewModeIndex>(i));
            }

            if (bIsSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ShowFlags::GetInstance().Draw(ActiveViewport);
}

void ViewerControlPanel::CreateSRTButton(ImVec2 ButtonSize)
{
    const USubEngine* Engine = nullptr; 

    if (WindowType == WT_SkeletalSubWindow)
    {
        Engine = GEngineLoop.SkeletalViewerSubEngine;
    }
    else if (WindowType == WT_AnimationSubWindow)
    {
        Engine = GEngineLoop.AnimationViewerSubEngine;
    }
    AEditorPlayer* Player = Engine->EditorPlayer;

    constexpr ImVec4 ActiveColor = ImVec4(0.00f, 0.30f, 0.00f, 1.0f);

    const EControlMode ControlMode = Player->GetControlMode();

    if (ControlMode == CM_TRANSLATION)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9bc", ButtonSize)) // Move
    {
        Player->SetMode(CM_TRANSLATION);
    }
    if (ControlMode == CM_TRANSLATION)
    {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (ControlMode == CM_ROTATION)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9d3", ButtonSize)) // Rotate
    {
        Player->SetMode(CM_ROTATION);
    }
    if (ControlMode == CM_ROTATION)
    {
        ImGui::PopStyleColor();
    }
    ImGui::SameLine();
    if (ControlMode == CM_SCALE)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ActiveColor);
    }
    if (ImGui::Button("\ue9ab", ButtonSize)) // Scale
    {
        Player->SetMode(CM_SCALE);
    }
    if (ControlMode == CM_SCALE)
    {
        ImGui::PopStyleColor();
    }
}
