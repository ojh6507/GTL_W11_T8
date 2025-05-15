#pragma once

#include "ImGui/imgui_neo_sequencer.h" // im-neo-sequencer 헤더
#include "Container/Array.h"
#include "Container/Set.h"
#include "UObject/NameTypes.h"
#include "UnrealEd/EditorPanel.h" // UEditorPanel 상속 가정
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/AnimDataModel.h" 
#include "Components/Mesh/SkeletalMeshComponent.h"

#include "Imgui/imgui.h"
#include "Imgui/imgui_internal.h"

class SAnimationTimelinePanel : public UEditorPanel
{
public:
    SAnimationTimelinePanel();
    virtual ~SAnimationTimelinePanel(); // 가상 소멸자 권장

    // --- Public Interface Methods ---
    void InitSkeletalMeshComponent();
    void InitTargetSequence();
    
    // 데이터 접근 및 변환 함수
    float GetSequenceDurationSeconds() const;
    float GetSequenceFrameRate() const;
    int GetSequenceTotalFrames() const;
    int ConvertTimeToFrame() const;
    void ConvertFrameToTimeAndSet(int Frame);

    // 메인 UI 렌더링 함수 (UEditorPanel 오버라이드)
    virtual void Render() override;
    virtual void OnResize(HWND hWnd) override; // Windows 핸들 의존성 제거 고려

private:
    // --- Private Helper Methods for UI Rendering ---
    void RenderAnimationSelector(); // 애니메이션 선택 UI
    void RenderTimelineEditor();      // 전체 타임라인 UI를 그리는 메인 함수
    void RenderPlaybackControls();    // 재생 관련 컨트롤 UI
    void RenderSequencerWidget();     // im-neo-sequencer를 사용하여 실제 시퀀서 부분을 그리는 함수
    void RenderTrackManagementUI();   // 트랙 추가/삭제, 노티파이 할당 등의 UI
    void RenderNotifyPropertiesUI(); // 선택된 노티파이의 속성 UI

    // 트랙 및 노티파이 관리 함수
    void UpdatePlayback(float DeltaSeconds);
    void AddUserNotifyTrack(int ParentRootTrackId, const std::string& NewTrackName);
    void RemoveUserNotifyTrack(int TrackIdToRemove); // TODO: 구현 필요
    void AssignNotifyToTrack(int NotifyEventId, int TargetTrackId); // TODO: 구현 필요

    // 계층적 트랙 렌더링 헬퍼
    void RenderTracksRecursive(int ParentId);


    // --- Member Variables ---
    UAnimSequence* TargetSequence;           // 현재 편집 대상 시퀀스

    TArray<FEditorTimelineTrack> DisplayableTracks; // UI에 표시될 트랙 정보

    // Playback state
    float AnimationDeltaTime = 0;
    bool bIsPlaying;
    bool bIsLooping;
    float PlaybackSpeed;
    float CurrentTimeSeconds;

    // UI Interaction State
    int SelectedNotifyEventId; // 현재 선택된 노티파이의 EventId
    // int SequencerSelectedTimelineId; // im-neo-sequencer의 선택된 타임라인 ID (필요시)
    bool bIsDraggingNotify; // im-neo-sequencer의 드래그 기능을 사용하므로, 별도 플래그 필요성 재검토

    int LastSelectedUserTrackId = -1;
    // Windows 핸들 의존성 제거를 위해 Width, Height는 다른 방식으로 관리 고려
    // (예: ImGui::GetContentRegionAvail())
    float Width;
    float Height;

    USkeletalMeshComponent* SelectedComponent = nullptr;
};
