#include "MyAnimInstance.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimSequence.h"
#include "Engine/AnimationRuntime.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"

// Test animation instance for blending
UMyAnimInstance::UMyAnimInstance()
    : AnimA(nullptr)
    , AnimB(nullptr)
    , BlendAlpha(0.0f)
    , bIsPlaying(false)
{
}

UMyAnimInstance::~UMyAnimInstance()
{
}

void UMyAnimInstance::SetLooping(bool bInIsLooping)
{
    bIsLooping = bInIsLooping;
}

void UMyAnimInstance::SetPlaying(bool bInIsPlaying)
{
    bIsPlaying = bInIsPlaying;
}

void UMyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!AnimA || !AnimB || !bIsPlaying)
    {
        return;
    }

    USkeleton* Skeleton = OwningComponent->GetSkeletalMesh()->Skeleton;

    if (!Skeleton)
    {
        return;
    }

    // 일단 A 애니메이션의 실행시간에 종속적으로 테스트 하도록 함
    CurrentTime += DeltaSeconds * PlayRate;
    //CurrentTime += DeltaSeconds * 1.0f;

    const double PlayLengthA = AnimA->GetDataModel()->GetPlayLength();
    float SampleTimeA = static_cast<float>(CurrentTime); // AnimA를 샘플링할 실제 시간

    if (PlayLengthA > 0.0)
    {
        if (bIsLooping) // bIsLooping은 AnimA의 루핑 여부
        {
            SampleTimeA = FMath::Fmod(SampleTimeA, static_cast<float>(PlayLengthA));
            if (SampleTimeA < 0.0f)
            {
                SampleTimeA += static_cast<float>(PlayLengthA);
            }
        }
        else
        {
            SampleTimeA = FMath::Clamp(SampleTimeA, 0.0f, static_cast<float>(PlayLengthA));
            if ((PlayRate > 0.0f && SampleTimeA >= PlayLengthA) ||
                (PlayRate < 0.0f && SampleTimeA <= 0.0f))
            {
                bIsPlaying = false; 
            }
        }
    }
    else // PlayLengthA가 0 또는 음수인 경우
    {
        SampleTimeA = 0.0f;
        if (!bIsLooping)
        {
            bIsPlaying = false;
        }
    }

    float SampleTimeB = static_cast<float>(CurrentTime);
    const double PlayLengthB = AnimB->GetDataModel()->GetPlayLength();

    if (PlayLengthA > 0.0 && PlayLengthB > 0.0)
    {
        SampleTimeB = FMath::Fmod(SampleTimeB, static_cast<float>(PlayLengthB));
        if (SampleTimeB < 0.0f)
        {
            SampleTimeB += static_cast<float>(PlayLengthB);
        }
    }

    if (AnimA && AnimB)
    {
        FPoseContext PoseA(this);
        FPoseContext PoseB(this);

        FAnimExtractContext ExtractA(SampleTimeA, false);
        FAnimExtractContext ExtractB(SampleTimeB, false);

        AnimA->GetAnimationPose(PoseA, ExtractA);
        AnimB->GetAnimationPose(PoseB, ExtractB);

        FAnimationRuntime::BlendTwoPosesTogether(
            PoseA.Pose, PoseB.Pose,
            PoseA.Curve, PoseB.Curve,
            BlendAlpha, /*Out*/ PoseA.Pose, /*Out*/ PoseA.Curve);

        // 결과 포즈를 출력
        //Output.Pose = PoseA.Pose;
        //Output.Curve = PoseA.Curve;

        for (int32 i = 0; i < PoseA.Pose.BonContainer.BoneLocalTransforms.Num(); i++)
        {
            OwningComponent->GetSkeletalMesh()->SetBoneLocalMatrix(i, PoseA.Pose.BonContainer.BoneLocalTransforms[i]);
        }

        OwningComponent->GetSkeletalMesh()->UpdateWorldTransforms();
        OwningComponent->GetSkeletalMesh()->UpdateAndApplySkinning();
    }

    // Animation blending
    //if (AnimA && AnimB)
    //{
    //    FPoseContext PoseA(this);
    //    FPoseContext PoseB(this);

    //    FAnimExtractContext ExtractA(CurrentTime, false);
    //    FAnimExtractContext ExtractB(CurrentTime, false);

    //    AnimA->GetAnimationPose(PoseA, ExtractA);
    //    AnimB->GetAnimationPose(PoseB, ExtractB);

    //    FAnimationRuntime::BlendTwoPosesTogether(
    //        PoseA.Pose, PoseB.Pose,
    //        PoseA.Curve, PoseB.Curve,
    //        BlendAlpha, /*Out*/ PoseA.Pose, /*Out*/ PoseA.Curve);

    //    // 결과 포즈를 출력
    //    //Output.Pose = PoseA.Pose;
    //    //Output.Curve = PoseA.Curve;

    //    for (int32 i = 0; i < PoseA.Pose.BonContainer.BoneLocalTransforms.Num(); i++)
    //    {
    //        OwningComponent->GetSkeletalMesh()->SetBoneLocalMatrix(i, PoseA.Pose.BonContainer.BoneLocalTransforms[i]);
    //    }

    //    OwningComponent->GetSkeletalMesh()->UpdateWorldTransforms();
    //    OwningComponent->GetSkeletalMesh()->UpdateAndApplySkinning();
    //}
}
