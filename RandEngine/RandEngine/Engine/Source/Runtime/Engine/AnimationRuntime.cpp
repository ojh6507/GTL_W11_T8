#include "AnimationRuntime.h"
#include "Animation/AnimTypes.h"
#include "Core/Math/JungleMath.h"

void FAnimationRuntime::BlendTwoPosesTogether(
    const FCompactPose& SourcePose1,
    const FCompactPose& SourcePose2,
    const FBlendedCurve& SourceCurve1,
    const FBlendedCurve& SourceCurve2,
    const float			WeightOfPose1,
    /*out*/ FCompactPose& ResultPose,
    /*out*/ FBlendedCurve& ResultCurve)
{
    // 일단 테스트용 선형보간
    if (SourcePose1.BonContainer.BoneLocalTransforms.Num() != SourcePose2.BonContainer.BoneLocalTransforms.Num())
    {
        return;
    }

    int32 NumBones = SourcePose1.BonContainer.BoneLocalTransforms.Num();

    // 결과 포즈의 본 로컬 트랜스폼 배열 크기 조정
    ResultPose.BonContainer.BoneLocalTransforms.SetNum(NumBones);

    for (int32 i = 0; i < NumBones; ++i)
    {
        // FMatrix에서 Rotation, Translation, Scale을 추출
        FMatrix MatrixA = SourcePose1.BonContainer.BoneLocalTransforms[i];
        FMatrix MatrixB = SourcePose2.BonContainer.BoneLocalTransforms[i];

        // Matrix -> Translation, Rotation(Quat), Scale Vector로 분리
        FVector TranslationA = MatrixA.GetTranslationVector();
        FQuat RotationA = MatrixA.ToQuat();
        FVector ScaleA = MatrixA.GetScaleVector();

        FVector TranslationB = MatrixB.GetTranslationVector();
        FQuat RotationB = MatrixB.ToQuat();
        FVector ScaleB = MatrixB.GetScaleVector();

        // Translation, Rotation, Scale 보간
        FVector BlendedTranslation = FMath::Lerp(TranslationA, TranslationB, WeightOfPose1);
        FQuat BlendedRotation = FQuat::Slerp(RotationA, RotationB, WeightOfPose1);
        FVector BlendedScale = FMath::Lerp(ScaleA, ScaleB, WeightOfPose1);

        // 블렌딩된 Translation, Rotation, Scale을 다시 FMatrix로 Combine
        FMatrix BlendedMatrix = JungleMath::CreateModelMatrix(BlendedTranslation, BlendedRotation, BlendedScale);
        ResultPose.BonContainer.BoneLocalTransforms[i] = BlendedMatrix;
    }
    /*UE::Anim::FStackAttributeContainer TempAttributes;

    FAnimationPoseData AnimationPoseData = { ResultPose, ResultCurve, TempAttributes };

    const FAnimationPoseData SourceOnePoseData(const_cast<FCompactPose&>(SourcePose1), const_cast<FBlendedCurve&>(SourceCurve1), TempAttributes);
    const FAnimationPoseData SourceTwoPosedata(const_cast<FCompactPose&>(SourcePose2), const_cast<FBlendedCurve&>(SourceCurve2), TempAttributes);

    BlendTwoPosesTogether(SourceOnePoseData, SourceTwoPosedata, WeightOfPose1, AnimationPoseData);*/
}

void FAnimationRuntime::BlendTwoPosesTogether(const FAnimationPoseData& SourcePoseOneData, const FAnimationPoseData& SourcePoseTwoData, const float WeightOfPoseOne, /*out*/ FAnimationPoseData& OutAnimationPoseData)
{
    // 일단 테스트용 선형보간
    // 
    //FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    //FBlendedCurve& OutCurve = OutAnimationPoseData.GetCurve();
    //UE::Anim::FStackAttributeContainer& OutAttributes = OutAnimationPoseData.GetAttributes();

    //const float WeightOfPoseTwo = 1.f - WeightOfPoseOne;

    //BlendPose<ETransformBlendMode::Overwrite>(SourcePoseOneData.GetPose(), OutPose, WeightOfPoseOne);
    //BlendPose<ETransformBlendMode::Accumulate>(SourcePoseTwoData.GetPose(), OutPose, WeightOfPoseTwo);

    //// Ensure that all of the resulting rotations are normalized
    //OutPose.NormalizeRotations();

    //OutCurve.Lerp(SourcePoseOneData.GetCurve(), SourcePoseTwoData.GetCurve(), WeightOfPoseTwo);
    //UE::Anim::Attributes::BlendAttributes({ SourcePoseOneData.GetAttributes(), SourcePoseTwoData.GetAttributes() }, { WeightOfPoseOne, WeightOfPoseTwo }, { 0, 1 }, OutAttributes);
}
