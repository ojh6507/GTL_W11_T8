#pragma once

#include "UObject/NameTypes.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"

class UAnimSequence;
class UAnimInstance;
struct FPoseContext;
struct FAnimExtractContext;

struct FAnimationTransitionRule
{
    FName RuleToExecute;

    int32 TransitionIndex;

    // 전환 조건을 검사하는 함수 포인터
    typedef bool (*TransitionConditionFunc)(const UAnimInstance* AnimInstance);
    TransitionConditionFunc Condition;

    FAnimationTransitionRule(int32 InTransitionIndex, TransitionConditionFunc InCond, FName InRuleName = NAME_None)
        : RuleToExecute(InRuleName), TransitionIndex(InTransitionIndex), Condition(InCond) {
    }

    bool CanEnter(const UAnimInstance* AnimInstance) const
    {
        if (Condition && AnimInstance)
        {
            return Condition(AnimInstance);
        }
        return false;
    }
};

struct FAnimationState
{
    FName StateName;
    UAnimSequence* AnimationToPlay;
    float PreviousTime = 0.0f;
    float CurrentTime = 0.0f;
    float PlayRate = 1.f;
    TArray<FAnimationTransitionRule> Transitions;

    FAnimationState(FName InName, UAnimSequence* InAnim)
        : StateName(InName), AnimationToPlay(InAnim) {
    }

    void AddTransitionRule(int32 TargetStateIndex, FAnimationTransitionRule::TransitionConditionFunc ConditionFunc, FName RuleName = NAME_None)
    {
        Transitions.Emplace(TargetStateIndex, ConditionFunc, RuleName);
    }

    // 포즈 생성
    void Evaluate(float CurrentStateTime, FPoseContext& OutPoseContext, FAnimExtractContext& InOutExtractContext) const
    {
        if (AnimationToPlay && AnimationToPlay->GetDataModel())
        {
            InOutExtractContext.CurrentTime = CurrentStateTime;
            //InOutExtractContext.bLooping = AnimationToPlay->bLooping;

            AnimationToPlay->GetAnimationPose(OutPoseContext, InOutExtractContext);
        }
        else
        {
            // 애니메이션이 없으면 참조 포즈 또는 기본 포즈로 설정
            // OutPoseContext.ResetToRefPose(...);
        }
    }
};
