#include "AnimSequence.h"
#include "Classes/Animation/Skeleton.h"
#include "Core/Math/JungleMath.h"
#include "Classes/Components/Mesh/SkeletalMeshComponent.h"
#include "UserInterface/Console.h"

void UAnimSequence::GetAnimationPose(FPoseContext& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const
{
    if (!DataModel)
    {
        return;
    }

    // 현재 RefBonePose에 글로벌이 저장되어 있어 주석, 로컬 데이터 추가 시 아래 메서드로 변경
    /*OutAnimationPoseData.ResetToRefPose(
        OutAnimationPoseData.AnimInstance->CurrentSkeleton->ReferenceSkeleton.RefBonePose
    );*/

    USkeleton* Skeleton = OutAnimationPoseData.AnimInstance->GetSkelMeshComponent()->GetSkeletalMesh()->Skeleton;

    const int32 NumBones = Skeleton->BoneTree.Num();

    TArray<FMatrix> LocalTransforms;
    LocalTransforms.SetNum(NumBones);

    // Pose 추출
    OutAnimationPoseData.Pose.BonContainer.BoneLocalTransforms.Empty();

    for (int32 BoneIdx = 0; BoneIdx < NumBones; BoneIdx++)
    {
        OutAnimationPoseData.Pose.BonContainer.BoneLocalTransforms.Add(Skeleton->BoneTree[BoneIdx].BindTransform);
    }

    const TArray<FBoneAnimationTrack>& BoneTracks = DataModel->GetBoneAnimationTracks();
    const FFrameRate FrameRate = DataModel->GetFrameRate();
    const float FrameTime = ExtractionContext.CurrentTime * FrameRate.AsDecimal();

    for (const FBoneAnimationTrack& Track : BoneTracks)
    {
        const int32 BoneIndex = Skeleton->GetBoneIndex(Track.Name);

        if (BoneIndex == INDEX_NONE)
        {
            continue;
        }
        if (BoneTracks.Num() <= BoneIndex)
        {
            continue;
        }
        FString tt = BoneTracks[BoneIndex].Name.ToString();
        FString nn = Skeleton->BoneTree[BoneIndex].Name.ToString();

        // 키 프레임 보간
        int32 PrevKey = FMath::Clamp(FMath::FloorToInt(FrameTime), 0, Track.InternalTrackData.PosKeys.Num() - 1);
        int32 NextKey = FMath::Clamp(PrevKey + 1, 0, Track.InternalTrackData.PosKeys.Num() - 1);

        float Alpha = FMath::Clamp(FrameTime - PrevKey, 0.f, 1.f);

        // 변환 요소 보간
        const FVector Translation = FMath::Lerp(
            Track.InternalTrackData.PosKeys[PrevKey],
            Track.InternalTrackData.PosKeys[NextKey],
            Alpha
        );
        const FQuat Rotation = FQuat::Slerp(
            Track.InternalTrackData.RotKeys[PrevKey],
            Track.InternalTrackData.RotKeys[NextKey],
            Alpha
        );
        const FVector Scale = FMath::Lerp(
            Track.InternalTrackData.ScaleKeys[PrevKey],
            Track.InternalTrackData.ScaleKeys[NextKey],
            Alpha
        );

        OutAnimationPoseData.Pose.BonContainer.BoneLocalTransforms[BoneIndex] = JungleMath::CreateModelMatrix(Translation, Rotation, Scale);
    }

    // Curve 추출
    //OutAnimationPoseData.Curve.CurveElement.Empty();
    //const FAnimationCurveData& AnimationCurves = DataModel->GetCurveData();

    //for (const FTransformCurve& TransformCurve : AnimationCurves.TransformCurves)
    //{
    //    FName BaseCurveName = TransformCurve.GetName();
    //    // [TEMP] Current time ?
    //    FTransform EvaluatedTransform = TransformCurve.Evaluate(ExtractionContext.CurrentTime, 1.0f);

    //    OutAnimationPoseData.Curve.Set(FName(*(BaseCurveName.ToString() + TEXT("_TX"))), EvaluatedTransform.GetTranslation().X);
    //    OutAnimationPoseData.Curve.Set(FName(*(BaseCurveName.ToString() + TEXT("_TY"))), EvaluatedTransform.GetTranslation().Y);
    //    OutAnimationPoseData.Curve.Set(FName(*(BaseCurveName.ToString() + TEXT("_TZ"))), EvaluatedTransform.GetTranslation().Z);

    //    FRotator EulerRotation = EvaluatedTransform.GetRotation().Rotator();
    //    OutAnimationPoseData.Curve.Set(FName(*(BaseCurveName.ToString() + TEXT("_RX"))), EulerRotation.Roll);
    //    OutAnimationPoseData.Curve.Set(FName(*(BaseCurveName.ToString() + TEXT("_RY"))), EulerRotation.Pitch);
    //    OutAnimationPoseData.Curve.Set(FName(*(BaseCurveName.ToString() + TEXT("_RZ"))), EulerRotation.Yaw);

    //    OutAnimationPoseData.Curve.Set(FName(*(BaseCurveName.ToString() + TEXT("_SX"))), EvaluatedTransform.GetScale3D().X);
    //    OutAnimationPoseData.Curve.Set(FName(*(BaseCurveName.ToString() + TEXT("_SY"))), EvaluatedTransform.GetScale3D().Y);
    //    OutAnimationPoseData.Curve.Set(FName(*(BaseCurveName.ToString() + TEXT("_SZ"))), EvaluatedTransform.GetScale3D().Z);
    //}
}
