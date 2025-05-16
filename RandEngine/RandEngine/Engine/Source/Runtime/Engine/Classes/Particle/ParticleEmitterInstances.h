#pragma once
#include "ParticleHelper.h"

class UParticleEmitter;
class UParticleSystemComponent;
class UParticleLODLevel;
struct FParticleEmitterInstance
{
    /** The template this instance is based on.							*/
    UParticleEmitter* SpriteTemplate;
    /** The component who owns it.										*/
    UParticleSystemComponent* Component;
    /** The currently set LOD level.									*/
    UParticleLODLevel* CurrentLODLevel;
    /** The index of the currently set LOD level.						*/
    int32 CurrentLODLevelIndex;
    /** The offset to the TypeData payload in the particle data.		*/
    int32 TypeDataOffset;
    /** The offset to the TypeData instance payload.					*/
    int32 TypeDataInstanceOffset;
    /** The offset to the SubUV payload in the particle data.			*/
    int32 SubUVDataOffset;
    /** The offset to the dynamic parameter payload in the particle data*/
    int32 DynamicParameterDataOffset;
    /** Offset to the light module data payload.						*/
    int32 LightDataOffset;
    float LightVolumetricScatteringIntensity;
    /** The offset to the Orbit module payload in the particle data.	*/
    int32 OrbitModuleOffset;
    /** The offset to the Camera payload in the particle data.			*/
    int32 CameraPayloadOffset;
    /** The offset to the particle data.								*/
    int32 PayloadOffset;
    /** The location of the emitter instance							*/
    FVector Location;
    /** Transform from emitter local space to simulation space.			*/
    FMatrix EmitterToSimulation;
    /** Transform from simulation space to world space.					*/
    FMatrix SimulationToWorld;
    /** Component can disable Tick and Rendering of this emitter. */
    uint8 bEnabled : 1;
    /** If true, kill this emitter instance when it is deactivated.		*/
    uint8 bKillOnDeactivate : 1;
    /** if true, kill this emitter instance when it has completed.		*/
    uint8 bKillOnCompleted : 1;
    /** Whether this emitter requires sorting as specified by artist.	*/
    uint8 bRequiresSorting : 1;
    /** If true, halt spawning for this instance.						*/
    uint8 bHaltSpawning : 1;
    /** If true, this emitter has been disabled by game code and some systems to re-enable are not allowed. */
    uint8 bHaltSpawningExternal : 1;
    /** If true, the emitter has modules that require loop notification.*/
    uint8 bRequiresLoopNotification : 1;
    /** If true, the emitter ignores the component's scale. (Mesh emitters only). */
    uint8 bIgnoreComponentScale : 1;
    /** Hack: Make sure this is a Beam type to avoid casting from/to wrong types. */
    uint8 bIsBeam : 1;
    /** Whether axis lock is enabled, cached here to avoid finding it from the module each frame */
    uint8 bAxisLockEnabled : 1;
    /** When true and spawning is supressed, the bursts will be faked so that when spawning is enabled again, the bursts don't fire late. */
    uint8 bFakeBurstsWhenSpawningSupressed : 1;
    /** true if the emitter has no active particles and will no longer spawn any in the future */
    uint8 bEmitterIsDone : 1;
    /** Axis lock flags, cached here to avoid finding it from the module each frame */
    /*TEnumAsByte<EParticleAxisLock> LockAxisFlags;*/
    /** The sort mode to use for this emitter as specified by artist.	*/
    int32 SortMode;
    /** Pointer to the particle data array.								*/
    uint8* ParticleData;
    /** Pointer to the particle index array.							*/
    uint16* ParticleIndices;
    /** Pointer to the instance data array.								*/
    uint8* InstanceData;
    /** The size of the Instance data array.							*/
    int32 InstancePayloadSize;
    /** The total size of a particle (in bytes).						*/
    int32 ParticleSize;
    /** The stride between particles in the ParticleData array.			*/
    int32 ParticleStride;
    /** The number of particles currently active in the emitter.		*/
    int32 ActiveParticles;
    /** Monotonically increasing counter. */
    uint32 ParticleCounter;
    /** The maximum number of active particles that can be held in
     *	the particle data array.
     */
    int32 MaxActiveParticles;
    /** The fraction of time left over from spawning.					*/
    float SpawnFraction;
    /** The number of seconds that have passed since the instance was
     *	created.
     */
    float SecondsSinceCreation;
    /** The amount of time simulated in the previous time step. */
    float EmitterTime;
    /** how long did the last tick take? */
    float LastDeltaTime;


    void SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity, struct FParticleEventInstancePayload* EventPayload);

    void KillParticle(int32 Index);
};
