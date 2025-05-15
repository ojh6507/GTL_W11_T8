#include "Animation/AnimationAsset.h"
#include "AnimSingleNodeInstance.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "Userinterface/Console.h"
#include "Animation/AnimSequence.h"
#include "UObject/Casts.h"
#include "Math/JungleMath.h"
#include "SoundManager.h"

UAnimSingleNodeInstance::UAnimSingleNodeInstance()
{
    CurrentTime = 0.0f;
}

UAnimSingleNodeInstance::~UAnimSingleNodeInstance()
{
}

void UAnimSingleNodeInstance::SetAnimationAsset(class UAnimationAsset* NewAsset, bool bInIsLooping, float InPlayRate)
{
    if (NewAsset != CurrentAsset)
    {
        CurrentAsset = NewAsset;
        CurrentTime = 0.0f;
        PreviousTime = 0.0f;
        if (NotifyQueue) NotifyQueue->ActiveNotifies.Empty();
        NotifyActionMap.Empty();
        TriggeredNotifyIDsThisCycle.Empty();
        FSoundManager::GetInstance().StopAllSounds();
    }
    // Proxy 사용하지 않을 듯
    //FAnimSingleNodeInstanceProxy& Proxy = GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>();

    USkeletalMeshComponent* MeshComponent = GetSkelMeshComponent();
    if (MeshComponent)
    {
        if (MeshComponent->GetSkeletalMeshAsset() == nullptr)
        {
            // if it does not have SkeletalMesh, we nullify it
            CurrentAsset = nullptr;
        }
        else if (CurrentAsset != nullptr)
        {
            // if we have an asset, make sure their skeleton is valid, otherwise, null it
            if (CurrentAsset->GetSkeleton() == nullptr)
            {
                // clear asset since we do not have matching skeleton
                CurrentAsset = nullptr;
            }
        }
    }

    //Proxy.SetAnimationAsset(NewAsset, GetSkelMeshComponent(), bInIsLooping, InPlayRate);
    bIsLooping = bInIsLooping;
    PlayRate = InPlayRate;

    // if composite, we want to make sure this is valid
    // this is due to protect recursive created composite
    // however, if we support modifying asset outside of viewport, it will have to be called whenever modified
    /*if (UAnimCompositeBase* CompositeBase = Cast<UAnimCompositeBase>(NewAsset))
    {
        CompositeBase->InvalidateRecursiveAsset();
    }*/
}

void UAnimSingleNodeInstance::SetLooping(bool bInIsLooping)
{
    //FAnimSingleNodeInstanceProxy& Proxy = GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>();
    //Proxy.SetLooping(bIsLooping);
    bIsLooping = bInIsLooping;
}

void UAnimSingleNodeInstance::SetPlaying(bool bInIsPlaying)
{
    //FAnimSingleNodeInstanceProxy& Proxy = GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>();
    //Proxy.SetPlaying(bIsPlaying);
    bIsPlaying = bInIsPlaying;
}

bool UAnimSingleNodeInstance::IsPlaying() const
{
    //return GetProxyOnGameThread<FAnimSingleNodeInstanceProxy>().IsPlaying();
    return bIsPlaying;
}

void UAnimSingleNodeInstance::ResetToReferencePose()
{
    USkeleton* Skeleton = OwningComponent->GetSkeletalMesh()->Skeleton;

    const int32 NumBones = Skeleton->BoneTree.Num();

    TArray<FMatrix> LocalTransforms;
    LocalTransforms.SetNum(NumBones);

    for (int32 BoneIdx = 0; BoneIdx < NumBones; BoneIdx++)
    {
        LocalTransforms[BoneIdx] = Skeleton->BoneTree[BoneIdx].BindTransform;
    }

    for (int32 i = 0; i < LocalTransforms.Num(); i++)
    {
        OwningComponent->GetSkeletalMesh()->SetBoneLocalMatrix(i, LocalTransforms[i]);
    }

    OwningComponent->GetSkeletalMesh()->UpdateWorldTransforms();
    OwningComponent->GetSkeletalMesh()->UpdateAndApplySkinning();
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    if (!CurrentAsset || !bIsPlaying)
    {
        return;
    }


    if (bUseExternalTime)
    {
        CurrentTime = ExternalTime;
    }
    else
    {
        CurrentTime += DeltaSeconds * PlayRate;
    }
    UAnimSequence* CurrentSequence = Cast<UAnimSequence>(CurrentAsset);
    const double PlayLengthD = CurrentSequence ? CurrentSequence->GetDataModel()->GetPlayLength() : 0.0;
    const float ActualPlayLength = static_cast<float>(PlayLengthD);

    if (ActualPlayLength > 0.0f)
    {
        TriggeredNotifyIDsThisCycle.Empty();
        if (bIsLooping)
        {
            CurrentTime = FMath::Fmod(CurrentTime, ActualPlayLength);
            if (CurrentTime < 0.0f)
            {
                CurrentTime += ActualPlayLength;
            }
        }
        else
        {
            float ClampedTime = FMath::Clamp(CurrentTime, 0.0f, ActualPlayLength);
            if (FMath::Abs(CurrentTime - ClampedTime) > KINDA_SMALL_NUMBER && !bUseExternalTime)
            {
                bIsPlaying = false;
            }
            CurrentTime = ClampedTime;
        }
    }
    else
    {
        CurrentTime = 0.0f;
        bIsPlaying = false;
    }

    if (!bIsPlaying && !bUseExternalTime)
    {
        PreviousTime = CurrentTime;
        return;
    }

    if (CurrentSequence)
    {
        for (const FAnimNotifyEvent& NotifyEvent : CurrentSequence->Notifies)
        {
            bool bShouldTrigger = false;
            const float NTime = NotifyEvent.TriggerTime;

            if (PlayRate >= 0.f)
            {
                if (PreviousTime <= CurrentTime)
                {
                    if (NTime >= PreviousTime && NTime < CurrentTime)
                    {
                        bShouldTrigger = true;
                    }
                }
                else
                {
                    if ((NTime >= PreviousTime && NTime < ActualPlayLength) ||
                        (NTime >= 0.0f && NTime < CurrentTime))
                    {
                        bShouldTrigger = true;
                    }
                }
            }
            else
            {
                if (CurrentTime < PreviousTime)
                {
                    if (PreviousTime >= NTime && CurrentTime < NTime)
                    {
                        bShouldTrigger = true;
                    }
                }
                else if (bIsLooping) // 루프
                {
                    if ((NTime > CurrentTime) || (NTime <= PreviousTime))
                    {
                        bShouldTrigger = true;
                    }
                }
            }

            if (CurrentTime == PreviousTime)
            {
                if (NTime == CurrentTime)
                {
                    bShouldTrigger = true;
                }
            }
            else if (NTime == CurrentTime) {
                if (PlayRate >= 0.f && PreviousTime < CurrentTime) {
                    bShouldTrigger = true;
                }
                else if (PlayRate < 0.f && PreviousTime > CurrentTime) {
                    bShouldTrigger = true;
                }
            }

            if (bShouldTrigger)
            {
                if (!TriggeredNotifyIDsThisCycle.Contains(NotifyEvent.EventId))
                {
                    if (NotifyQueue)
                    {
                        NotifyQueue->ActiveNotifies.Add(NotifyEvent);
                    }
                    TriggeredNotifyIDsThisCycle.Add(NotifyEvent.EventId);
                }
            }
        }
    }

    if (CurrentSequence && OwningComponent && OwningComponent->GetSkeletalMesh())
    {
        FPoseContext Pose(this);
        FAnimExtractContext Extract(CurrentTime, false);
        CurrentSequence->GetAnimationPose(Pose, Extract);

        for (int32 i = 0; i < Pose.Pose.BonContainer.BoneLocalTransforms.Num(); i++)
        {
            OwningComponent->GetSkeletalMesh()->SetBoneLocalMatrix(i, Pose.Pose.BonContainer.BoneLocalTransforms[i]);
        }

        OwningComponent->GetSkeletalMesh()->UpdateWorldTransforms();
        OwningComponent->GetSkeletalMesh()->UpdateAndApplySkinning();
    }

    TriggerAnimNotifies(CurrentTime);

    PreviousTime = CurrentTime;
}
