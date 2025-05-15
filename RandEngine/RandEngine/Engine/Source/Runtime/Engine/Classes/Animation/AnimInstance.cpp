// CustomAnimInstance.cpp
#include "AnimInstance.h"
#include "Animation/AnimTypes.h"
#include "UObject/Casts.h"
#include "SoundManager.h"

UAnimInstance::UAnimInstance()
{
    NotifyQueue = new FAnimNotifyQueue();
}

UAnimInstance::~UAnimInstance()
{
    if (NotifyQueue)
    {
        delete NotifyQueue;
        NotifyQueue = nullptr;
    }
}

void UAnimInstance::Initialize()
{
    // 뼈 데이터 초기화
    if (OwningComponent)
    {
        //RequiredBones.BoneNames = OwningComponent->GetBoneNames();
        //RequiredBones.ParentIndices = OwningComponent->GetBoneParentIndices();
    }
   
}

void UAnimInstance::Update(float DeltaTime)
{
    //AnimProxy->Update(DeltaTime);
}

void UAnimInstance::PostEvaluate()
{
}

void UAnimInstance::TriggerAnimNotifies(float DeltaTime)
{
    if (NotifyQueue)
    {
        for (const FAnimNotifyEvent& NotifyToTrigger : NotifyQueue->ActiveNotifies)
        {
            // --- 3. 각 노티파이에 대한 실제 액션 수행 (HandleNotify 역할) ---
            if (NotifyActionMap.Contains(NotifyToTrigger.NotifyName))
            {
                NotifyActionMap[NotifyToTrigger.NotifyName].Broadcast(NotifyToTrigger.NotifyName);
            }
            TriggerSoundNotifies(NotifyToTrigger); // 사운드 노티파이 처리
        }
        NotifyQueue->ActiveNotifies.Empty(); // 처리 후 큐 비우기
    }
}

void UAnimInstance::TriggerSoundNotifies(const FAnimNotifyEvent& NotifyToTrigger)
{
    if (!NotifyToTrigger.SoundNameToPlay.IsEmpty())
    {
        FSoundManager::GetInstance().PlaySound(NotifyToTrigger.SoundNameToPlay.ToAnsiString(),true);
    }
}

//void UAnimInstance::HandleNotify(const FAnimNotifyEvent& NotifyEvent)
//{
//}

USkeletalMeshComponent* UAnimInstance::GetSkelMeshComponent() const
{
    //return CastChecked<USkeletalMeshComponent>(GetOuter());
    return OwningComponent;
}

void UAnimInstance::UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion)
{
    NativeUpdateAnimation(DeltaSeconds);
}
