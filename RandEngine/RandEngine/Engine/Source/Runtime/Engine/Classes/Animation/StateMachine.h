#include "Core/Container/Array.h"
#include "UObject/NameTypes.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimStateMachineTypes.h"

class UAnimInstance;
class UAnimSequence;
struct FPoseContext;
struct FAnimExtractContext;

class UStateMachine : public UObject
{
    DECLARE_CLASS(UStateMachine, UObject)
public:
    TArray<FAnimationState> States;
    int32 EntryStateIndex;
    int32 CurrentStateIndex;
    float TimeInCurrentState;
    UAnimInstance* OwningAnimInstance;

    // Memebers for Blend
    bool bIsBlending;
    int32 PreviousStateIndex;
    float BlendDuration;
    float CurrentBlendTime;
    FCompactPose SourceBlendPose;
    FBlendedCurve SourceBlendCurve;

public:
    UStateMachine();
    UStateMachine(UAnimInstance* InOwningAnimInstance);
    ~UStateMachine();

    int32 AddState(FName StateName, UAnimSequence* Animation);
    void AddTransition(int32 FromStateIdx, int32 ToStateIdx, FAnimationTransitionRule::TransitionConditionFunc Condition, FName RuleName = NAME_None);
    void SetEntryState(int32 StateIdx);
    void Initialize();
    void Update(float DeltaTime);
    void Evaluate(FPoseContext& OutPoseContext, FAnimExtractContext& InOutExtractContext);
    FName GetCurrentStateName() const;
    const FAnimationState* GetCurrentActiveState() const;
private:
    void StartBlending(int32 InTargetStateIndex);
};
