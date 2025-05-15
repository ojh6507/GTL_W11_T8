#pragma once
#include "Actors/Player.h"
#include "Components/ProjectileMovementComponent.h"

class USphereComponent;

class ARandCharacter : public APlayer
{
    DECLARE_CLASS(ARandCharacter, APlayer)
public:
    ARandCharacter();
    virtual ~ARandCharacter() override = default;

    virtual void PostSpawnInitialize() override;

    UObject* Duplicate(UObject* InOuter) override;

    void BeginPlay() override;

    void Tick(float DeltaTime) override;

    UPROPERTY
    (USphereComponent*, SphereComponent, = nullptr)

    UPROPERTY
    (USkeletalMeshComponent*, SkeletalComponent, = nullptr)
private:
    FVector PreviousLocation;
};
