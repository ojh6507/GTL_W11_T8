#pragma once

#include "Animation/AnimInstance.h"

class UStateMachine;
class UAnimSequence;

class UStateMachineAnimInstance : public UAnimInstance {
    DECLARE_CLASS(UStateMachineAnimInstance, UAnimInstance)

public:
    // 스테이트 머신 전환 조건에 사용될 변수들
    bool bIsMoving = false;
    bool bIsJumping = false;
    float Speed = 0.0f;

    // 상태에서 사용될 애니메이션 에셋들
    UAnimSequence* IdleAnimation;
    UAnimSequence* WalkAnimation;
    UAnimSequence* JumpAnimation;

private:
    UStateMachine* StateMachine;
    TSet<uint32> TriggeredNotifyIDsThisCycle_StateMachine;
public:
    UStateMachineAnimInstance();
    virtual ~UStateMachineAnimInstance();

    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    void HandleStateMachineNotifies();
    // 전환 조건 함수들
    static bool Cond_CanWalk(const UAnimInstance* InAnimInstance);
    static bool Cond_CanIdle(const UAnimInstance* InAnimInstance);
    static bool Cond_CanJump(const UAnimInstance* InAnimInstance);
};
