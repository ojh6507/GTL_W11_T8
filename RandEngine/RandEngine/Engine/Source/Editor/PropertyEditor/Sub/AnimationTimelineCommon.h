#pragma once
#include "string"
// --- Editor Specific Data Structures (ImGui 연동용) ---
enum class EEditorTimelineTrackType
{
    AnimNotify_Root,     // "Notifies" 같은 최상위 그룹
    AnimNotify_UserTrack // 사용자가 추가한 하위 트랙
};

struct FEditorTimelineTrack
{
    EEditorTimelineTrackType TrackType;
    std::string DisplayName; // UTF-8 문자열 사용 권장 (ImGui는 UTF-8 기본)
    int TrackId;             // 이 트랙의 고유 ID
    int ParentTrackId;       // 부모 트랙의 ID (루트면 -1 또는 자기 자신 ID)
    bool bIsExpanded;        // 자신이 그룹일 경우 확장 상태
    // int IndentLevel;      // im-neo-sequencer가 Begin/EndTimelineEx 중첩으로 자동 처리

    FEditorTimelineTrack(EEditorTimelineTrackType InTrackType, const std::string& InDisplayName, int InTrackId, int InParentTrackId = -1)
        : TrackType(InTrackType), DisplayName(InDisplayName), TrackId(InTrackId),
        ParentTrackId(InParentTrackId), bIsExpanded(true) // 기본적으로 확장
    {
        if (NextTrackIdCounter <= InTrackId)
        {
            NextTrackIdCounter = InTrackId + 1;
        }
    }
    inline static int NextTrackIdCounter = 1;
};
