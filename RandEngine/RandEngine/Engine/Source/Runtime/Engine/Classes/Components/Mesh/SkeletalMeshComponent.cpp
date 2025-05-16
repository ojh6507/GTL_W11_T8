#include "SkeletalMeshComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/Asset/SkeletalMeshAsset.h"
#include "Engine/FbxLoader.h"
#include "Engine/SkeletalMeshActor.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "MyAnimInstance.h"
#include "StateMachineAnimInstance.h"

USkeletalMeshComponent::USkeletalMeshComponent()
{
    SkeletalMesh = nullptr;
    selectedSubMeshIndex = -1;
    AnimScriptInstance = FObjectFactory::ConstructObject<UAnimSingleNodeInstance>(nullptr);
    // [TEMP] Animation blend test
    //AnimScriptInstance = FObjectFactory::ConstructObject<UMyAnimInstance>(nullptr);
    AnimScriptInstance->SetOwningComponent(this);
}

UObject* USkeletalMeshComponent::Duplicate(UObject* InOuter)
{
    ThisClass* NewComponent = Cast<ThisClass>(Super::Duplicate(InOuter));
    // TODO: 이후 애니메이션 상태 등 복사 시 추가 구현
    NewComponent->SkeletalMesh = Cast<USkeletalMesh>(SkeletalMesh->Duplicate(InOuter));
    return NewComponent;
}

void USkeletalMeshComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);
    // [TEMP] test for animation
    TickAnimation(DeltaTime, true);
}

void USkeletalMeshComponent::GetProperties(TMap<FString, FString>& OutProperties) const
{
    Super::GetProperties(OutProperties);

    if (SkeletalMesh)
    {
        FString PathFString = FString(SkeletalMesh->GetRenderData()->FilePath);
        OutProperties.Add(TEXT("SkeletalMeshPath"), PathFString);
    }
    else
    {
        OutProperties.Add(TEXT("SkeletalMeshPath"), TEXT("None"));
    }
}

void USkeletalMeshComponent::SetProperties(const TMap<FString, FString>& InProperties)
{
    Super::SetProperties(InProperties);

    const FString* MeshPath = InProperties.Find(TEXT("SkeletalMeshPath"));
    if (MeshPath)
    {
        if (*MeshPath != TEXT("None"))
        {
            if (USkeletalMesh* LoadedMesh = UAssetManager::Get().GetSkeletalMesh(GetData(*MeshPath)))
            {
                SetSkeletalMesh(LoadedMesh);
                UE_LOG(ELogLevel::Display, TEXT("Set SkeletalMesh '%s' for %s"), **MeshPath, *GetName());
            }
            else
            {
                UE_LOG(ELogLevel::Warning, TEXT("Could not load SkeletalMesh '%s' for %s"), **MeshPath, *GetName());
                SetSkeletalMesh(nullptr);
            }
        }
        else
        {
            SetSkeletalMesh(nullptr);
        }
    }
}

int USkeletalMeshComponent::CheckRayIntersection(const FVector& InRayOrigin, const FVector& InRayDirection, float& OutHitDistance) const
{
    if (!SkeletalMesh)
    {
        return 0;
    }

    if (!AABB.Intersect(InRayOrigin, InRayDirection, OutHitDistance))
    {
        return 0;
    }

    const auto* RenderData = SkeletalMesh->GetRenderData();
    const auto BindVertices = RenderData->Vertices;
    TArray<FSkeletalMeshVertex> SkinnedVertices;
    const auto& Indices = RenderData->Indices;

    USkeleton* Skeleton = SkeletalMesh->Skeleton;
    SkinnedVertices.SetNum(BindVertices.Num());
    for (int32 i = 0; i < BindVertices.Num(); ++i)
    {
        const FSkeletalMeshVertex& BindVertex = BindVertices[i];
        FSkeletalMeshVertex& SkinnedVertex = SkinnedVertices[i];
        SkinnedVertex = BindVertex;

        // 해당 정점에 영향을 주는 본이 없으면 바인드 포즈 그대로 사용
        bool bHasInfluence = false;
        for (int32 j = 0; j < MAX_BONE_INFLUENCES; ++j)
        {
            if (BindVertex.BoneWeights[j] > KINDA_SMALL_NUMBER &&
                Skeleton->BoneTree.IsValidIndex(static_cast<int32>(BindVertex.BoneIndices[j])))
            {
                bHasInfluence = true;
                break;
            }
        }

        if (!bHasInfluence) continue;

        // 가중 합산을 위한 초기화
        FVector SkinnedPosition = FVector::ZeroVector;
        FVector SkinnedNormal = FVector::ZeroVector;

        // 각 본의 영향력을 합산
        float TotalWeight = 0.0f;
        for (int32 j = 0; j < MAX_BONE_INFLUENCES; ++j)
        {
            float Weight = BindVertex.BoneWeights[j];
            if (Weight <= KINDA_SMALL_NUMBER) continue;

            int32 BoneIndex = static_cast<int32>(BindVertex.BoneIndices[j]);
            if (!Skeleton->BoneTree.IsValidIndex(BoneIndex)) continue;

            TotalWeight += Weight;

            // 스키닝 행렬 적용
            const FMatrix& SkinMatrix = Skeleton->CurrentPose.SkinningMatrices[BoneIndex];

            // 위치 변환
            SkinnedPosition += SkinMatrix.TransformPosition(BindVertex.Position) * Weight;

            // 법선 변환 (위치 변환과 다름)
            FMatrix NormalMatrix = SkinMatrix;
            NormalMatrix.RemoveTranslation();

            if (FMath::Abs(NormalMatrix.Determinant()) > SMALL_NUMBER)
            {
                // 법선 벡터 변환을 위한 역전치(inverse transpose) 행렬 계산
                FMatrix InvTranspose = FMatrix::Transpose(FMatrix::Inverse(NormalMatrix));
                SkinnedNormal += FMatrix::TransformVector(BindVertex.Normal, InvTranspose) * Weight;
            }
            else
            {
                SkinnedNormal += BindVertex.Normal * Weight;
            }
        }

        // 가중치 합이 1에 가까워야 함 (부동소수점 오차 허용)
        if (TotalWeight > KINDA_SMALL_NUMBER)
        {
            if (!FMath::IsNearlyEqual(TotalWeight, 1.0f))
            {
                // 가중치 합이 1이 아니면 정규화
                float InvWeight = 1.0f / TotalWeight;
                SkinnedPosition *= InvWeight;
                SkinnedNormal *= InvWeight;
            }

            // 최종 결과 적용
            SkinnedVertex.Position = SkinnedPosition;
            SkinnedVertex.Normal = SkinnedNormal.GetSafeNormal();
        }
    }

    OutHitDistance = FLT_MAX;
    int HitCount = 0;

    for (int i = 0; i + 2 < Indices.Num(); i += 3)
    {
        const FVector v0(SkinnedVertices[Indices[i + 0]].Position.X, SkinnedVertices[Indices[i + 0]].Position.Y, SkinnedVertices[Indices[i + 0]].Position.Z);
        const FVector v1(SkinnedVertices[Indices[i + 1]].Position.X, SkinnedVertices[Indices[i + 1]].Position.Y, SkinnedVertices[Indices[i + 1]].Position.Z);
        const FVector v2(SkinnedVertices[Indices[i + 2]].Position.X, SkinnedVertices[Indices[i + 2]].Position.Y, SkinnedVertices[Indices[i + 2]].Position.Z);

        float HitDistance = 0.f;
        if (IntersectRayTriangle(InRayOrigin, InRayDirection, v0, v1, v2, HitDistance))
        {
            OutHitDistance = FMath::Min(OutHitDistance, HitDistance);
            ++HitCount;
        }
    }
    return HitCount;
}

void USkeletalMeshComponent::SetAnimationMode(EAnimationMode::Type InAnimationMode)
{
    if (!bEnableAnimation)
    {
        UE_LOG(ELogLevel::Warning, TEXT("SetAnimationMode: Animation is currently disabled"));
        return;
    }

    // Only from Animasset currently
    /*const bool bNeedChange = AnimationMode != InAnimationMode;
    if (bNeedChange)
    {
        AnimationMode = InAnimationMode;
        ClearAnimScriptInstance();
    }*/

    // when mode is swapped, make sure to reinitialize
    // even if it was same mode, this was due to users who wants to use BP construction script to do this
    // if you use it in the construction script, it gets serialized, but it never instantiate. 
    //if (GetSkeletalMeshAsset() != nullptr && (bNeedChange || (AnimationMode == EAnimationMode::AnimationBlueprint)))
    if (GetSkeletalMeshAsset() != nullptr)
    {
        //if (InitializeAnimScriptInstance(true))
        //{
            //OnAnimInitialized.Broadcast();
        //}
    }
}

USkeletalMesh* USkeletalMeshComponent::GetSkeletalMeshAsset() const
{
    return GetSkeletalMesh();
}

EAnimationMode::Type USkeletalMeshComponent::GetAnimationMode() const
{
    return AnimationMode;
}

void USkeletalMeshComponent::PlayAnimation(class UAnimationAsset* NewAnimToPlay, bool bLooping)
{
    if (!bEnableAnimation)
    {
        UE_LOG(ELogLevel::Warning, TEXT("PlayAnimation: Animation is currently disabled"));
        return;
    }

    SetAnimationMode(EAnimationMode::AnimationSingleNode);
    SetAnimation(NewAnimToPlay);
    Play(bLooping);
}

// [TEMP] Animation blend test
class UMyAnimInstance* USkeletalMeshComponent::GetMyAnimInstance() const
{
    return Cast<UMyAnimInstance>(AnimScriptInstance);
}

class UAnimSingleNodeInstance* USkeletalMeshComponent::GetSingleNodeInstance() const
{
    return Cast<UAnimSingleNodeInstance>(AnimScriptInstance);
}

void USkeletalMeshComponent::SetAnimation(UAnimationAsset* NewAnimToPlay)
{
    if (!bEnableAnimation)
    {
        UE_LOG(ELogLevel::Warning, TEXT("SetAnimation: Animation is currently disabled"));
        return;
    }

    UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
    if (SingleNodeInstance)
    {
        SingleNodeInstance->SetAnimationAsset(NewAnimToPlay, false);
        SingleNodeInstance->SetPlaying(false);
    }
    else if (AnimScriptInstance != nullptr)
    {
        UE_LOG(ELogLevel::Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
    }
}

void USkeletalMeshComponent::Play(bool bLooping)
{
    if (!bEnableAnimation)
    {
        UE_LOG(ELogLevel::Warning, TEXT("Play: Animation is currently disabled"));
        return;
    }

    // [TEST] Animation blending test
    UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
    UMyAnimInstance* MyAnimInstance = GetMyAnimInstance();

    if (SingleNodeInstance)
    {
        UAnimSequence* CurrentSequence = Cast<UAnimSequence>(SingleNodeInstance->CurrentAsset);

        if (!CurrentSequence)
        {
            UE_LOG(ELogLevel::Warning, TEXT("No animation asset set currently"));
            return;
        }

        const double PlayLength = CurrentSequence->GetDataModel()->GetPlayLength();

        // [TEMP] 확인 편의를 위해 Play시 CurrentTime 초기화
        if (SingleNodeInstance->PlayRate >= 0.0f)
        {
            SingleNodeInstance->CurrentTime = 0.0f;
        }
        else
        {
            SingleNodeInstance->CurrentTime = PlayLength;
        }
        SingleNodeInstance->SetPlaying(true);
        SingleNodeInstance->SetLooping(bLooping);
    }
    else if (MyAnimInstance)
    {
        const double PlayLength = MyAnimInstance->AnimA->GetDataModel()->GetPlayLength();

        if (MyAnimInstance->PlayRate >= 0.0f)
        {
            MyAnimInstance->CurrentTime = 0.0f;
        }
        else
        {
            MyAnimInstance->CurrentTime = PlayLength;
        }
        MyAnimInstance->SetPlaying(true);
        MyAnimInstance->SetLooping(bLooping);
    }
    else if (AnimScriptInstance != nullptr)
    {
        UE_LOG(ELogLevel::Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
    }
}

void USkeletalMeshComponent::Stop()
{
    if (!bEnableAnimation)
    {
        UE_LOG(ELogLevel::Warning, TEXT("Stop: Animation is currently disabled"));
        return;
    }

    // [TEST] Animation blending test
    UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
    UMyAnimInstance* MyAnimInstance = GetMyAnimInstance();

    if (SingleNodeInstance)
    {
        SingleNodeInstance->SetPlaying(false);
        SingleNodeInstance->ResetToReferencePose();
    }
    else if (MyAnimInstance)
    {
        MyAnimInstance->SetPlaying(false);
        //MyAnimInstance->ResetToReferencePose();
    }
    else if (AnimScriptInstance != nullptr)
    {
        UE_LOG(ELogLevel::Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
    }
}

bool USkeletalMeshComponent::IsPlaying() const
{
    UAnimSingleNodeInstance* SingleNodeInstance = GetSingleNodeInstance();
    if (SingleNodeInstance)
    {
        return SingleNodeInstance->IsPlaying();
    }
    else if (AnimScriptInstance != nullptr)
    {
        UE_LOG(ELogLevel::Warning, TEXT("Currently in Animation Blueprint mode. Please change AnimationMode to Use Animation Asset"));
    }

    return false;
}

void USkeletalMeshComponent::TickAnimation(float DeltaTime, bool bNeedsValidRootMotion)
{
    if (!bEnableAnimation)
    {
        return;
    }

    // Recalculate the RequiredBones array, if necessary
    /*if (!bRequiredBonesUpToDate)
    {
        RecalcRequiredBones(GetPredictedLODLevel());
    }*/
    // if curves have to be refreshed
   /* else if (!AreRequiredCurvesUpToDate())
    {
        RecalcRequiredCurves();
    }*/

    if (GetSkeletalMeshAsset() != nullptr)
    {
        // We're about to UpdateAnimation, this will potentially queue events that we'll need to dispatch.
        //bNeedsQueuedAnimEventsDispatched = true;

        // Tick all of our anim instances
        TickAnimInstances(DeltaTime, bNeedsValidRootMotion);
    }
}

void USkeletalMeshComponent::TickAnimInstances(float DeltaTime, bool bNeedsValidRootMotion)
{
    if (!bEnableAnimation)
    {
        return;
    }

    // Allow animation instance to do some processing before the linked instances update
    /*if (AnimScriptInstance != nullptr)
    {
        AnimScriptInstance->PreUpdateLinkedInstances(DeltaTime);
    }*/

    // We update linked instances first incase we're using either root motion or non-threaded update.
    // This ensures that we go through the pre update process and initialize the proxies correctly.
    //for (UAnimInstance* LinkedInstance : LinkedInstances)
    //{
    //    // Sub anim instances are always forced to do a parallel update 
    //    LinkedInstance->UpdateAnimation(DeltaTime * GlobalAnimRateScale, false, UAnimInstance::EUpdateAnimationFlag::ForceParallelUpdate);
    //}
    // [TEMP] FSM test - earky return if state machine exist
    if (AnimSMInstance != nullptr)
    {
        AnimSMInstance->UpdateAnimation(DeltaTime, bNeedsValidRootMotion);
        return;
    }

    if (AnimScriptInstance != nullptr)
    {
        // Tick the animation
        //AnimScriptInstance->UpdateAnimation(DeltaTime * GlobalAnimRateScale, bNeedsValidRootMotion);
        AnimScriptInstance->UpdateAnimation(DeltaTime, bNeedsValidRootMotion);
    }

    /*if (ShouldUpdatePostProcessInstance())
    {
        PostProcessAnimInstance->UpdateAnimation(DeltaTime * GlobalAnimRateScale, false);
    }*/
}
