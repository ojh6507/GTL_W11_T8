#pragma once

#include "Animation/AnimInstance.h"
#include "UObject/ObjectMacros.h"

class UAnimSingleNodeInstance : public UAnimInstance
{
    DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)

public:
    UAnimSingleNodeInstance();
    ~UAnimSingleNodeInstance();

    void SetAnimationAsset(UAnimationAsset* NewAsset, bool bInIsLooping, float InPlayRate = 1.f);

    void SetLooping(bool bIsLooping);

    void SetPlaying(bool bIsPlaying);

    bool IsPlaying() const;

    void ResetToReferencePose();

    void SetUseExternalTime(bool bUse) { bUseExternalTime = bUse; }

    void SetExternalTime(float NewTime) { ExternalTime = NewTime; }
    void SetPlayRate(float InPlayRate) { PlayRate = InPlayRate; }

    float GetCurrentAnimationTime() const { return CurrentTime; }

    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
public:
    UAnimationAsset* CurrentAsset = nullptr;
    int32 frame = 0;
    float PreviousTime = 0;
    bool bIsLooping;
    bool bIsPlaying;
    float CurrentTime;
    float PlayRate;
    bool bUseExternalTime = false;
    float ExternalTime = 0.f;
    TSet<int> TriggeredNotifyIDsThisCycle;

};

