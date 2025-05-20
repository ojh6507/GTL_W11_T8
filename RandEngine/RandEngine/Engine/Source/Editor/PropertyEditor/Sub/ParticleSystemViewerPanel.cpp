#include "ParticleSystemViewerPanel.h"
#include "Define.h"

#include "Engine/AssetManager.h"
#include "Engine/UnrealClient.h"

#include "UnrealEd/EditorViewportClient.h"

#include "PropertyEditor/ShowFlags.h"

#include "Renderer/RendererHelpers.h"

#include "SubWindow/SubEngine.h"


#include "Components/Material/Material.h"

#include "Particle/ParticleSystem.h"
#include "Particle/ParticleEmitter.h"
#include "Particle/ParticleLODLevel.h"

#include "Particle/ParticleModule.h"
#include "Particle/ParticleModuleRequired.h"
#include "Particle/ParticleModuleTypeDataSprite.h"
#include "Particle/ParticleModuleTypeDataMesh.h"
#include "Particle/ParticleModuleSpawn.h"
#include "Particle/ParticleModuleLifetime.h"
#include "Particle/ParticleModuleColor.h"
#include "Particle/ParticleModuleLocation.h"
#include "Particle/ParticleModuleSize.h"
#include "Particle/ParticleModuleVelocity.h"




float ParticleSystemViewerPanel::LeftAreaTotalRatio = 0.7f;
float ParticleSystemViewerPanel::ViewportInLeftRatio = 0.6f;


const ImVec4 GRegularBgColor(0.15f, 0.15f, 0.19f, 0.2f);
const ImVec4 GSelectedBgColor(1.0f, 0.15f, 0.0f, 1.0f);
const ImVec4 GBlackTextColor(0.0f, 0.0f, 0.0f, 1.0f);

const int REQUIRED_MODULE_INDEX = -2;
const int TYPEDATA_MODULE_INDEX = -3;
const int NO_MODULE_SELECTED = -1;


ParticleSystemViewerPanel::ParticleSystemViewerPanel()
	: CurrentEditedSystem(nullptr)
	, SelectedEmitterIndex_Internal(-1)
	, SelectedModuleIndex_Internal(-1)
	, Width(800.0f), Height(600.0f) // 생성자에서 초기화
	, RenderTargetRHI(nullptr), DepthStencilRHI(nullptr)
{
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
	RenderTargetRHI = ViewportResource->GetRenderTarget(EResourceType::ERT_SubScene);
	DepthStencilRHI = ViewportResource->GetDepthStencil(EResourceType::ERT_SubScene);

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

void ParticleSystemViewerPanel::RenderPropertiesPanel(const ImVec2& panelSize, UParticleEmitter* SelectedEmitter, UParticleModule* DefaultSelectedModule)
{
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
                if (ImGui::DragFloat("Lifetime", &LifetimeModule->Lifetime.Constant, 0.1f, 0.0f, 1000.0f))
                {
                    if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem(); // 값 변경 시 재빌드
                }
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


				const TMap<FName, FAssetInfo> Assets = UAssetManager::Get().GetAssetRegistry();
				FString PreviewName = MeshTD->MeshAssetPath;
				if (PreviewName.IsEmpty())
				{
					PreviewName = "None";
				}
				if (ImGui::BeginCombo("##StaticMesh", GetData(PreviewName), ImGuiComboFlags_None))
				{
					for (const auto& Asset : Assets)
					{
						if (Asset.Value.AssetType != EAssetType::StaticMesh)
						{
							continue;
						}
						if (ImGui::Selectable(GetData(Asset.Value.AssetName.ToString()), false))
						{
							FString MeshName = Asset.Value.PackagePath.ToString() + "/" + Asset.Value.AssetName.ToString();
							MeshTD->MeshAssetPath = MeshName;
							if (CurrentEditedSystem)
							{
								CurrentEditedSystem->InitializeSystem();
							}
						}
					}
					ImGui::EndCombo();
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
				ImGui::Text("Texture:");
				FString CurrentTexturePath = SpriteTD->TextureAssetPath;
				if (CurrentTexturePath.IsEmpty())
				{
					CurrentTexturePath = "None";
				}
				if (ImGui::BeginCombo("##SpriteTextureAsset", CurrentTexturePath.ToAnsiString().c_str()))
				{
					const auto& LoadedTextures = FEngineLoop::ResourceManager.GetAllTextures();
					bool bNoneSelected = CurrentTexturePath == "None";
					if (ImGui::Selectable("None", bNoneSelected))
					{
						if (!bNoneSelected)
						{
							SpriteTD->TextureAssetPath.Empty();
							SpriteTD->CachedTexture = nullptr;
							bPropertyChanged = true;
						}
					}
					for (const auto& Pair : LoadedTextures)
					{
						FString TexturePathFStr = Pair.Key.c_str(); // FWString -> FString
						bool is_selected = (SpriteTD->TextureAssetPath == TexturePathFStr);
						if (ImGui::Selectable(TexturePathFStr.ToAnsiString().c_str(), is_selected))
						{
							if (!is_selected)
							{
								SpriteTD->TextureAssetPath = TexturePathFStr;
								std::shared_ptr<FTexture> TexSharedPtr = FEngineLoop::ResourceManager.GetTexture(Pair.Key);
								SpriteTD->CachedTexture = TexSharedPtr ? TexSharedPtr.get() : nullptr;
								bPropertyChanged = true;
							}
						}
						if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				ImGui::SameLine();

				if (!SpriteTD->TextureAssetPath.IsEmpty() && SpriteTD->CachedTexture->TextureSRV)
				{
					ImGui::Image((ImTextureID)SpriteTD->CachedTexture->TextureSRV, ImVec2(64, 64));
				}
				ImGui::Separator();
				// --- Texture 선택 UI 끝 ---
			}
            else if (UParticleModuleColor* ColorModule = Cast<UParticleModuleColor>(ActualSelectedModule))
            {
                ImGui::SeparatorText("Color Over Life Module");
                if (ImGui::CollapsingHeader("Start Color", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::ColorEdit4("Start Color##ColMod", &ColorModule->StartColor.R))
                    {
                        if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                    }
                }
                if (ImGui::CollapsingHeader("End Color", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::ColorEdit4("End Color##ColMod", &ColorModule->EndColor.R))
                    {
                        if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                    }
                }
                if (ImGui::Checkbox("Interpolate Color", &ColorModule->bInterpolateColor))
                {
                    if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                }
            }
            else if (UParticleModuleLocation* LocationModule = Cast<UParticleModuleLocation>(ActualSelectedModule))
            {
                ImGui::SeparatorText("Location Module");

                // Shape 편집 (Enum)
                const char* shapeNames[] = { "Point", "Box", "Sphere" };
                int currentShapeIndex = static_cast<int>(LocationModule->Shape);

                if (ImGui::Combo("Shape", &currentShapeIndex, shapeNames, IM_ARRAYSIZE(shapeNames)))
                {
                    LocationModule->Shape = static_cast<ELocationShape>(currentShapeIndex);
                    if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                }

                if (ImGui::DragFloat3("Start Location", &LocationModule->StartLocation.X, 0.1f))
                {
                    if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                }

                switch (LocationModule->Shape)
                {
                case ELocationShape::Point:
                    // Point는 StartLocation 외에 추가 파라미터 없음
                    ImGui::TextDisabled("Spawns particles at the Start Location.");
                    break;

                case ELocationShape::Box:
                    ImGui::Text("Box Shape Parameters:");
                    if (ImGui::DragFloat3("Box Extent", &LocationModule->BoxExtent.X, 0.1f, 0.0f, 1000.0f, "%.2f")) // 최소 0.0f, 최대 1000.0f
                    {
                        // BoxExtent는 음수 값을 가지면 안 되므로, 필요하다면 입력 후 양수로 클램핑
                        LocationModule->BoxExtent.X = FMath::Max(0.0f, LocationModule->BoxExtent.X);
                        LocationModule->BoxExtent.Y = FMath::Max(0.0f, LocationModule->BoxExtent.Y);
                        LocationModule->BoxExtent.Z = FMath::Max(0.0f, LocationModule->BoxExtent.Z);
                        if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                    }
                    ImGui::TextDisabled("Half the size of the box in each axis direction from Start Location.");
                    break;

                case ELocationShape::Sphere:
                    ImGui::Text("Sphere Shape Parameters:");
                    if (ImGui::DragFloat("Sphere Radius", &LocationModule->SphereRadius, 0.1f, 0.0f, 1000.0f, "%.2f")) // 최소 0.0f, 최대 1000.0f
                    {
                        // Radius는 음수 값을 가지면 안 되므로, 필요하다면 입력 후 양수로 클램핑
                        LocationModule->SphereRadius = FMath::Max(0.0f, LocationModule->SphereRadius);

                        if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                    }
                    ImGui::TextDisabled("Radius of the sphere centered at Start Location.");
                    break;
                }
            }
            else if (UParticleModuleSize* SizeModule = Cast<UParticleModuleSize>(ActualSelectedModule))
            {
                ImGui::SeparatorText("Size Module");
                if (ImGui::CollapsingHeader("Start Size", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::DragFloat3("Start Size##SizeMod", &SizeModule->StartSize.X))
                    {
                        if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                    }
                }
                if (ImGui::CollapsingHeader("End Size", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::DragFloat3("End Size##SizeMod", &SizeModule->EndSize.X))
                    {
                        if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                    }
                }
                if (ImGui::Checkbox("Interpolate Size", &SizeModule->bInterpolateSize))
                {
                    if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                }
            }
            else if (UParticleModuleVelocity* VelocityModule = Cast<UParticleModuleVelocity>(ActualSelectedModule))
            {
                ImGui::SeparatorText("Start Velocity Module");
                if (ImGui::CollapsingHeader("Max Start Velocity", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::DragFloat3("Max Start Velocity##VelMod", &VelocityModule->MaxStartVelocity.X))
                    {
                        if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                    }
                }
                if (ImGui::CollapsingHeader("Min Start Velocity", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (ImGui::DragFloat3("Min Start Velocity##VelMod", &VelocityModule->MinStartVelocity.X))
                    {
                        if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
                    }
                }
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
			float fixedBlockWidth = 200.0f;

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
				childBgColor = ImVec4(0.028f, 0.028f, 0.048f, 0.2f);      // 매우 어두운 배경
				childBorderColorVec4 = ImVec4(0.028f, 0.028f, 0.048f, 1.0f);      // 미묘하게 밝은 테두리

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

void ParticleSystemViewerPanel::RenderEmitterHeader(int emitterIdx, UParticleEmitter* Emitter, UParticleLODLevel* LOD)
{
	ImVec2 headerTopLeft = ImGui::GetCursorScreenPos();
	float headerHeight = 50.0f;
	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImVec2 headerBottomRight = ImVec2(headerTopLeft.x + availableWidth, headerTopLeft.y + headerHeight);

	bool isEmitterSelected = (SelectedEmitterIndex_Internal == emitterIdx && SelectedModuleIndex_Internal == NO_MODULE_SELECTED);
	ImVec4 currentBgColor = isEmitterSelected ? GSelectedBgColor : ImVec4(0.7, 0.7, 0.7, 1);
	ImU32 currentBgColorU32 = ImGui::GetColorU32(currentBgColor);

	ImGui::GetWindowDrawList()->AddRectFilled(headerTopLeft, headerBottomRight, currentBgColorU32);

	FString emitterSelectButtonId = FString::Printf("##EmitterSelect_%d", emitterIdx);
	ImGui::SetCursorScreenPos(headerTopLeft);
	if (ImGui::InvisibleButton(emitterSelectButtonId.ToAnsiString().c_str(), ImVec2(availableWidth, headerHeight)))
	{
		SelectedEmitterIndex_Internal = emitterIdx;
		SelectedModuleIndex_Internal = NO_MODULE_SELECTED;
	}
	ImGui::SetItemAllowOverlap();

	float textPaddingX = ImGui::GetStyle().FramePadding.x;
	float textBaselineY = headerTopLeft.y + (headerHeight - ImGui::GetTextLineHeight()) / 2.0f;

	ImGui::SetCursorScreenPos(ImVec2(headerTopLeft.x + textPaddingX, textBaselineY));
	ImGui::PushStyleColor(ImGuiCol_Text, GBlackTextColor);
	ImGui::Text("%s", Emitter->EmitterName.ToAnsiString().c_str());
	ImGui::PopStyleColor();

	float checkboxWidth = ImGui::GetFrameHeight();
	float checkboxPaddingX = ImGui::GetStyle().FramePadding.x;
	float checkboxX = headerBottomRight.x - checkboxWidth - checkboxPaddingX;
	float checkboxY = headerTopLeft.y + (headerHeight - ImGui::GetFrameHeight()) / 2.0f;

	ImGui::SetCursorScreenPos(ImVec2(checkboxX, checkboxY));
	FString checkboxId = FString::Printf("##Enable_%d", emitterIdx);
	ImGui::PushID(checkboxId.ToAnsiString().c_str());
	if (ImGui::Checkbox("", &LOD->bEnabled))
	{
		if (CurrentEditedSystem)
			CurrentEditedSystem->InitializeSystem();
	}
	ImGui::PopID();

	if (LOD->TypeDataModule && LOD->TypeDataModule->GetModuleType() == EModuleType::TypeDataSprite)
	{
		UParticleModuleTypeDataSprite* SpriteTD = static_cast<UParticleModuleTypeDataSprite*>(LOD->TypeDataModule);
		if (SpriteTD->CachedTexture && !SpriteTD->TextureAssetPath.IsEmpty() && SpriteTD->CachedTexture->TextureSRV)
		{
			ImVec2 texturePreviewSize(48, 48);
			float texturePreviewSpacing = ImGui::GetStyle().ItemSpacing.x;
			float texturePreviewX = checkboxX - texturePreviewSize.x - texturePreviewSpacing;
			float texturePreviewY = headerTopLeft.y + (headerHeight - texturePreviewSize.y) / 2.0f;

			ImGui::SetCursorScreenPos(ImVec2(texturePreviewX, texturePreviewY));
			ImGui::Image((ImTextureID)SpriteTD->CachedTexture->TextureSRV, texturePreviewSize);
		}
	}
	// 메시 타입 데이터 모듈의 미리보기 로직 (필요시 추가)
	// else if (LOD->TypeDataModule && LOD->TypeDataModule->GetModuleType() == EModuleType::TypeDataMesh) { ... }


	ImGui::SetCursorScreenPos(ImVec2(headerTopLeft.x, headerBottomRight.y));
}

void ParticleSystemViewerPanel::RenderEmitterBlockContents(int emitterIdx, UParticleEmitter* Emitter)
{
	if (!Emitter || Emitter->LODLevels.IsEmpty() || !Emitter->LODLevels[0] || !Emitter->LODLevels[0]->RequiredModule)
	{
		return;
	}

	UParticleLODLevel* LOD = Emitter->LODLevels[0];

	// 1. 이미터 헤더 렌더링
	RenderEmitterHeader(emitterIdx, Emitter, LOD);
	ImGui::Separator();

	// 2. Required 모듈 렌더링
	if (LOD->RequiredModule)
	{
		RenderModuleEntry(LOD->RequiredModule, LOD->RequiredModule->GetModuleDisplayName(), emitterIdx, REQUIRED_MODULE_INDEX);
	}

	// 3. TypeData 모듈 렌더링
	if (LOD->TypeDataModule)
	{
		RenderModuleEntry(LOD->TypeDataModule, LOD->TypeDataModule->GetModuleDisplayName(), emitterIdx, TYPEDATA_MODULE_INDEX);
	}

	if (LOD->RequiredModule || LOD->TypeDataModule)
	{
		ImGui::Separator();
	}

	// 4. 일반 모듈들 렌더링
	for (int moduleIdxLoop = 0; moduleIdxLoop < LOD->Modules.Num(); ++moduleIdxLoop)
	{
		UParticleModule* Module = LOD->Modules[moduleIdxLoop];
		if (!Module)
		{
			continue;
		}
		RenderModuleEntry(Module, Module->GetModuleDisplayName(), emitterIdx, moduleIdxLoop);
	}


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
	
    if (ImGui::BeginPopupContextItem("AddModuleContextItem")) // 특정 아이템(이미터 블록)에 대한 컨텍스트 메뉴
	{
		if (ImGui::Selectable("Add Color Module"))
		{
			TargetLOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleColor>(TargetLOD));
			if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem(); // 또는 TargetEmitter->Build();
		}
		if (ImGui::Selectable("Add Location Module"))
		{
			TargetLOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleLocation>(TargetLOD));
			if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
		}
        if (ImGui::Selectable("Add Size Module"))
        {
            TargetLOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleSize>(TargetLOD));
            if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
        }
        if (ImGui::Selectable("Add Velocity Module"))
        {
            TargetLOD->Modules.Add(FObjectFactory::ConstructObject<UParticleModuleVelocity>(TargetLOD));
            if (CurrentEditedSystem) CurrentEditedSystem->InitializeSystem();
        }
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

void ParticleSystemViewerPanel::RenderModuleEntry(UParticleModule* Module, const FString& DisplayName, int emitterIdx, int moduleIdentifier)
{
	if (!Module) return;

	ImGui::PushID(moduleIdentifier);

	bool isThisModuleSelected = (SelectedEmitterIndex_Internal == emitterIdx && SelectedModuleIndex_Internal == moduleIdentifier);
	ImVec4 currentBgColorForRect = isThisModuleSelected ? GSelectedBgColor : GRegularBgColor;
	ImU32 currentBgColorForRectU32 = ImGui::GetColorU32(currentBgColorForRect);

	if (isThisModuleSelected)
	{
		ImGui::PushStyleColor(ImGuiCol_Header, GSelectedBgColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, GSelectedBgColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, GSelectedBgColor);
	}
	else
	{
		ImGui::PushStyleColor(ImGuiCol_Header, GRegularBgColor);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, GSelectedBgColor);
	}

	ImVec2 itemTopLeft = ImGui::GetCursorScreenPos();
	float availableWidth = ImGui::GetContentRegionAvail().x;
	//float itemHeight = ImGui::GetTextLineHeightWithSpacing() + ImGui::GetStyle().FramePadding.y * 2.0f;
	float itemHeight = ImGui::GetTextLineHeightWithSpacing();
	ImVec2 selectableSize = ImVec2(availableWidth, itemHeight);
	//ImVec2 itemBottomRight = ImVec2(itemTopLeft.x + availableWidth, itemTopLeft.y + itemHeight);

	float bgPaddingY = ImGui::GetStyle().FramePadding.y;
	ImVec2 bgTopLeft = itemTopLeft;
	ImVec2 bgBottomRight = ImVec2(itemTopLeft.x + availableWidth, itemTopLeft.y + itemHeight + bgPaddingY * 2.0f);
	float totalItemHeightWithPadding = itemHeight + bgPaddingY * 2.0f;

	if (availableWidth < 1.0f) 
	{
		ImGui::PopStyleColor(3);
		ImGui::PopID();
		ImGui::SetCursorScreenPos(ImVec2(itemTopLeft.x, itemTopLeft.y + ImGui::GetTextLineHeightWithSpacing()));
		return;
	}
	ImGui::GetWindowDrawList()->AddRectFilled(bgTopLeft, bgBottomRight, currentBgColorForRectU32);
	ImGui::SetCursorScreenPos(ImVec2(itemTopLeft.x + ImGui::GetStyle().FramePadding.x, itemTopLeft.y + bgPaddingY));
	ImVec2 actualSelectableSize = ImVec2(availableWidth - ImGui::GetStyle().FramePadding.x * 2.0f, itemHeight);

	if (ImGui::Selectable(DisplayName.ToAnsiString().c_str(), isThisModuleSelected, ImGuiSelectableFlags_DontClosePopups, actualSelectableSize)) 
	{
		SelectedEmitterIndex_Internal = emitterIdx;
		SelectedModuleIndex_Internal = moduleIdentifier;
	}

	ImGui::PopStyleColor(3);
	ImGui::PopID();

	ImGui::SetCursorScreenPos(ImVec2(bgTopLeft.x, bgBottomRight.y));
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
