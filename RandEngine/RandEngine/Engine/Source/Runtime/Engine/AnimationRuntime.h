#pragma once

struct FCompactPose;
struct FBlendedCurve;
struct FAnimationPoseData;

class FAnimationRuntime
{
public:
   static void BlendTwoPosesTogether(
        const FCompactPose& SourcePose1,
        const FCompactPose& SourcePose2,
        const FBlendedCurve& SourceCurve1,
        const FBlendedCurve& SourceCurve2,
        const float WeightOfPose1,
        /*out*/ FCompactPose& ResultPose,
        /*out*/ FBlendedCurve& ResultCurve);
   
   static void BlendTwoPosesTogether(const FAnimationPoseData& SourcePoseOneData, const FAnimationPoseData& SourcePoseTwoData, const float WeightOfPoseOne, FAnimationPoseData& OutAnimationPoseData);
};
