#include "ParticleSystemViewerPanel.h"

#include "SubWindow/SubEngine.h"
#include "PropertyEditor/ShowFlags.h"
#include "Renderer/RendererHelpers.h"
#include "Engine/UnrealClient.h"
#include "UnrealEd/EditorViewportClient.h"

#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"
#include "Particle/ParticleModule.h"
#include "Particle/ParticleModuleRequired.h"

#include "Particle/ParticleModuleTypeDataSprite.h"
#include "Particle/ParticleModuleTypeDataMesh.h"

#include "Particle/ParticleModuleSpawn.h"
#include "Particle/ParticleModuleLifetime.h"

float ParticleSystemViewerPanel::LeftAreaTotalRatio = 0.7f;
float ParticleSystemViewerPanel::ViewportInLeftRatio = 0.6f;


ParticleSystemViewerPanel::ParticleSystemViewerPanel()
    : CurrentEditedSystem(nullptr)
    , SelectedEmitterIndex_Internal(-1)
    , SelectedModuleIndex_Internal(-1)
    , Width(800.0f), Height(600.0f) // 생성자에서 초기화
    , RenderTargetRHI(nullptr), DepthStencilRHI(nullptr)
{
    // 테스트용 기본 파티클 시스템 생성 (실제로는 파일에서 로드하거나 "새로 만들기" 메뉴로 생성)
    CreateNewTestParticleSystem();
}

void ParticleSystemViewerPanel::SetEditedParticleSystem(UParticleSystem* System)
{
    CurrentEditedSystem = System;
    if (CurrentEditedSystem)
    {
        CurrentEditedSystem->InitializeSystem(); // 로드/설정 시 빌드
    }
    SelectedEmitterIndex_Internal = -1;
    SelectedModuleIndex_Internal = -1;
}

// 테스트용 함수
void ParticleSystemViewerPanel::CreateNewTestParticleSystem()
{
    //CurrentEditedSystem = FObjectFactory::ConstructObject<UParticleSystem>(nullptr); // 적절한 Outer 제공
    //if (CurrentEditedSystem)
    //{
    //    // 기본 스프라이트 이미터 추가
    //    UParticleEmitter* Emitter = FObjectFactory::ConstructObject<UParticleEmitter>(CurrentEditedSystem);
    //    Emitter->EmitterName = FString(TEXT("DefaultSpriteEmitter"));

    //    UParticleLODLevel* LOD = FObjectFactory::ConstructObject<UParticleLODLevel>(Emitter);
    //    LOD->RequiredModule = FObjectFactory::ConstructObject<UParticleModuleRequired>(LOD);
    //    LOD->TypeDataModule = FObjectFactory::ConstructObject<UParticleModuleTypeDataSprite>(LOD); // 기본은 스프라이트
    //    LOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleSpawn>(LOD));
    //    LOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleLifetime>(LOD));
    //    Emitter->LODLevels.Add(LOD);
    //    CurrentEditedSystem->Emitters.Add(Emitter);

    //    CurrentEditedSystem->InitializeSystem();
    //}
}

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


        TArray< UParticleEmitter*>& CurrentEditedEmitters = CurrentEditedSystem->Emitters;
        UParticleEmitter* currentEmitter = nullptr;
        UParticleModule* currentModule = nullptr;

        if (CurrentEditedSystem && SelectedEmitterIndex_Internal != -1 && 
            CurrentEditedSystem->Emitters.IsValidIndex(SelectedEmitterIndex_Internal))
        {
            currentEmitter = CurrentEditedSystem->Emitters[SelectedEmitterIndex_Internal];

            if (currentEmitter && !currentEmitter->LODLevels.IsEmpty() && currentEmitter->LODLevels[0])
            {
                UParticleLODLevel* lod = currentEmitter->LODLevels[0];
                if (SelectedModuleIndex_Internal != -1 && lod->Modules.IsValidIndex(SelectedModuleIndex_Internal))
                {
                    currentModule = lod->Modules[SelectedModuleIndex_Internal];
                }
                // 또는 RequiredModule, TypeDataModule도 선택 대상이 될 수 있음 (UI 로직 추가 필요)
            }
        }
        RenderPropertiesPanel(ImVec2(leftAreaContentSize.x, ImGui::GetContentRegionAvail().y), currentEmitter, currentModule);
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
    RenderEmitterStrip(panelSize);
}


// --- 개별 UI 패널 렌더링 함수들 ---
void ParticleSystemViewerPanel::RenderViewportPanel(const ImVec2& panelSize, ImTextureID textureId, float originalImageWidth, float originalImageHeight)
{

    ImGui::BeginTabBar("##Viewport");
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.05f, 0.05f, 0.08f, 0.80f));         // 비활성 탭
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));   // 활성 탭

    ImGui::BeginTabItem("Viewport");
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
    ImGui::PopStyleColor(3);
    ImGui::EndTabItem();
    ImGui::EndTabBar();
}

void ParticleSystemViewerPanel::RenderPropertiesPanel(const ImVec2& panelSize, UParticleEmitter* SelectedEmitter, UParticleModule* DefaultSelectedModule) {
    ImVec2 actualPanelSize = panelSize;
    if (actualPanelSize.y <= 0) 
    { 
        // 높이가 0 또는 음수이면 남은 공간 모두 사용
        actualPanelSize.y = ImGui::GetContentRegionAvail().y;
    }

    bool childVisible = ImGui::BeginChild("PropertiesPanel_BottomLeft", actualPanelSize, ImGuiChildFlags_Border);
    if (childVisible)
    {
        ImGui::BeginTabBar("##Details");
        ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.05f, 0.05f, 0.08f, 0.80f));         // 비활성 탭
        ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.14f, 0.14f, 0.14f, 1.00f));   // 활성 탭

        ImGui::BeginTabItem("Details");

        const int REQUIRED_MODULE_INDEX = -2;
        const int TYPEDATA_MODULE_INDEX = -3;
        UParticleModule* ActualSelectedModule = DefaultSelectedModule;

        if (SelectedEmitter && SelectedEmitter->GetHighestLODLevel()) 
        {
            if (SelectedModuleIndex_Internal == REQUIRED_MODULE_INDEX) 
            {
                ActualSelectedModule = SelectedEmitter->GetHighestLODLevel()->RequiredModule;
            }
            else if (SelectedModuleIndex_Internal == TYPEDATA_MODULE_INDEX) {
                ActualSelectedModule = SelectedEmitter->GetHighestLODLevel()->TypeDataModule;
            }
        }

        if (ActualSelectedModule)
        {
            FString moduleDisplayName = ActualSelectedModule->GetModuleDisplayName();
            ImGui::Text("Module: %s", moduleDisplayName.ToAnsiString().c_str());
            ImGui::Separator();

            bool bPropertyChanged = false;
            if (ImGui::Checkbox("Enabled", &ActualSelectedModule->bEnabled))
            {
                bPropertyChanged = true;
            }
            ImGui::Separator();

            if (UParticleModuleSpawn* SpawnModule = Cast<UParticleModuleSpawn>(ActualSelectedModule))
            {
                if (ImGui::DragFloat("Spawn Rate", &SpawnModule->Rate.Constant, 0.1f, 0.0f, 1000.0f))
                {
                    if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem(); // 값 변경 시 재빌드
                }
                // BurstList 편집 UI (TArray<FParticleBurst>는 더 복잡한 UI 필요)
                ImGui::Text("Bursts: %d", SpawnModule->BurstList.Num());
                // ... (버스트 추가/삭제/편집 UI) ...
            }
            else if (UParticleModuleLifetime* LifetimeModule = Cast<UParticleModuleLifetime>(ActualSelectedModule))
            {
                // ... (라이프타임 모듈 프로퍼티 UI) ...
            }
            else if (UParticleModuleRequired* RequiredMod = Cast<UParticleModuleRequired>(ActualSelectedModule))
            {
                ImGui::TextUnformatted("Required Module Properties:");
                if (ImGui::DragFloat("Emitter Duration", &RequiredMod->EmitterDuration, 0.1f, 0.0f, 1000.0f)) 
                {
                    if (CurrentEditedSystem) 
                        CurrentEditedSystem->InitializeSystem();
                }
                if (ImGui::DragInt("Emitter Loops", &RequiredMod->EmitterLoops, 1, 0, 100)) 
                {
                    if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                }
                ImGui::Text("SubUV (Sprite Sheet):");
                if (ImGui::DragInt("SubImages Horizontal", &RequiredMod->SubImages_Horizontal, 1, 1, 64)) bPropertyChanged = true;
                if (ImGui::DragInt("SubImages Vertical", &RequiredMod->SubImages_Vertical, 1, 1, 64)) bPropertyChanged = true;
                ImGui::Separator();
                ImGui::Text("Lifecycle Flags:");
                if (ImGui::Checkbox("Kill On Deactivate", &RequiredMod->bKillOnDeactivate)) bPropertyChanged = true;
                if (ImGui::Checkbox("Kill On Completed", &RequiredMod->bKillOnCompleted)) bPropertyChanged = true;
                ImGui::Separator();

                ImGui::Text("Rendering Flags:");
                if (ImGui::Checkbox("Requires Sorting", &RequiredMod->bRequiresSorting)) bPropertyChanged = true;
                if (RequiredMod->bRequiresSorting)
                {
                     const char* sortModeItems[] = { "None", "View Depth", "View Distance" /*, ... */ };
                     int currentSortMode = RequiredMod->SortMode; // 또는 static_cast<int>(RequiredMod->SortModeEnum);
                     if (ImGui::Combo("Sort Mode", &currentSortMode, sortModeItems, IM_ARRAYSIZE(sortModeItems))) {
                         RequiredMod->SortMode = currentSortMode; // 또는 static_cast<EParticleSortMode>(currentSortMode);
                         bPropertyChanged = true;
                     }
                }
                if (ImGui::Checkbox("Ignore Component Scale (Meshes Only)", &RequiredMod->bIgnoreComponentScale)) bPropertyChanged = true;
            }
            else if (UParticleModuleTypeDataMesh* MeshTD = Cast<UParticleModuleTypeDataMesh>(ActualSelectedModule))
            {
                ImGui::TextUnformatted("Mesh TypeData Properties:");
                char pathBuf[256];
                // FString의 ToAnsiString()은 임시 객체를 반환할 수 있으므로 주의. strcpy_s 권장.
                // 여기서는 간단히 ToAnsiString().c_str() 사용. 실제로는 버퍼 오버플로우 방지 필요.
                strncpy_s(pathBuf, MeshTD->MeshAssetPath.ToAnsiString().c_str(), sizeof(pathBuf) - 1);
                pathBuf[sizeof(pathBuf) - 1] = 0; // 널 종료 보장

                if (ImGui::InputText("Mesh Asset Path", pathBuf, sizeof(pathBuf)))  // Mesh 설정
                {
                    MeshTD->MeshAssetPath = FString(pathBuf);
                    if (CurrentEditedSystem) 
                        CurrentEditedSystem->InitializeSystem();
                }

                if (ImGui::DragFloat3("Mesh Scale", &MeshTD->MeshScale.X, 0.01f)) 
                { 
                    if (CurrentEditedSystem) 
                        CurrentEditedSystem->InitializeSystem();
                }
            }
            else if (UParticleModuleTypeDataSprite* SpriteTD = Cast<UParticleModuleTypeDataSprite>(ActualSelectedModule))
            {
                ImGui::TextUnformatted("Sprite TypeData Properties:");
                // 스프라이트 타입 데이터 관련 프로퍼티 (예: 기본 SubUV 설정 등)
            }
            else
            {
                ImGui::Text("Selected module type has no specific properties exposed for editing yet.");
            }


        }

        else if (SelectedEmitter)
        {
            ImGui::Text("Emitter: %s", *SelectedEmitter->EmitterName);
            ImGui::Separator();
            // 예: 이미터 활성화 상태 편집 (LOD 0의 bEnabled)
            if (SelectedEmitter->GetHighestLODLevel())
            {
                if (ImGui::Checkbox("Enabled", &SelectedEmitter->GetHighestLODLevel()->bEnabled))
                {
                    if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                }
            }
        }
        else
        {
            ImGui::Text("선택된 항목 없음");
        }
        ImGui::PopStyleColor(2);
        ImGui::EndTabItem();
        ImGui::EndTabBar();
    }
    ImGui::EndChild(); // PropertiesPanel_BottomLeft
}

void ParticleSystemViewerPanel::RenderEmitterStrip(const ImVec2& panelSize)
{
    if (!CurrentEditedSystem) return;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.007, 0.007, 0.007, 1));
    bool systemHierarchyVisible = ImGui::BeginChild("SystemHierarchyPanel_Right", panelSize, ImGuiChildFlags_None);
    if (systemHierarchyVisible)
    {
        if (ImGui::Button("선택 이미터 삭제"))
        {
            HandleDeleteSelectedEmitter();
        }
        ImGui::Separator();


        ImVec2 emitterStripContentSize = ImGui::GetContentRegionAvail();
        ImGuiWindowFlags emitterStripFlags = ImGuiWindowFlags_HorizontalScrollbar;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));

        bool emitterStripVisible = ImGui::BeginChild("EmitterStrip", emitterStripContentSize, ImGuiChildFlags_None, emitterStripFlags);
        if (emitterStripVisible)
        {
            float fixedBlockWidth = 150.0f;

            float blockHeight = ImGui::GetContentRegionAvail().y > 50.0f ? ImGui::GetContentRegionAvail().y : 50.0f;
            ImVec2 emitterBlockSize = ImVec2(fixedBlockWidth, blockHeight - ImGui::GetStyle().WindowPadding.y * 2.0f);


            HandleAddEmitterMenu();

            for (int i = 0; i < CurrentEditedSystem->Emitters.Num(); ++i)
            {
                UParticleEmitter* Emitter = CurrentEditedSystem->Emitters[i];
                if (!Emitter) continue;

                if (i > 0) ImGui::SameLine();
                
                ImGui::PushID(i); // 각 이미터 블록에 고유 ID 부여

                bool isEmitterSelectedForFrame = (SelectedEmitterIndex_Internal == i && SelectedModuleIndex_Internal == -1);
                
            
                ImVec4 childBgColor;
                ImVec4 childBorderColorVec4;
                if (isEmitterSelectedForFrame)
                {
                    childBgColor = ImVec4(0.05f, 0.14f, 0.24f, 1.0f);
                    childBorderColorVec4 = ImVec4(0.1f, 0.7f, 0.6f, 1.0f);     // 청록
                }
                else
                {
                    childBgColor = ImVec4(0.028f, 0.028f, 0.048f, 0.2f);      // 매우 어두운 배경
                    childBorderColorVec4 = ImVec4(0.028f, 0.028f, 0.048f, 1.0f);      // 미묘하게 밝은 테두리
                }
                ImU32 childBorderColorU32 = ImGui::GetColorU32(childBorderColorVec4);;

                ImGui::PushStyleColor(ImGuiCol_ChildBg, childBgColor);
                ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(childBorderColorU32));
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

                char child_id_str[64];
                snprintf(child_id_str, sizeof(child_id_str), "emitter_block_frame_%d", i);
                bool emitterBlockFrameVisible = ImGui::BeginChild(child_id_str, emitterBlockSize, ImGuiChildFlags_Border, ImGuiWindowFlags_None);
                if (emitterBlockFrameVisible)
                {
                    RenderEmitterBlockContents(i, Emitter);
                    HandleAddModuleMenu(Emitter);
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
    ImGui::PopStyleColor();
}

void ParticleSystemViewerPanel::HandleAddEmitterMenu()
{
    if (!CurrentEditedSystem) return;

    // EmitterStrip의 빈 공간 우클릭 시 팝업 (ImGui::BeginChild("EmitterStrip", ...) 내부에서 호출)
    if (ImGui::BeginPopupContextWindow("AddEmitterContextWindow", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
    {
        if (ImGui::Selectable("New Particle Sprite Emitter"))
        {
            UParticleEmitter* NewEmitter = FObjectFactory::ConstructObject<UParticleEmitter>(CurrentEditedSystem);
            NewEmitter->EmitterName = FString::Printf(TEXT("SpriteEmitter_%d"), CurrentEditedSystem->Emitters.Num());

            UParticleLODLevel* LOD = FObjectFactory::ConstructObject<UParticleLODLevel>(NewEmitter);
            LOD->RequiredModule = FObjectFactory::ConstructObject<UParticleModuleRequired>(LOD);
            LOD->TypeDataModule = FObjectFactory::ConstructObject<UParticleModuleTypeDataSprite>(LOD);
            LOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleSpawn>(LOD));    // 기본 스폰 모듈
            LOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleLifetime>(LOD)); // 기본 라이프타임 모듈
            NewEmitter->LODLevels.Add(LOD);

            CurrentEditedSystem->Emitters.Add(NewEmitter);
            CurrentEditedSystem->InitializeSystem(); // 시스템 재빌드
            SelectedEmitterIndex_Internal = CurrentEditedSystem->Emitters.Num() - 1; // 새로 추가된 이미터 선택
            SelectedModuleIndex_Internal = -1;
        }
        if (ImGui::Selectable("New Particle Mesh Emitter"))
        {
            UParticleEmitter* NewEmitter = FObjectFactory::ConstructObject<UParticleEmitter>(CurrentEditedSystem);
            NewEmitter->EmitterName = FString::Printf(TEXT("MeshEmitter_%d"), CurrentEditedSystem->Emitters.Num());

            UParticleLODLevel* LOD = FObjectFactory::ConstructObject<UParticleLODLevel>(NewEmitter);
            LOD->RequiredModule = FObjectFactory::ConstructObject<UParticleModuleRequired>(LOD);
            LOD->TypeDataModule = FObjectFactory::ConstructObject<UParticleModuleTypeDataMesh>(LOD); // 메시 타입 데이터
            LOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleSpawn>(LOD));
            LOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleLifetime>(LOD));
            NewEmitter->LODLevels.Add(LOD);

            CurrentEditedSystem->Emitters.Add(NewEmitter);
            CurrentEditedSystem->InitializeSystem();
            SelectedEmitterIndex_Internal = CurrentEditedSystem->Emitters.Num() - 1;
            SelectedModuleIndex_Internal = -1;
        }
        ImGui::EndPopup();
    }
}

void ParticleSystemViewerPanel::HandleAddModuleMenu(UParticleEmitter* TargetEmitter)
{
    if (!TargetEmitter || TargetEmitter->LODLevels.IsEmpty() || !TargetEmitter->LODLevels[0]) return;
    UParticleLODLevel* TargetLOD = TargetEmitter->LODLevels[0];

    // 이미터 블록 내부(예: 모듈 목록 아래 빈 공간) 우클릭 시 팝업
    // RenderEmitterBlockContents 함수 내부의 적절한 위치에서 이 함수를 호출하거나,
    // 또는 모듈 목록 영역에 대한 컨텍스트 메뉴로 처리
    if (ImGui::BeginPopupContextItem("AddModuleContextItem")) // 특정 아이템(이미터 블록)에 대한 컨텍스트 메뉴
    {
        if (ImGui::Selectable("Add Lifetime Module"))
        {
            TargetLOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleLifetime>(TargetLOD));
            if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem(); // 또는 TargetEmitter->Build();
        }
        if (ImGui::Selectable("Add Spawn Module")) // 이미 있다면 중복 추가 방지 로직 필요할 수 있음
        {
            TargetLOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleSpawn>(TargetLOD));
            if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
        }
        // ... 기타 모듈 추가 ...
        ImGui::EndPopup();
    }
}

void ParticleSystemViewerPanel::HandleDeleteSelectedEmitter()
{
    if (CurrentEditedSystem && SelectedEmitterIndex_Internal != -1 && CurrentEditedSystem->Emitters.IsValidIndex(SelectedEmitterIndex_Internal))
    {
        UParticleEmitter* EmitterToRemove = CurrentEditedSystem->Emitters[SelectedEmitterIndex_Internal];
        CurrentEditedSystem->Emitters.RemoveAt(SelectedEmitterIndex_Internal);
        // EmitterToRemove->MarkAsGarbage(); // UObject 시스템의 소멸 처리
        SelectedEmitterIndex_Internal = -1;
        SelectedModuleIndex_Internal = -1;
        CurrentEditedSystem->InitializeSystem();
    }
}

void ParticleSystemViewerPanel::HandleDeleteSelectedModule(UParticleEmitter* TargetEmitter)
{
    if (!CurrentEditedSystem || !TargetEmitter || TargetEmitter->LODLevels.IsEmpty() || !TargetEmitter->LODLevels[0])
    {
        return; // 유효하지 않은 입력 또는 상태
    }

    UParticleLODLevel* LOD = TargetEmitter->LODLevels[0];

    // SelectedModuleIndex_Internal이 유효한 인덱스인지 확인
    if (SelectedModuleIndex_Internal != -1 && LOD->Modules.IsValidIndex(SelectedModuleIndex_Internal))
    {

        LOD->Modules.RemoveAt(SelectedModuleIndex_Internal);

        // 선택 상태 초기화 또는 조정
        SelectedModuleIndex_Internal = -1;
        // 만약 SelectedEmitterIndex_Internal이 TargetEmitter의 인덱스와 다르면 문제가 될 수 있으므로,
        // 이 함수는 보통 현재 선택된 이미터(SelectedEmitterIndex_Internal로 식별되는)에 대해서만 작동해야 합니다.

        // 변경 사항 반영을 위해 시스템 재빌드
        CurrentEditedSystem->InitializeSystem();
        // 또는 최소한 해당 이미터만 재빌드
        // TargetEmitter->Build();
        // CurrentEditedSystem->UpdateComputedFlags(); // 이미터 변경이 시스템 전체 플래그에 영향 줄 수 있음
    }
}

void ParticleSystemViewerPanel::RenderEmitterBlockContents(int emitterIdx, UParticleEmitter* Emitter) 
{
    if (!Emitter || Emitter->LODLevels.IsEmpty() || !Emitter->LODLevels[0] || !Emitter->LODLevels[0]->RequiredModule)
    {
        return;
    }
    
    UParticleLODLevel* LOD = Emitter->LODLevels[0];

    ImVec2 nameStartPos = ImGui::GetCursorScreenPos();
    FString EmitterName = Emitter->EmitterName;
    ImGui::Text("%s", EmitterName.ToAnsiString().c_str());

    float checkboxWidth = ImGui::GetFrameHeight();
    // 체크박스를 이름 오른쪽에 정렬하기 위한 오프셋 계산
    // 현재 사용 가능한 너비에서 체크박스 너비와 약간의 간격을 뺌
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float desiredCheckboxX = ImGui::GetWindowContentRegionMax().x - checkboxWidth - ImGui::GetStyle().ItemInnerSpacing.x;
    float currentX = ImGui::GetCursorPosX(); // Text 다음에 오는 커서 X 위치

    if (currentX < desiredCheckboxX) 
    {
        ImGui::SameLine(0.0f, desiredCheckboxX - currentX);
    }
    else { // 이름이 너무 길어서 공간이 없으면 그냥 옆에 붙임
        ImGui::SameLine();
    }

    if(ImGui::Checkbox("##Enable", &LOD->bEnabled))
    {
        if (CurrentEditedSystem) 
            CurrentEditedSystem->InitializeSystem();
    }

    ImVec2 checkboxEndPos = ImGui::GetItemRectMax();

    // 이름과 체크박스 영역 전체를 클릭 가능하게 만들기 위해 커서 위치 재설정
    ImGui::SetCursorScreenPos(nameStartPos);
    if (ImGui::InvisibleButton("##emitter_header_clickable", ImVec2(checkboxEndPos.x - nameStartPos.x, ImGui::GetTextLineHeightWithSpacing())))
    {
        SelectedEmitterIndex_Internal = emitterIdx;
        SelectedModuleIndex_Internal = -1;
    }
    ImGui::SetItemAllowOverlap();

    ImGui::Separator();

    const int REQUIRED_MODULE_INDEX = -2;
    const int TYPEDATA_MODULE_INDEX = -3;

    if (LOD->RequiredModule)
    {
        UParticleModule* Module = LOD->RequiredModule; // 공통 타입으로 처리
        ImGui::PushID("RequiredModule"); // 고유 ID

        bool isThisModuleSelected = (SelectedEmitterIndex_Internal == emitterIdx && SelectedModuleIndex_Internal == REQUIRED_MODULE_INDEX);
        // ... (Selectable 배경색 및 스타일 설정 로직 - 이전 답변 참고) ...
        ImVec4 moduleSelectedBgColor = ImVec4(0.8f, 0.5f, 0.2f, 1.0f); // 선택 시 다른 색상 (예시)
        ImVec4 moduleRegularBgColor = ImVec4(0.2f, 0.2f, 0.22f, 0.5f); // 기본 배경색 (예시)
        // ... (PushStyleColor 등) ...
        if (isThisModuleSelected) ImGui::PushStyleColor(ImGuiCol_Header, moduleSelectedBgColor);
        else ImGui::PushStyleColor(ImGuiCol_Header, moduleRegularBgColor);
        // HeaderHovered, HeaderActive도 유사하게 설정

        FString ModuleDisplayName = Module->GetModuleDisplayName(); // "Required" 반환 예상
        if (ImGui::Selectable(ModuleDisplayName.ToAnsiString().c_str(), isThisModuleSelected, ImGuiSelectableFlags_DontClosePopups, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 2.0f)))
        {
            SelectedEmitterIndex_Internal = emitterIdx;
            SelectedModuleIndex_Internal = REQUIRED_MODULE_INDEX; // 특별 인덱스로 선택
        }
        if (isThisModuleSelected) ImGui::PopStyleColor(1); else ImGui::PopStyleColor(1); // Header
        // ImGui::PopStyleColor(2); // HeaderHovered, HeaderActive도 Pop
        ImGui::PopID();
    }

    if (LOD->TypeDataModule)
    {
        UParticleModule* Module = LOD->TypeDataModule; // 공통 타입으로 처리
        ImGui::PushID("TypeDataModule");

        bool isThisModuleSelected = (SelectedEmitterIndex_Internal == emitterIdx && SelectedModuleIndex_Internal == TYPEDATA_MODULE_INDEX);
        // ... (Selectable 배경색 및 스타일 설정 로직) ...
        ImVec4 moduleSelectedBgColor = ImVec4(0.2f, 0.5f, 0.8f, 1.0f); // 선택 시 다른 색상 (예시)
        ImVec4 moduleRegularBgColor = ImVec4(0.2f, 0.2f, 0.22f, 0.5f); // 기본 배경색 (예시)
        if (isThisModuleSelected) ImGui::PushStyleColor(ImGuiCol_Header, moduleSelectedBgColor);
        else ImGui::PushStyleColor(ImGuiCol_Header, moduleRegularBgColor);

        FString ModuleDisplayName = Module->GetModuleDisplayName(); // "TypeData Sprite", "TypeData Mesh" 등 반환 예상
        if (ImGui::Selectable(ModuleDisplayName.ToAnsiString().c_str(), isThisModuleSelected, ImGuiSelectableFlags_DontClosePopups, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 2.0f)))
        {
            SelectedEmitterIndex_Internal = emitterIdx;
            SelectedModuleIndex_Internal = TYPEDATA_MODULE_INDEX; // 특별 인덱스로 선택
        }
        if (isThisModuleSelected) ImGui::PopStyleColor(1); else ImGui::PopStyleColor(1);
        ImGui::PopID();
    }

    if (LOD->RequiredModule || LOD->TypeDataModule) ImGui::Separator(); // 구분선
    for (int moduleIdxLoop = 0; moduleIdxLoop < LOD->Modules.Num(); ++moduleIdxLoop)
    {
        UParticleModule* Module = LOD->Modules[moduleIdxLoop];
        if (!Module) continue;

        
        ImGui::PushID(moduleIdxLoop); // 각 모듈에 대해 고유 ID 스택 생성

        bool isThisModuleSelected = (SelectedEmitterIndex_Internal == emitterIdx && SelectedModuleIndex_Internal == moduleIdxLoop);

        // Selectable의 레이블이 중복될 수 있으므로, PushID로 구분하거나 레이블 자체를 고유하게 만듦

        // --- 배경색 변경 시작 ---
        ImVec4 moduleRegularBgColor = ImVec4(0.15f, 0.15f, 0.19f, 0.2f);

        ImVec4 moduleSelectedBgColor = ImVec4(1.0f, 0.15f, 0.0f, 1.0f);

        ImVec4 moduleHoveredColor = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);

        ImVec4 moduleSelectedHoveredColor = ImVec4(moduleSelectedBgColor.x * 1.1f,
            moduleSelectedBgColor.y * 1.1f, moduleSelectedBgColor.z * 1.1f, 1.0f);

        ImVec4 moduleActiveColor = moduleSelectedBgColor;

        if (isThisModuleSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, moduleSelectedBgColor);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, moduleSelectedBgColor);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, moduleSelectedBgColor);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Header, moduleRegularBgColor);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, moduleRegularBgColor);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, moduleActiveColor);
        }

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 itemTopLeft = ImGui::GetCursorScreenPos();
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float itemHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 2.0f; // 표준 아이템 높이

        ImVec2 itemBottomRight = ImVec2(itemTopLeft.x + availableWidth, itemTopLeft.y + itemHeight);

        // 3. 가로로 꽉 차는 배경 사각형 그리기
        ImVec4 currentBgColorForRect = isThisModuleSelected ? moduleSelectedBgColor : moduleRegularBgColor;
        ImU32 currentBgColorForRectU32 = ImGui::GetColorU32(currentBgColorForRect);

        ImGui::GetWindowDrawList()->AddRectFilled(itemTopLeft, itemBottomRight, currentBgColorForRectU32);

        ImVec2 Size = ImVec2(0, itemHeight);
        FString ModuleName = Module->GetModuleDisplayName();
        if (ImGui::Selectable(ModuleName.ToAnsiString().c_str(), isThisModuleSelected, ImGuiSelectableFlags_DontClosePopups, Size))
        {
            SelectedEmitterIndex_Internal = emitterIdx;
            SelectedModuleIndex_Internal = moduleIdxLoop;
        }
        ImGui::PopStyleColor(3);
        ImGui::PopID(); // moduleIdxLoop에 대한 Pop
    }
    if (SelectedEmitterIndex_Internal == emitterIdx && SelectedModuleIndex_Internal != -1) 
    {
        if (ImGui::BeginPopupContextItem("DeleteModuleContextItem")) 
        { 
            // 현재 선택된 모듈 아이템 위에서 우클릭
            if (ImGui::Selectable("Delete Selected Module")) 
            {
                HandleDeleteSelectedModule(Emitter);
            }
            ImGui::EndPopup();
        }
    }
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
