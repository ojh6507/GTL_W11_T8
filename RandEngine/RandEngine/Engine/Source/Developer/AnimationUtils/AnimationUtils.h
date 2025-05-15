#pragma once
#include "Animation/AnimSequence.h"
#include "Animation/AnimationAsset.h"
#include  "Animation/AnimSingleNodeInstance.h"

namespace NotifyUtils
{
    void AddSoundNotifies(UAnimSequenceBase* TargetAnimSequence, const FString& NotifyName, 
                          const FString SoundName, float TriggerTime, int TrackID)
    {
        if (!TargetAnimSequence) return;
        if (SoundName.IsEmpty())
        {
            UE_LOG(ELogLevel::Error, TEXT("SoundName is empty"));
        }
        TargetAnimSequence->AddNotify(NotifyName, SoundName, TriggerTime, TrackID);
    }
}
