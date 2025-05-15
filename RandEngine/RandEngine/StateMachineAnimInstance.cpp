#include "StateMachineAnimInstance.h" 
#include "Animation/StateMachine.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "UObject/Casts.h"
#include "UObject/ObjectFactory.h"
#include "Engine/AssetManager.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "Contents/Actors/ARandCharacter.h"

// 원래는 Script 또는 Blueprint로 해야할 작업에 해당함
UStateMachineAnimInstance::UStateMachineAnimInstance() : StateMachine(nullptr)
{
    // [TEMP] Get asset
    IdleAnimation = Cast<UAnimSequence>(UAssetManager::Get().GetAnimationAsset(L"Contents/Idle_mixamo.com"));
    WalkAnimation = Cast<UAnimSequence>(UAssetManager::Get().GetAnimationAsset(L"Contents/Walk_mixamo.com"));;
    JumpAnimation = Cast<UAnimSequence>(UAssetManager::Get().GetAnimationAsset(L"Contents/Jump_mixamo.com"));;

    StateMachine = FObjectFactory::ConstructObject<UStateMachine>(this);
    StateMachine->OwningAnimInstance = this;

    // 상태 등록
    if (IdleAnimation)
    {
        int32 IdleStateIdx = StateMachine->AddState(TEXT("Idle"), IdleAnimation);
        StateMachine->SetEntryState(IdleStateIdx);
    }
    if (WalkAnimation)
    {
        StateMachine->AddState(TEXT("Walk"), WalkAnimation);
    }
    if (JumpAnimation)
    {
        StateMachine->AddState(TEXT("Jump"), JumpAnimation);
    }

    int32 IdleIdx = StateMachine->States.IndexOfByPredicate([](const FAnimationState& S) { return S.StateName == TEXT("Idle"); });
    int32 WalkIdx = StateMachine->States.IndexOfByPredicate([](const FAnimationState& S) { return S.StateName == TEXT("Walk"); });
    int32 JumpIdx = StateMachine->States.IndexOfByPredicate([](const FAnimationState& S) { return S.StateName == TEXT("Jump"); });

    // 전환 규칙 등록
    if (IdleIdx != INDEX_NONE && WalkIdx != INDEX_NONE) {
        StateMachine->AddTransition(IdleIdx, WalkIdx, &UStateMachineAnimInstance::Cond_CanWalk, TEXT("IdleToWalk"));
    }
    if (WalkIdx != INDEX_NONE && IdleIdx != INDEX_NONE) {
        StateMachine->AddTransition(WalkIdx, IdleIdx, &UStateMachineAnimInstance::Cond_CanIdle, TEXT("WalkToIdle"));
    }
    if (IdleIdx != INDEX_NONE && JumpIdx != INDEX_NONE) {
        StateMachine->AddTransition(IdleIdx, JumpIdx, &UStateMachineAnimInstance::Cond_CanJump, TEXT("IdleToJump"));
    }
    if (WalkIdx != INDEX_NONE && JumpIdx != INDEX_NONE) {
        StateMachine->AddTransition(WalkIdx, JumpIdx, &UStateMachineAnimInstance::Cond_CanJump, TEXT("WalkToJump"));
    }
    if (IdleIdx != INDEX_NONE && JumpIdx != INDEX_NONE) {
        StateMachine->AddTransition(JumpIdx, IdleIdx, &UStateMachineAnimInstance::Cond_CanIdle, TEXT("JumpToIdle"));
    }
    if (WalkIdx != INDEX_NONE && JumpIdx != INDEX_NONE) {
        StateMachine->AddTransition(JumpIdx, WalkIdx, &UStateMachineAnimInstance::Cond_CanWalk, TEXT("JumpToWalk"));
    }

    StateMachine->Initialize();
}

UStateMachineAnimInstance::~UStateMachineAnimInstance()
{
}

void UStateMachineAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds); // 부모의 업데이트 로직 (예: 시간 누적)

    USkeletalMeshComponent* SkelMeshComp = GetSkelMeshComponent();
    AActor* OwnerActor = SkelMeshComp ? SkelMeshComp->GetOwner() : nullptr;

    // Player Actor로 부터 데이터 업데이트
    if (ARandCharacter* Character = Cast<ARandCharacter>(OwnerActor)) {
        //bIsJumping = Character->GetCharacterMovement()->IsFalling();
    }

    if (StateMachine)
    {
        StateMachine->Update(DeltaSeconds);
        HandleStateMachineNotifies();
        // 스테이트 머신으로부터 포즈 추출
        FPoseContext StateMachineOutput(this); // 결과 저장 위치
        FAnimExtractContext ExtractContext;

        StateMachine->Evaluate(StateMachineOutput, ExtractContext);

        for (int32 i = 0; i < StateMachineOutput.Pose.BonContainer.BoneLocalTransforms.Num(); i++)
        {
            OwningComponent->GetSkeletalMesh()->SetBoneLocalMatrix(i, StateMachineOutput.Pose.BonContainer.BoneLocalTransforms[i]);
        }

        OwningComponent->GetSkeletalMesh()->UpdateWorldTransforms();
        OwningComponent->GetSkeletalMesh()->UpdateAndApplySkinning();
    }
    TriggerAnimNotifies(DeltaSeconds);
    TriggeredNotifyIDsThisCycle_StateMachine.Empty();

}

void UStateMachineAnimInstance::HandleStateMachineNotifies()
{
 
    if (!StateMachine || !OwningComponent || !OwningComponent->GetSkeletalMesh()) return;

    const FAnimationState* ActiveStateData = StateMachine->GetCurrentActiveState();
    if (!ActiveStateData || !ActiveStateData->AnimationToPlay)
    {
        return;
    }

    UAnimSequence* CurrentSequence = ActiveStateData->AnimationToPlay;
    const float CurrentAnimTime = ActiveStateData->CurrentTime;
    const float PreviousAnimTime = ActiveStateData->PreviousTime;
    const float AnimPlayRate = ActiveStateData->PlayRate; // UAnimSingleNodeInstance의 PlayRate와 동일한 역할

    const double PlayLength = CurrentSequence->GetDataModel()->GetPlayLength();

    if (PlayLength <= 0.0) return;
    // UAnimSingleNodeInstance의 노티파이 감지 로직을 여기에 적용
    for (const FAnimNotifyEvent& NotifyEvent : CurrentSequence->Notifies)
    {
        bool bShouldTrigger = false;

        // --- UAnimSingleNodeInstance의 bShouldTrigger 결정 로직 복사/붙여넣기 ---
        // 주의: CurrentTime, PreviousTime, PlayLength, bIsLooping, PlayRate 변수명을
        // 이 함수 스코프의 변수명(CurrentAnimTime, PreviousAnimTime 등)으로 맞춰야 함.

        if (PreviousAnimTime < CurrentAnimTime) // 정방향 재생
        {
            if (PreviousAnimTime <= NotifyEvent.TriggerTime && NotifyEvent.TriggerTime <= CurrentAnimTime)
            {
                bShouldTrigger = true;
            }
        }
        else if (PreviousAnimTime > CurrentAnimTime)
        {
            if (AnimPlayRate >= 0.f) // 정방향 루프
            {
                if ((NotifyEvent.TriggerTime > PreviousAnimTime && NotifyEvent.TriggerTime <= PlayLength) ||
                    (NotifyEvent.TriggerTime >= 0.0f && NotifyEvent.TriggerTime <= CurrentAnimTime))
                {
                    bShouldTrigger = true;
                }
            }
            else // 역방향 루프
            {
                if ((NotifyEvent.TriggerTime > PreviousAnimTime && NotifyEvent.TriggerTime <= PlayLength) ||
                    (NotifyEvent.TriggerTime >= 0.0f && NotifyEvent.TriggerTime <= CurrentAnimTime))
                {
                    bShouldTrigger = true;
                }
            }
        }

        if (!bShouldTrigger)
        {
            if (AnimPlayRate >= 0.f) // 정방향 또는 정지
            {
                // 루프가 아닌 일반 진행시 (PreviousAnimTime < CurrentAnimTime)
                if (PreviousAnimTime < NotifyEvent.TriggerTime && NotifyEvent.TriggerTime <= CurrentAnimTime)
                {
                    bShouldTrigger = true;
                }
            }
            else // 역방향 (AnimPlayRate < 0.f)
            {
                // 역방향 진행시 (CurrentAnimTime < PreviousAnimTime)
                if (CurrentAnimTime < NotifyEvent.TriggerTime && NotifyEvent.TriggerTime <= PreviousAnimTime)
                {
                    bShouldTrigger = true;
                }
            }
        }
        // --- bShouldTrigger 결정 로직 끝 ---

        if (bShouldTrigger)
        {
            if (!TriggeredNotifyIDsThisCycle_StateMachine.Contains(NotifyEvent.EventId))
            {
                if (NotifyQueue) // UAnimInstance의 멤버
                {
                    NotifyQueue->ActiveNotifies.Add(NotifyEvent);
                }
                TriggeredNotifyIDsThisCycle_StateMachine.Add(NotifyEvent.EventId);
            }
        }
    }
   
}

bool UStateMachineAnimInstance::Cond_CanWalk(const UAnimInstance* InAnimInstance)
{
    const UStateMachineAnimInstance* StateMachineInstance = Cast<UStateMachineAnimInstance>(InAnimInstance);
    return StateMachineInstance && StateMachineInstance->bIsMoving && !StateMachineInstance->bIsJumping;
}

bool UStateMachineAnimInstance::Cond_CanIdle(const UAnimInstance* InAnimInstance)
{
    const UStateMachineAnimInstance* StateMachineInstance = Cast<UStateMachineAnimInstance>(InAnimInstance);
    return StateMachineInstance && !StateMachineInstance->bIsMoving && !StateMachineInstance->bIsJumping;
}

bool UStateMachineAnimInstance::Cond_CanJump(const UAnimInstance* InAnimInstance)
{
    const UStateMachineAnimInstance* StateMachineInstance = Cast<UStateMachineAnimInstance>(InAnimInstance);
    return StateMachineInstance && StateMachineInstance->bIsJumping;
}
