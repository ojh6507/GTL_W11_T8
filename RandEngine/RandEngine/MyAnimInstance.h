#pragma once

#include "UObject/ObjectMacros.h"
#include "Animation/AnimInstance.h"

class UAnimSequence;

class UMyAnimInstance : public UAnimInstance
{
    DECLARE_CLASS(UMyAnimInstance, UAnimInstance)
public:
    UMyAnimInstance();
    ~UMyAnimInstance();

    void SetLooping(bool bInIsLooping);

    void SetPlaying(bool bInIsPlaying);

    UAnimSequence* AnimA;

    UAnimSequence* AnimB;

    float BlendAlpha = 0.5f;
    int32 frame = 0;
    bool bIsLooping;
    bool bIsPlaying;
    float CurrentTime;
    float PlayRate;

protected:
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
