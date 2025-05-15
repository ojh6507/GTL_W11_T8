#include "StateMachine.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Userinterface/Console.h"
#include "Engine/AnimationRuntime.h"

UStateMachine::UStateMachine()
    : EntryStateIndex(INDEX_NONE),
    CurrentStateIndex(INDEX_NONE),
    TimeInCurrentState(0.0f),
    OwningAnimInstance(nullptr),
    bIsBlending(false),
    PreviousStateIndex(INDEX_NONE),
    BlendDuration(0.4f),
    CurrentBlendTime(0.0f)
{
}

UStateMachine::UStateMachine(UAnimInstance* InOwningAnimInstance)
    : EntryStateIndex(INDEX_NONE),
    CurrentStateIndex(INDEX_NONE),
    TimeInCurrentState(0.0f),
    OwningAnimInstance(InOwningAnimInstance),
    bIsBlending(false),
    PreviousStateIndex(INDEX_NONE),
    BlendDuration(0.4f),
    CurrentBlendTime(0.0f)
{
}

UStateMachine::~UStateMachine()
{
}

int32 UStateMachine::AddState(FName StateName, UAnimSequence* InAnimaSequence)
{
    States.Emplace(StateName, InAnimaSequence);
    return States.Num() - 1;
}

void UStateMachine::AddTransition(int32 FromStateIdx, int32 ToStateIdx, FAnimationTransitionRule::TransitionConditionFunc Condition, FName RuleName)
{
    if (States.IsValidIndex(FromStateIdx) && States.IsValidIndex(ToStateIdx))
    {
        States[FromStateIdx].AddTransitionRule(ToStateIdx, Condition, RuleName);
    }
    else
    {
         UE_LOG(ELogLevel::Display, TEXT("UStateMachine::AddTransition: Invalid state index. From: %d, To: %d"), FromStateIdx, ToStateIdx);
    }
}

void UStateMachine::SetEntryState(int32 StateIdx)
{
    if (States.IsValidIndex(StateIdx))
    {
        EntryStateIndex = StateIdx;
    }
}

void UStateMachine::Initialize()
{
    if (States.IsValidIndex(EntryStateIndex))
    {
        CurrentStateIndex = EntryStateIndex;
        TimeInCurrentState = 0.0f;
    }
    else if (States.Num() > 0)
    {
        CurrentStateIndex = 0;
        TimeInCurrentState = 0.0f;
    }
}

void UStateMachine::Update(float DeltaTime)
{
    if (!OwningAnimInstance || !States.IsValidIndex(CurrentStateIndex))
    {
        return;
    }

    if (bIsBlending)
    {
        CurrentBlendTime += DeltaTime;
        if (CurrentBlendTime >= BlendDuration)
        {
            bIsBlending = false;
            PreviousStateIndex = INDEX_NONE; // 이전 상태 정보 초기화
        }
        TimeInCurrentState += DeltaTime;
    }
    else 
    {
        TimeInCurrentState += DeltaTime;

        const FAnimationState& ActiveState = States[CurrentStateIndex];

        for (const FAnimationTransitionRule& Rule : ActiveState.Transitions)
        {
            if (Rule.CanEnter(OwningAnimInstance))
            {
                if (States.IsValidIndex(Rule.TransitionIndex)) // 목표 상태 유효성 검사
                {
                    UE_LOG(ELogLevel::Display, TEXT("State Transition: From '%s' To '%s' via Rule '%s'"),
                        *ActiveState.StateName.ToString(),
                        *States[Rule.TransitionIndex].StateName.ToString(),
                        *Rule.RuleToExecute.ToString());
                }

                StartBlending(Rule.TransitionIndex);
                break; // 첫 번째 유효한 전환을 바로 적용
            }
        }
    }
}

void UStateMachine::Evaluate(FPoseContext& OutPoseContext, FAnimExtractContext& InOutExtractContext)
{
    if (!States.IsValidIndex(CurrentStateIndex))
    {
        return;
    }

    // On Blending
    if (bIsBlending && States.IsValidIndex(PreviousStateIndex))
    {
        float BlendAlpha = FMath::Clamp(CurrentBlendTime / BlendDuration, 0.0f, 1.0f);

        FPoseContext TargetPoseContext(this->OwningAnimInstance);
        FAnimExtractContext TargetExtractContext = InOutExtractContext;

        const FAnimationState& TargetState = States[CurrentStateIndex];
        if (TargetState.AnimationToPlay && TargetState.AnimationToPlay->GetDataModel())
        {
            const float TargetPlayLength = static_cast<float>(TargetState.AnimationToPlay->GetDataModel()->GetPlayLength());
            float TargetSampleTime = TimeInCurrentState; 
            if (TargetPlayLength > 0.f) 
            { 
                TargetSampleTime = FMath::Fmod(TimeInCurrentState, TargetPlayLength);
            }
            else if (TargetPlayLength > 0.f) {
                TargetSampleTime = FMath::Clamp(TimeInCurrentState, 0.f, TargetPlayLength);
            }
            else {
                TargetSampleTime = 0.f;
            }
            TargetExtractContext.CurrentTime = TargetSampleTime;
            TargetExtractContext.bLooping = true;

            TargetState.Evaluate(TargetSampleTime, TargetPoseContext, TargetExtractContext);
        }
        else
        {
            // 목표 상태에 애니메이션이 없는 케이스
        }

        FAnimationRuntime::BlendTwoPosesTogether(
            SourceBlendPose, TargetPoseContext.Pose,
            SourceBlendCurve, TargetPoseContext.Curve,
            BlendAlpha,
            OutPoseContext.Pose, OutPoseContext.Curve);
    }
    else // Not on Blending
    {
        FAnimationState& ActiveState = States[CurrentStateIndex];
        if (ActiveState.AnimationToPlay && ActiveState.AnimationToPlay->GetDataModel())
        {
            const float PlayLength = static_cast<float>(ActiveState.AnimationToPlay->GetDataModel()->GetPlayLength());
            float SampleTime = TimeInCurrentState;
            if (PlayLength > 0.f) { 
                SampleTime = FMath::Fmod(TimeInCurrentState, PlayLength);
            }
            else if (PlayLength > 0.f) {
                SampleTime = FMath::Clamp(TimeInCurrentState, 0.f, PlayLength);
            }
            else {
                SampleTime = 0.f;
            }
            InOutExtractContext.CurrentTime = SampleTime;
            InOutExtractContext.bLooping = true;

            ActiveState.PreviousTime = ActiveState.CurrentTime;
            ActiveState.CurrentTime = SampleTime;
            ActiveState.Evaluate(SampleTime, OutPoseContext, InOutExtractContext);
        }
        else
        {
            // 현재 상태에 애니메이션 없는 케이스
        }
    }
}

FName UStateMachine::GetCurrentStateName() const
{
    if (States.IsValidIndex(CurrentStateIndex))
    {
        return States[CurrentStateIndex].StateName;
    }
    return NAME_None;
}

const FAnimationState* UStateMachine::GetCurrentActiveState() const
{
    if (States.IsValidIndex(CurrentStateIndex))
    {
        return &States[CurrentStateIndex];
    }
    return nullptr;
}

void UStateMachine::StartBlending(int32 InTargetStateIndex)
{
    if (!States.IsValidIndex(CurrentStateIndex) || !States.IsValidIndex(InTargetStateIndex))
    {
        return;
    }

    bIsBlending = true;
    PreviousStateIndex = CurrentStateIndex;
    CurrentStateIndex = InTargetStateIndex;
    CurrentBlendTime = 0.0f;
    TimeInCurrentState = 0.0f;

    FPoseContext PrevPoseContext(this->OwningAnimInstance);
    FAnimExtractContext PrevExtractContext;

    float PrevSampleTime = TimeInCurrentState;
    const FAnimationState& PrevState = States[PreviousStateIndex];
    if (PrevState.AnimationToPlay) {
        const float PrevPlayLength = static_cast<float>(PrevState.AnimationToPlay->GetDataModel()->GetPlayLength());
        if (PrevPlayLength > 0.f) {
            PrevSampleTime = FMath::Fmod(TimeInCurrentState, PrevPlayLength);
        }
    }

    States[PreviousStateIndex].Evaluate(PrevSampleTime, PrevPoseContext, PrevExtractContext);
    SourceBlendPose = PrevPoseContext.Pose;
}
