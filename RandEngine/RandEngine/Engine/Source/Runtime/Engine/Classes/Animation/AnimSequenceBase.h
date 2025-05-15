#pragma once
#include "AnimationAsset.h"
#include "AnimData/AnimDataModel.h"
#include "Animation/AnimTypes.h"
#include "Editor/PropertyEditor/Sub/AnimationTimelineCommon.h"

class UAnimSequenceBase : public UAnimationAsset
{
    DECLARE_CLASS(UAnimSequenceBase, UAnimationAsset)
public:
    TArray<FEditorTimelineTrack> UserDefinedNotifyTracks;

    UAnimSequenceBase() = default;
  
    UAnimDataModel* GetDataModel() const;

    // 언리얼에서는 Set이 없음.
    void SetAnimDataModel(UAnimDataModel* InDataModel);

    TArray<struct FAnimNotifyEvent> Notifies;

    void AddNotify(const FName& Name,const FString& InSoundNameToPlay, float TriggerTime, int AssignedTrackId = -1) // 트랙 ID 인자 추가
    {
        int NewEventId = NextNotifyEventId++;
        Notifies.Add(FAnimNotifyEvent(Name, InSoundNameToPlay, NewEventId, AssignedTrackId, TriggerTime));
        // 정렬은 필요시 외부에서 수행하거나, 추가 직후가 아닌 다른 시점에 수행
    }
    void SortNotifies()
    {
        Notifies.Sort([](const FAnimNotifyEvent& A, const FAnimNotifyEvent& B)
            {
                return A.TriggerTime < B.TriggerTime;
            });
    }
    void RemoveNotifyByEventId(int EventId)
    {
        Notifies.RemoveAll([EventId](const FAnimNotifyEvent& Event)
            {
                return Event.EventId == EventId;
            });
    }
    inline static int NextNotifyEventId;

    FEditorTimelineTrack* FindOrAddUserTrack(const FString& TrackName, int RootTrackId);
    void RemoveUserTrack(int TrackId);
    
protected:
    UAnimDataModel* DataModel;
};
