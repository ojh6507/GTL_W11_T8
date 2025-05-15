// CustomAnimInstance.h
#pragma once

#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnNotifyByNameSignature, FName /*NotifyName*/);

class USkeleton;
class USkeletalMeshComponent;
struct FAnimNotifyEvent;
struct FAnimNotifyQueue;

class UAnimInstance : public UObject
{
    DECLARE_CLASS(UAnimInstance, UObject)
public:
    // 기본 생성자/소멸자
    UAnimInstance();
    virtual ~UAnimInstance();

    FOnNotifyByNameSignature OnNotifyTriggered;

    TMap<FName, FOnNotifyByNameSignature> NotifyActionMap;
    void BindNotifyActionLambda(FName NotifyName, std::function<void(FName)> Lambda)
    {
        NotifyActionMap.FindOrAdd(NotifyName).AddLambda(std::move(Lambda));
    }

    // 초기화/업데이트 인터페이스
    void Initialize();
    void Update(float DeltaTime);
    void PostEvaluate();

    void TriggerAnimNotifies(float DeltaTime);
    void TriggerSoundNotifies(const FAnimNotifyEvent& NotifyToTrigger);
    //void HandleNotify(const FAnimNotifyEvent& NotifyEvent);

    
    //const FBoneContainer& GetRequiredBones() const { return RequiredBones; }

    USkeletalMeshComponent* GetSkelMeshComponent() const;
    
    virtual void UpdateAnimation(float DeltaSeconds, bool bNeedsValidRootMotion);
    virtual void NativeUpdateAnimation(float DeltaSeconds) {}

    void SetCurrentSkeleton(USkeleton* InSkeleton) { CurrentSkeleton = InSkeleton; }

    void SetOwningComponent(USkeletalMeshComponent* InComponent) { OwningComponent = InComponent; }

public:
    USkeleton* CurrentSkeleton = nullptr;

protected:
    // 필수 데이터
    //FBoneContainer RequiredBones;
    TMap<FName, float> AnimationCurves;
    FAnimNotifyQueue* NotifyQueue;

    // 컴포넌트 참조
    USkeletalMeshComponent* OwningComponent = nullptr;
};
