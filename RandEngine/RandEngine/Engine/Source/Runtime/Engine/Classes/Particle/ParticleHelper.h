#pragma once
#include "Define.h"
#include "Components/Material/Material.h"
#include "Renderer/GlobalRenderResource.h"

#include <cassert>
#include <cstddef>

#ifndef check
#define check(expr) assert(expr)
#endif
// UE5에서 일부 가져옴

class UStaticMesh;
struct FParticleMeshEmitterInstance;

#define DECLARE_PARTICLE(Name,Address)		\
	FBaseParticle& Name = *((FBaseParticle*) (Address));

#define DECLARE_PARTICLE_CONST(Name,Address)		\
	const FBaseParticle& Name = *((const FBaseParticle*) (Address));

#define DECLARE_PARTICLE_PTR(Name,Address)		\
	FBaseParticle* Name = (FBaseParticle*) (Address);

#define BEGIN_UPDATE_LOOP																								\
	{																													\
		check((Owner != NULL) && (Owner->Component != NULL));															\
		int32&			ActiveParticles = Owner->ActiveParticles;														\
		const uint8*		ParticleData	= Owner->ParticleData;															\
		const uint32		ParticleStride	= Owner->ParticleStride;														\
		uint16*			ParticleIndices	= Owner->ParticleIndices;														\
		for(int32 i=ActiveParticles-1; i>=0; i--)																			\
		{																												\
			const int32	CurrentIndex	= ParticleIndices[i];															\
			const uint8* ParticleBase	= ParticleData + CurrentIndex * ParticleStride;									\
			FBaseParticle& Particle		= *((FBaseParticle*) ParticleBase);												\
			if ((Particle.Flags & STATE_Particle_Freeze) == 0)															\
			{																											\

#define END_UPDATE_LOOP																									\
			}																											\
		}																												\
	}

#define CONTINUE_UPDATE_LOOP																							\
		continue;

#define SPAWN_INIT																										\
	check((Owner != NULL) && (Owner->Component != NULL));																\
	const int32		ActiveParticles	= Owner->ActiveParticles;															\
	const uint32		ParticleStride	= Owner->ParticleStride;															\
	uint32			CurrentOffset	= Offset;																			\
	FBaseParticle&	Particle		= *(ParticleBase);

#define PARTICLE_ELEMENT(Type,Name)																						\
	Type& Name = *((Type*)((uint8*)ParticleBase + CurrentOffset));																\
	CurrentOffset += sizeof(Type);

#define KILL_CURRENT_PARTICLE																							\
	{																													\
		ParticleIndices[i]					= ParticleIndices[ActiveParticles-1];										\
		ParticleIndices[ActiveParticles-1]	= CurrentIndex;																\
		ActiveParticles--;																								\
	}

enum class EParticleFlags : uint32 
{
    None = 0,
    Freeze = 1 << 0,
    Death = 1 << 1,
    Collision = 1 << 2,
    DisableGravity = 1 << 3,
    IsLit = 1 << 4,
};

/*-----------------------------------------------------------------------------
    FBaseParticle
-----------------------------------------------------------------------------*/
// Mappings for 'standard' particle data
// Only used when required.

struct FBaseParticle
{
    // 48 bytes
    FVector		OldLocation;			// Last frame's location, used for collision
    FVector		Location;				// Current location

    // 16 bytes
    FVector		    BaseVelocity;			// Velocity = BaseVelocity at the start of each frame.
    float			Rotation;				// Rotation of particle (in Radians)

    // 16 bytes
    FVector		    Velocity;				// Current velocity, gets reset to BaseVelocity each frame to allow 
    float			BaseRotationRate;		// Initial angular velocity of particle (in Radians per second)

    // 16 bytes
    FVector		    BaseSize;				// Size = BaseSize at the start of each frame
    float			RotationRate;			// Current rotation rate, gets reset to BaseRotationRate each frame

    // 16 bytes
    FVector		    Size;					// Current size, gets reset to BaseSize each frame
    int32			Flags;					// Flags indicating various particle states

    // 16 bytes
    FLinearColor	Color;					// Current color of particle.

    // 16 bytes
    FLinearColor	BaseColor;				// Base color of the particle

    // 16 bytes
    float			RelativeTime;			// Relative time, range is 0 (==spawn) to 1 (==death)
    float			OneOverMaxLifetime;		// Reciprocal of lifetime
    float			Placeholder0;
    float			Placeholder1;
};

/*-----------------------------------------------------------------------------
    Particle State Flags
-----------------------------------------------------------------------------*/
enum EParticleStates
{
    /** Ignore updates to the particle						*/
    STATE_Particle_JustSpawned = 0x02000000,
    /** Ignore updates to the particle						*/
    STATE_Particle_Freeze = 0x04000000,
    /** Ignore collision updates to the particle			*/
    STATE_Particle_IgnoreCollisions = 0x08000000,
    /**	Stop translations of the particle					*/
    STATE_Particle_FreezeTranslation = 0x10000000,
    /**	Stop rotations of the particle						*/
    STATE_Particle_FreezeRotation = 0x20000000,
    /** Combination for a single check of 'ignore' flags	*/
    STATE_Particle_CollisionIgnoreCheck = STATE_Particle_Freeze | STATE_Particle_IgnoreCollisions | STATE_Particle_FreezeTranslation | STATE_Particle_FreezeRotation,
    /** Delay collision updates to the particle				*/
    STATE_Particle_DelayCollisions = 0x40000000,
    /** Flag indicating the particle has had at least one collision	*/
    STATE_Particle_CollisionHasOccurred = 0x80000000,
    /** State mask. */
    STATE_Mask = 0xFE000000,
    /** Counter mask. */
    STATE_CounterMask = (~STATE_Mask)
};


/**
 * Per-particle data sent to the GPU.
 */
struct FParticleSpriteVertex
{
    /** The position of the particle. */
    FVector Position;
    /** The relative time of the particle. */
    float RelativeTime;
    /** The previous position of the particle. */
    FVector	OldPosition;
    /** Value that remains constant over the lifetime of a particle. */
    float ParticleId;
    /** The size of the particle. */
    FVector2D Size;
    /** The rotation of the particle. */
    float Rotation;
    /** The sub-image index for the particle. */
    float SubImageIndex;
    /** The color of the particle. */
    FLinearColor Color;
};

/**
 * Per-particle data sent to the GPU.
 */
struct FParticleSpriteVertexNonInstanced
{
    /** The texture UVs. */
    FVector2D UV;
    /** The position of the particle. */
    FVector Position;
    /** The relative time of the particle. */
    float RelativeTime;
    /** The previous position of the particle. */
    FVector	OldPosition;
    /** Value that remains constant over the lifetime of a particle. */
    float ParticleId;
    /** The size of the particle. */
    FVector2D Size;
    /** The rotation of the particle. */
    float Rotation;
    /** The sub-image index for the particle. */
    float SubImageIndex;
    /** The color of the particle. */
    FLinearColor Color;
};


//	FParticleSpriteVertexDynamicParameter
struct FParticleVertexDynamicParameter
{
    /** The dynamic parameter of the particle			*/
    float			DynamicValue[4];
};


// Per-particle data sent to the GPU.
struct FMeshParticleInstanceVertex
{
    /** The color of the particle. */
    FLinearColor Color;

    /** The instance to world transform of the particle. Translation vector is packed into W components. */
    FVector4 Transform[3];

    /** The velocity of the particle, XYZ: direction, W: speed. */
    FVector4 Velocity;

    /** The sub-image texture offsets for the particle. */
    int16 SubUVParams[4];

    /** The sub-image lerp value for the particle. */
    float SubUVLerp;

    /** The relative time of the particle. */
    float RelativeTime;
};

struct FMeshParticleInstanceVertexDynamicParameter
{
    /** The dynamic parameter of the particle. */
    float DynamicValue[4];
};

/*-----------------------------------------------------------------------------
    Particle Sorting Helper
-----------------------------------------------------------------------------*/
struct FParticleOrder
{
    int32 ParticleIndex;

    union
    {
        float Z;
        uint32 C;
    };

    FParticleOrder(int32 InParticleIndex, float InZ) :
        ParticleIndex(InParticleIndex),
        Z(InZ)
    {
    }

    FParticleOrder(int32 InParticleIndex, uint32 InC) :
        ParticleIndex(InParticleIndex),
        C(InC)
    {
    }
};

/*-----------------------------------------------------------------------------
    Async Fill Organizational Structure
-----------------------------------------------------------------------------*/

struct FAsyncBufferFillData
{
    /** Local to world transform. */
    FMatrix LocalToWorld;
    /** World to local transform. */
    FMatrix WorldToLocal;
    /** View for this buffer fill task   */
    const FEditorViewportClient* View;
    /** Number of verts in VertexData   */
    int32 VertexCount;
    /** Stride of verts, used only for error checking   */
    int32 VertexSize;
    /** Pointer to vertex data   */
    void* VertexData;
    /** Number of indices in IndexData   */
    int32 IndexCount;
    /** Pointer to index data   */
    void* IndexData;
    /** Number of triangles filled in   */
    int32 OutTriangleCount;
    /** Pointer to dynamic parameter data */
    void* DynamicParameterData;

    /** Constructor, just zeros everything   */
    FAsyncBufferFillData()
    {
        // this is all POD
        FPlatformMemory::MemZero(this, sizeof(FAsyncBufferFillData));
    }
    /** Destructor, frees memory and zeros everything   */
    ~FAsyncBufferFillData()
    {
        FPlatformMemory::MemZero(this, sizeof(FAsyncBufferFillData));
    }
};

/**
 * Dynamic particle emitter types
 *
 * NOTE: These are serialized out for particle replay data, so be sure to update all appropriate
 *    when changing anything here.
 */
enum EDynamicEmitterType
{
    DET_Unknown = 0,
    DET_Sprite,
    DET_Mesh,
    DET_Beam2,
    DET_Ribbon,
    DET_AnimTrail,
    DET_Custom
};

struct FParticleDataContainer
{
    int32 MemBlockSize;
    int32 ParticleDataNumBytes;
    int32 ParticleIndicesNumShorts;
    uint8* ParticleData; // this is also the memory block we allocated
    uint16* ParticleIndices; // not allocated, this is at the end of the memory block

    FParticleDataContainer()
        : MemBlockSize(0)
        , ParticleDataNumBytes(0)
        , ParticleIndicesNumShorts(0)
        , ParticleData(nullptr)
        , ParticleIndices(nullptr)
    {
    }
    ~FParticleDataContainer()
    {
        Free();
    }
    void Alloc(int32 InParticleDataNumBytes, int32 InParticleIndicesNumShorts);
    void Free();
};


struct FMacroUVOverride
{
    FMacroUVOverride() : bOverride(false), Radius(0.f), Position(0.f, 0.f, 0.f) {}

    bool	bOverride;
    float   Radius;
    FVector Position;

    friend FORCEINLINE FArchive& operator<<(FArchive& Ar, FMacroUVOverride& O)
    {
        Ar << O.bOverride;
        Ar << O.Radius;
        Ar << O.Position;
        return Ar;
    }
};

//
//	SubUV-related payloads
//
struct FFullSubUVPayload
{
    // The integer portion indicates the sub-image index.
    // The fractional portion indicates the lerp factor.
    float ImageIndex;
    float RandomImageTime;
};

/**
 *	Chain-able Orbit module instance payload
 */
struct FOrbitChainModuleInstancePayload
{
    /** The base offset of the particle from it's tracked location	*/
    FVector	BaseOffset;
    /** The offset of the particle from it's tracked location		*/
    FVector	Offset;
    /** The rotation of the particle at it's offset location		*/
    FVector	Rotation;
    /** The base rotation rate of the particle offset				*/
    FVector	BaseRotationRate;
    /** The rotation rate of the particle offset					*/
    FVector	RotationRate;
    /** The offset of the particle from the last frame				*/
    FVector	PreviousOffset;
};

struct FParticleEventInstancePayload
{
    bool bSpawnEventsPresent = false;
    bool bDeathEventsPresent = false;
    bool bCollisionEventsPresent = false;
    bool bBurstEventsPresent = false;

    int32 SpawnTrackingCount;
    int32 DeathTrackingCount;
    int32 CollisionTrackingCount;
    int32 BurstTrackingCount;
};
/**
 *	DynamicParameter particle payload.
 */
struct FEmitterDynamicParameterPayload
{
    /** The float4 value to assign to the dynamic parameter. */
    float DynamicParameterValue[4];
};

/**
 *	Helper function for retrieving the dynamic payload of a particle.
 *
 *	@param	InDynamicPayloadOffset		The offset to the payload
 *	@param	InParticle					The particle being processed
 *	@param	OutDynamicData				The dynamic data from the particle
 */
FORCEINLINE void GetDynamicValueFromPayload(int32 InDynamicPayloadOffset, const FBaseParticle& InParticle, FVector4& OutDynamicData)
{
    const FEmitterDynamicParameterPayload* DynPayload = ((const FEmitterDynamicParameterPayload*)((uint8*)(&InParticle) + InDynamicPayloadOffset));
    OutDynamicData.X = DynPayload->DynamicParameterValue[0];
    OutDynamicData.Y = DynPayload->DynamicParameterValue[1];
    OutDynamicData.Z = DynPayload->DynamicParameterValue[2];
    OutDynamicData.W = DynPayload->DynamicParameterValue[3];
}

/** Camera offset particle payload */
struct FCameraOffsetParticlePayload
{
    /** The base amount to offset the particle towards the camera */
    float	BaseOffset;
    /** The amount to offset the particle towards the camera */
    float	Offset;
};

/** Source data base class for all emitter types */
struct FDynamicEmitterReplayDataBase
{
    /**	The type of emitter. */
    EDynamicEmitterType	eEmitterType;

    /**	The number of particles currently active in this emitter. */
    int32 ActiveParticleCount;

    int32 ParticleStride;
    FParticleDataContainer DataContainer;

    FVector Scale;

    /** Whether this emitter requires sorting as specified by artist.	*/
    int32 SortMode;

    /** MacroUV (override) data **/
    FMacroUVOverride MacroUVOverride;

    /** Constructor */
    FDynamicEmitterReplayDataBase()
        : eEmitterType(DET_Unknown),
        ActiveParticleCount(0),
        ParticleStride(0),
        Scale(FVector(1.0f)),
        SortMode(0)	// Default to PSORTMODE_None		  
    {
    }

    virtual ~FDynamicEmitterReplayDataBase()
    {
    }
};

/** Source data base class for Sprite emitters */
struct FDynamicSpriteEmitterReplayDataBase
    : public FDynamicEmitterReplayDataBase
{
    UMaterial* MaterialInterface;
    struct FParticleRequiredModule* RequiredModule;
    FVector							NormalsSphereCenter;
    FVector							NormalsCylinderDirection;
    float							InvDeltaSeconds;
    FVector						    LWCTile;
    int32							MaxDrawCount;
    int32							OrbitModuleOffset;
    int32							DynamicParameterDataOffset;
    int32							LightDataOffset;
    float							LightVolumetricScatteringIntensity;
    int32							CameraPayloadOffset;
    int32							SubUVDataOffset;
    int32							SubImages_Horizontal;
    int32							SubImages_Vertical;
    bool						bUseLocalSpace;
    bool						bLockAxis;
    uint8						ScreenAlignment;
    uint8						LockAxisFlag;
    uint8						EmitterRenderMode;
    uint8						EmitterNormalsMode;
    FVector2D     				PivotOffset;
    bool						bUseVelocityForMotionBlur;
    bool						bRemoveHMDRoll;
    float						MinFacingCameraBlendDistance;
    float						MaxFacingCameraBlendDistance;

    /** Constructor */
    FDynamicSpriteEmitterReplayDataBase();
    ~FDynamicSpriteEmitterReplayDataBase();

};

/** Base class for all emitter types */
struct FDynamicEmitterDataBase
{
    FDynamicEmitterDataBase(const class UParticleModuleRequired* RequiredModule);

    virtual ~FDynamicEmitterDataBase()
    {
    }

    /** Custom new/delete with recycling */
    //void* operator new(size_t Size);
    //void operator delete(void* RawMemory, size_t Size);

    /** Returns the source data for this particle system */
    virtual const FDynamicEmitterReplayDataBase& GetSource() const = 0;

    /** Returns the current macro uv override. Specialized by FGPUSpriteDynamicEmitterData  */
    virtual const FMacroUVOverride& GetMacroUVOverride() const { return GetSource().MacroUVOverride; }

    /** Stat id of this object, 0 if nobody asked for it yet */
    mutable TStatId StatID;
    /** true if this emitter is currently selected */
    bool bSelected = false;
    /** true if this emitter has valid rendering data */
    bool bValid = false;

    int32  EmitterIndex;

    FGlobalDynamicVertexBufferAllocation VertexAllocation;
    FGlobalDynamicIndexBufferAllocation  IndexAllocation;
    FGlobalDynamicVertexBufferAllocation ParamAllocation;
};


/** Base class for Sprite emitters and other emitter types that share similar features. */
struct FDynamicSpriteEmitterDataBase : public FDynamicEmitterDataBase
{
    FDynamicSpriteEmitterDataBase(const UParticleModuleRequired* RequiredModule) :
        FDynamicEmitterDataBase(RequiredModule),
        bUsesDynamicParameter(false)
    {
        MaterialResource = nullptr;
    }

    virtual ~FDynamicSpriteEmitterDataBase()
    {
    }

    const UMaterial* GetMaterial()
    {
        return MaterialResource;
    }
    /**
     *	Sort the given sprite particles
     *
     *	@param	SorceMode			The sort mode to utilize (EParticleSortMode)
     *	@param	bLocalSpace			true if the emitter is using local space
     *	@param	ParticleCount		The number of particles
     *	@param	ParticleData		The actual particle data
     *	@param	ParticleStride		The stride between entries in the ParticleData array
     *	@param	ParticleIndices		Indirect index list into ParticleData
     *	@param	View				The scene view being rendered
     *	@param	LocalToWorld		The local to world transform of the component rendering the emitter
     *	@param	ParticleOrder		The array to fill in with ordered indices
     */
    void SortSpriteParticles(int32 SortMode, bool bLocalSpace,
        int32 ParticleCount, const uint8* ParticleData, int32 ParticleStride, const uint16* ParticleIndices,
        const FEditorViewportClient* View, const FMatrix& LocalToWorld, FParticleOrder* ParticleOrder) const;

    /**
     *	Get the vertex stride for the dynamic rendering data
     */
    virtual int32 GetDynamicVertexStride() const
    {
        return 0;
    }

    /**
     *	Get the vertex stride for the dynamic parameter rendering data
     */
    virtual int32 GetDynamicParameterVertexStride() const
    {
        return 0;
    }

    /**
     *	Get the source replay data for this emitter
     */
    virtual const FDynamicSpriteEmitterReplayDataBase* GetSourceData() const
    {
        return NULL;
    }

    /**
     *	Gets the information required for allocating this emitters indices from the global index array.
     */
    virtual void GetIndexAllocInfo(int32& OutNumIndices, int32& OutStride) const
    {
        // 활성 파티클 개수
        int32 NumParticles = GetSourceData()->ActiveParticleCount;

        // 스프라이트 한 개당 6 인덱스 (2개의 삼각형)
        const int32 IndicesPerParticle = 6;
        OutNumIndices = NumParticles * IndicesPerParticle;

        // 2바이트 인덱스 (uint16)
        OutStride = sizeof(uint16);
    }

    /**
     *	Set up an buffer for async filling
     *
     *	@param	Proxy					The primitive scene proxy for the emitter.
     *	@param	InView					View for this buffer
     *	@param	InVertexCount			Count of verts for this buffer
     *	@param	InVertexSize			Stride of these verts, only used for verification
     *	@param	InDynamicParameterVertexStride	Stride of the dynamic parameter
     */
    void BuildViewFillData(
        int32 InVertexCount,
        int32 InVertexSize,
        int32 InDynamicParameterVertexSize,
        FGlobalDynamicIndexBuffer& DynamicIndexBuffer,
        FGlobalDynamicVertexBuffer& DynamicVertexBuffer,
        FGlobalDynamicVertexBufferAllocation& DynamicVertexAllocation,
        FGlobalDynamicIndexBufferAllocation& DynamicIndexAllocation,
        FGlobalDynamicVertexBufferAllocation* DynamicParameterAllocation,
        FAsyncBufferFillData& Data) const;

    const UMaterial* MaterialResource;

    /** true if the particle emitter utilizes the DynamicParameter module */
    bool bUsesDynamicParameter = false;

    FAsyncBufferFillData AsyncFillData;
};

/** Source data for Sprite emitters */
struct FDynamicSpriteEmitterReplayData
    : public FDynamicSpriteEmitterReplayDataBase
{
    /** Constructor */
    FDynamicSpriteEmitterReplayData()
    {
    }

};


/** Dynamic emitter data for sprite emitters */
struct FDynamicSpriteEmitterData : public FDynamicSpriteEmitterDataBase
{
    FDynamicSpriteEmitterData(const UParticleModuleRequired* RequiredModule) :
        FDynamicSpriteEmitterDataBase(RequiredModule)
    {
    }

    ~FDynamicSpriteEmitterData()
    {
    }

    /** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
    void Init(bool bInSelected);

    /**
     *	Get the vertex stride for the dynamic rendering data
     */
    virtual int32 GetDynamicVertexStride() const override
    {
        return sizeof(FParticleSpriteVertexNonInstanced);
    }

    /**
     *	Get the vertex stride for the dynamic parameter rendering data
     */
    virtual int32 GetDynamicParameterVertexStride() const override
    {
        return sizeof(FParticleVertexDynamicParameter);
    }

    /**
     *	Get the source replay data for this emitter
     */
    virtual const FDynamicSpriteEmitterReplayDataBase* GetSourceData() const override
    {
        return &Source;
    }

    /**
     *	Retrieve the vertex and (optional) index required to render this emitter.
     *	Render-thread only
     *
     *	@param	VertexData			The memory to fill the vertex data into
     *	@param	FillIndexData		The index data to fill in
     *	@param	ParticleOrder		The (optional) particle ordering to use
     *	@param	InCameraPosition	The position of the camera in world space.
     *	@param	InLocalToWorld		Transform from local to world space.
     *	@param	InstanceFactor		The factor to duplicate instances by.
     *
     *	@return	bool			true if successful, false if failed
     */
    bool GetVertexAndIndexData(void* VertexData, void* DynamicParameterVertexData, void* FillIndexData, FParticleOrder* ParticleOrder, const FVector& InCameraPosition, const FMatrix& InLocalToWorld, uint32 InstanceFactor) const;

    /**
     *	Retrieve the vertex and (optional) index required to render this emitter.
     *  This version for non-instanced platforms.
     *	Render-thread only
     *
     *	@param	VertexData			The memory to fill the vertex data into
     *	@param	FillIndexData		The index data to fill in
     *	@param	ParticleOrder		The (optional) particle ordering to use
     *	@param	InCameraPosition	The position of the camera in world space.
     *	@param	InLocalToWorld		Transform from local to world space.
     *
     *	@return	bool			true if successful, false if failed
     */

    bool GetVertexAndIndexDataNonInstanced(void* VertexData, void* DynamicParameterVertexData, void* FillIndexData, FParticleOrder* ParticleOrder, const FVector& InCameraPosition, const FMatrix& InLocalToWorld, int32 NumVerticesPerParticle) const;

    ///** Gathers simple lights for this emitter. */
    //virtual void GatherSimpleLights(const FParticleSystemSceneProxy* Proxy, const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const override;

    //virtual void GetDynamicMeshElementsEmitter(const FParticleSystemSceneProxy* Proxy, const FSceneView* View, const FSceneViewFamily& ViewFamily, int32 ViewIndex, FMeshElementCollector& Collector) const override;

    ///**
    // *	Create the render thread resources for this emitter data
    // *
    // *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
    // *
    // *	@return	bool			true if successful, false if failed
    // */
    //virtual void UpdateRenderThreadResourcesEmitter(const FParticleSystemSceneProxy* InOwnerProxy) override;

    /** Returns the source data for this particle system */
    virtual const FDynamicEmitterReplayDataBase& GetSource() const override
    {
        return Source;
    }

    /** The frame source data for this particle system.  This is everything needed to represent this
        this particle system frame.  It does not include any transient rendering thread data.  Also, for
        non-simulating 'replay' particle systems, this data may have come straight from disk! */
    FDynamicSpriteEmitterReplayData Source;

    /** Uniform parameters. Most fields are filled in when updates are sent to the rendering thread, some are per-view! */
    //FParticleSpriteUniformParameters UniformParameters;
};


/** Source data for Mesh emitters */
struct FDynamicMeshEmitterReplayData
    : public FDynamicSpriteEmitterReplayDataBase
{
    int32	SubUVInterpMethod;
    int32	SubUVDataOffset;
    int32	SubImages_Horizontal;
    int32	SubImages_Vertical;
    bool	bScaleUV;
    int32	MeshRotationOffset;
    int32	MeshMotionBlurOffset;
    uint8	MeshAlignment;
    bool	bMeshRotationActive;
    FVector	LockedAxis;

    /** Constructor */
    FDynamicMeshEmitterReplayData() :
        SubUVInterpMethod(0),
        SubUVDataOffset(0),
        SubImages_Horizontal(0),
        SubImages_Vertical(0),
        bScaleUV(false),
        MeshRotationOffset(0),
        MeshMotionBlurOffset(0),
        MeshAlignment(0),
        bMeshRotationActive(false),
        LockedAxis(1.0f, 0.0f, 0.0f)
    {
    }
};


/** Dynamic emitter data for Mesh emitters */
struct FDynamicMeshEmitterData : public FDynamicSpriteEmitterDataBase
{
    FDynamicMeshEmitterData(const UParticleModuleRequired* RequiredModule);

    virtual ~FDynamicMeshEmitterData();

    //uint32 GetMeshLODIndexFromProxy(const FParticleSystemSceneProxy* InOwnerProxy) const;
    ///** Initialize this emitter's dynamic rendering data, called after source data has been filled in */
    //void Init(bool bInSelected,
    //    const FParticleMeshEmitterInstance* InEmitterInstance,
    //    UStaticMesh* InStaticMesh,
    //    bool InUseStaticMeshLODs,
    //    float InLODSizeScale,
    //    ERHIFeatureLevel::Type InFeatureLevel);

    ///**
    // *	Create the render thread resources for this emitter data
    // *
    // *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
    // *
    // *	@return	bool			true if successful, false if failed
    // */
    //virtual void UpdateRenderThreadResourcesEmitter(const FParticleSystemSceneProxy* InOwnerProxy) override;

    ///**
    // *	Release the render thread resources for this emitter data
    // *
    // *	@param	InOwnerProxy	The proxy that owns this dynamic emitter data
    // *
    // *	@return	bool			true if successful, false if failed
    // */
    //virtual void ReleaseRenderThreadResources(const FParticleSystemSceneProxy* InOwnerProxy) override;

    //virtual void GetDynamicMeshElementsEmitter(const FParticleSystemSceneProxy* Proxy, const FSceneView* View, const FSceneViewFamily& ViewFamily, int32 ViewIndex, FMeshElementCollector& Collector) const override;

    /**
     *	Retrieve the instance data required to render this emitter.
     *	Render-thread only
     *
     *	@param	InstanceData            The memory to fill the vertex data into
     *	@param	DynamicParameterData    The memory to fill the vertex dynamic parameter data into
     *	@param	PrevTransformBuffer     The memory to fill the vertex prev transform data into. May be null
     *	@param	Proxy                   The scene proxy for the particle system that owns this emitter
     *	@param	View                    The scene view being rendered
     *	@param	InstanceFactor			The factor to duplicate instances by
     */
   /* void GetInstanceData(void* InstanceData, void* DynamicParameterData, void* PrevTransformBuffer, const FParticleSystemSceneProxy* Proxy, const FSceneView* View, uint32 InstanceFactor) const;*/

    /**
     *	Helper function for retrieving the particle transform.
     *
     *	@param	InParticle					The particle being processed
     *  @param	Proxy					    The scene proxy for the particle system that owns this emitter
     *	@param	View						The scene view being rendered
     *	@param	OutTransform				The InstanceToWorld transform matrix for the particle
     */
    //void GetParticleTransform(const FBaseParticle& InParticle, const FParticleSystemSceneProxy* Proxy, const FSceneView* View, FMatrix& OutTransformMat) const;

    //void GetParticlePrevTransform(const FBaseParticle& InParticle, const FParticleSystemSceneProxy* Proxy, const FSceneView* View, FMatrix& OutTransformMat) const;

    void CalculateParticleTransform(
        const FMatrix& ProxyLocalToWorld,
        const FVector& ParticleLocation,
        float    ParticleRotation,
        const FVector& ParticleVelocity,
        const FVector& ParticleSize,
        const FVector& ParticlePayloadInitialOrientation,
        const FVector& ParticlePayloadRotation,
        const FVector& ParticlePayloadCameraOffset,
        const FVector& ParticlePayloadOrbitOffset,
        const FVector& ViewOrigin,
        const FVector& ViewDirection,
        FMatrix& OutTransformMat
    ) const;

    ///** Gathers simple lights for this emitter. */
    //virtual void GatherSimpleLights(const FParticleSystemSceneProxy* Proxy, const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const override;

    /**
     *	Get the vertex stride for the dynamic rendering data
     */
    virtual int32 GetDynamicVertexStride() const override
    {
        return sizeof(FMeshParticleInstanceVertex);
    }

    virtual int32 GetDynamicParameterVertexStride() const override
    {
        return sizeof(FMeshParticleInstanceVertexDynamicParameter);
    }

    /**
     *	Get the source replay data for this emitter
     */
    virtual const FDynamicSpriteEmitterReplayDataBase* GetSourceData() const override
    {
        return &Source;
    }

    /**
     *	 Initialize this emitter's vertex factory with the vertex buffers from the mesh's rendering data.
     */
    //void SetupVertexFactory(FRHICommandListBase& RHICmdList, FMeshParticleVertexFactory* InVertexFactory, const FStaticMeshLODResources& LODResources, uint32 LODIdx) const;

    /** Returns the source data for this particle system */
    virtual const FDynamicEmitterReplayDataBase& GetSource() const override
    {
        return Source;
    }

    /** The frame source data for this particle system.  This is everything needed to represent this
        this particle system frame.  It does not include any transient rendering thread data.  Also, for
        non-simulating 'replay' particle systems, this data may have come straight from disk! */
    FDynamicMeshEmitterReplayData Source;

    int32					LastFramePreRendered;

    UStaticMesh* StaticMesh;
    TArray<UMaterial*> MeshMaterials;

    /** offset to FMeshTypeDataPayload */
    uint32 MeshTypeDataOffset;

    // 'orientation' items...
    // These don't need to go into the replay data, as they are constant over the life of the emitter
    /** If true, apply the 'pre-rotation' values to the mesh. */
    bool bApplyPreRotation = false;
    /** If true, then use the locked axis setting supplied. Trumps locked axis module and/or TypeSpecific mesh settings. */
    bool bUseMeshLockedAxis = false;
    /** If true, then use the camera facing options supplied. Trumps all other settings. */
    bool bUseCameraFacing = false;
    /**
     *	If true, apply 'sprite' particle rotation about the orientation axis (direction mesh is pointing).
     *	If false, apply 'sprite' particle rotation about the camera facing axis.
     */
    bool bApplyParticleRotationAsSpin = false;
    /**
    *	If true, all camera facing options will point the mesh against the camera's view direction rather than pointing at the cameras location.
    *	If false, the camera facing will point to the cameras position as normal.
    */
    bool bFaceCameraDirectionRatherThanPosition = false;
    /** The EMeshCameraFacingOption setting to use if bUseCameraFacing is true. */
    uint8 CameraFacingOption;

    bool bUseStaticMeshLODs;
    float LODSizeScale;
    mutable int32 LastCalculatedMeshLOD;
    const FParticleMeshEmitterInstance* EmitterInstance;
};

FORCEINLINE FVector2D GetParticleSizeWithUVFlipInSign(const FBaseParticle& Particle, const FVector2D& ScaledSize)
{
    return FVector2D(
        Particle.BaseSize.X >= 0.0f ? ScaledSize.X : -ScaledSize.X,
        Particle.BaseSize.Y >= 0.0f ? ScaledSize.Y : -ScaledSize.Y);
}
