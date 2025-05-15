#include "ARandCharacter.h"

#include "Components/SphereComponent.h"
#include "Components/Mesh/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "StateMachineAnimInstance.h"
#include "UObject/ObjectFactory.h"

ARandCharacter::ARandCharacter()
{
}

void ARandCharacter::PostSpawnInitialize()
{
    Super::PostSpawnInitialize();
    SphereComponent = AddComponent<USphereComponent>(FName("SphereComponent_0"));
    SetRootComponent(SphereComponent);
    
    SkeletalComponent = AddComponent<USkeletalMeshComponent>(FName("SkeletalComponent_0"));
    SkeletalComponent->SetSkeletalMesh(UAssetManager::Get().GetSkeletalMesh(L"Contents/RealSharkry.fbx"));
    SkeletalComponent->SetupAttachment(SphereComponent);
    SkeletalComponent->SetRelativeRotation(FRotator(0,-90,0));

    USpringArmComponent* SpringArmComp = AddComponent<USpringArmComponent>(FName("SpringArmComponent_0"));
    SpringArmComp->SetupAttachment(SphereComponent);

    UCameraComponent* CameraComp = AddComponent<UCameraComponent>(FName("CameraComponent_0"));
    CameraComp->SetupAttachment(SpringArmComp);
}

UObject* ARandCharacter::Duplicate(UObject* InOuter)
{
    ThisClass* NewActor = Cast<ThisClass>(Super::Duplicate(InOuter));

    return NewActor;
}

void ARandCharacter::BeginPlay()
{
    APlayer::BeginPlay();

	PreviousLocation = GetActorLocation();

	UStateMachineAnimInstance* AnimSMInstance = FObjectFactory::ConstructObject<UStateMachineAnimInstance>(nullptr);

	USkeletalMeshComponent* SkeletalComponent = GetComponentByClass<USkeletalMeshComponent>();
	SkeletalComponent->AnimSMInstance = AnimSMInstance;
	AnimSMInstance->SetOwningComponent(SkeletalComponent);
}

void ARandCharacter::Tick(float DeltaTime)
{
    APlayer::Tick(DeltaTime);

	// [TEST] Update isMove
	FVector CurrentLocation = GetActorLocation();
	float DistanceMoved = FMath::Sqrt(
		FMath::Square(PreviousLocation.X - CurrentLocation.X)
		+ FMath::Square(PreviousLocation.Y - CurrentLocation.Y)
	);

	const float MovementThreshold = 0.0f; // 단위는 cm 또는 프로젝트 단위에 맞게 설정

	bool bIsMoving = DistanceMoved > MovementThreshold;
	bool bIsJumping = CurrentLocation.Z > 0.1f;

	USkeletalMeshComponent* SkeletalComponent = GetComponentByClass<USkeletalMeshComponent>();

	if (SkeletalComponent && SkeletalComponent->AnimSMInstance)
	{
		SkeletalComponent->AnimSMInstance->bIsJumping = bIsJumping;
	}

	if (SkeletalComponent && SkeletalComponent->AnimSMInstance)
	{
		SkeletalComponent->AnimSMInstance->bIsMoving = bIsMoving;
	}

	PreviousLocation = CurrentLocation;
}
