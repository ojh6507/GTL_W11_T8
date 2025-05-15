#include "AnimSequenceBase.h"

UAnimDataModel* UAnimSequenceBase::GetDataModel() const
{
    return DataModel;
}

void UAnimSequenceBase::SetAnimDataModel(UAnimDataModel* InDataModel)
{
    DataModel = InDataModel;
}

FEditorTimelineTrack* UAnimSequenceBase::FindOrAddUserTrack(const FString& TrackName, int RootTrackId)
{
    for (FEditorTimelineTrack& Track : UserDefinedNotifyTracks)
    {
        if (Track.DisplayName == TrackName && Track.TrackType == EEditorTimelineTrackType::AnimNotify_UserTrack)
        {
            return &Track;
        }
    }
    // 없으면 새로 추가
    int NewTrackId = FEditorTimelineTrack::NextTrackIdCounter++; // ID 카운터는 패널에서 관리하거나, 전역적이어야 함
    UserDefinedNotifyTracks.Add(FEditorTimelineTrack(EEditorTimelineTrackType::AnimNotify_UserTrack, TrackName.ToAnsiString(), NewTrackId, RootTrackId));
    return &UserDefinedNotifyTracks.Last();
}

void UAnimSequenceBase::RemoveUserTrack(int TrackId)
{
    UserDefinedNotifyTracks.RemoveAll([TrackId](const FEditorTimelineTrack& Track) {
        return Track.TrackId == TrackId;
        });
    // 이 트랙에 할당된 노티파이들의 NotifyTrackIndex도 -1 등으로 업데이트 필요
    for (FAnimNotifyEvent& Notify : Notifies)
    {
        if (Notify.NotifyTrackIndex == TrackId)
        {
            Notify.NotifyTrackIndex = -1; // 또는 기본 루트 트랙 ID
        }
    }
}
