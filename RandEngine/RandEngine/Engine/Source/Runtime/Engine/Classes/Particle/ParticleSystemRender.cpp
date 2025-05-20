#include "ParticleHelper.h"
#include "ParticleModuleRequired.h"
#include "Editor/UnrealEd/EditorViewportClient.h"

FVector2D GetParticleSize(const FBaseParticle& Particle, const FDynamicSpriteEmitterReplayDataBase& Source)
{
    FVector2D Size;
    Size.X = FMath::Abs(Particle.Size.X * Source.Scale.X);
    Size.Y = FMath::Abs(Particle.Size.Y * Source.Scale.Y);

    return Size;
}


void ApplyOrbitToPosition(
    const FBaseParticle& Particle,
    const FDynamicSpriteEmitterReplayDataBase& Source,
    const FMatrix& InLocalToWorld,
    FVector& ParticlePosition,
    FVector& ParticleOldPosition
)
{
    if (Source.OrbitModuleOffset != 0)
    {
        int32 CurrentOffset = Source.OrbitModuleOffset;
        const uint8* ParticleBase = (const uint8*)&Particle;
        PARTICLE_ELEMENT(FOrbitChainModuleInstancePayload, OrbitPayload);

        if (Source.bUseLocalSpace)
        {
            ParticlePosition += (FVector)OrbitPayload.Offset;
            ParticleOldPosition += (FVector)OrbitPayload.PreviousOffset;
        }
        else
        {
            ParticlePosition += FMatrix::TransformVector((FVector)OrbitPayload.Offset, InLocalToWorld);
            ParticleOldPosition += FMatrix::TransformVector((FVector)OrbitPayload.PreviousOffset, InLocalToWorld);
        }
    }
}


FORCEINLINE FVector GetCameraOffset(
    float CameraPayloadOffset,
    FVector DirToCamera
)
{
    float CheckSize = DirToCamera.SizeSquared();
    DirToCamera.Normalize();

    if (CheckSize > (CameraPayloadOffset * CameraPayloadOffset))
    {
        return DirToCamera * CameraPayloadOffset;
    }
    else
    {
        // If the offset will push the particle behind the camera, then push it 
        // WAY behind the camera. This is a hack... but in the case of 
        // PSA_Velocity, it is required to ensure that the particle doesn't 
        // 'spin' flat and come into view.
        return DirToCamera * CameraPayloadOffset * 10000.0f;
    }
}

/**
 *	Helper function for retrieving the camera offset payload of a particle.
 *
 *	@param	InCameraPayloadOffset	The offset to the camera offset payload data.
 *	@param	InParticle				The particle being processed.
 *	@param	InPosition				The position of the particle being processed.
 *	@param	InCameraPosition		The position of the camera in local space.
 *
 *	@returns the offset to apply to the particle's position.
 */
FORCEINLINE FVector GetCameraOffsetFromPayload(
    int32 InCameraPayloadOffset,
    const FBaseParticle& InParticle,
    const FVector& InParticlePosition,
    const FVector& InCameraPosition
)
{
    FVector DirToCamera = InCameraPosition - InParticlePosition;
    FCameraOffsetParticlePayload* CameraPayload = ((FCameraOffsetParticlePayload*)((uint8*)(&InParticle) + InCameraPayloadOffset));

    return GetCameraOffset(CameraPayload->Offset, DirToCamera);
}

void FDynamicSpriteEmitterDataBase::BuildViewFillData(
    int32 InVertexCount,
    int32 InVertexSize,
    int32 InDynamicParameterVertexStride,
    FGlobalDynamicIndexBuffer& DynamicIndexBuffer,
    FGlobalDynamicVertexBuffer& DynamicVertexBuffer,
    FGlobalDynamicVertexBuffer::FAllocation& DynamicVertexAllocation,
    FGlobalDynamicIndexBuffer::FAllocation& DynamicIndexAllocation,
    FGlobalDynamicVertexBuffer::FAllocation* DynamicParameterAllocation,
    FAsyncBufferFillData& Data) const
{
    if (Data.VertexSize != 0 && Data.VertexSize != InVertexSize)
    {
        UE_LOG(ELogLevel::Error, TEXT("Dynamic vertex size mismatch: %d != %d"), Data.VertexSize, InVertexSize);
        return;
    }

    DynamicVertexAllocation = DynamicVertexBuffer.Allocate(InVertexCount * InVertexSize);

    Data.VertexData = DynamicVertexAllocation.Buffer;
    Data.VertexCount = InVertexCount;
    Data.VertexSize = InVertexSize;

    int32 NumIndices, IndexStride;
    GetIndexAllocInfo(NumIndices, IndexStride);
    if (IndexStride <= 0)
    {
        UE_LOG(ELogLevel::Error, TEXT("Dynamic index size mismatch: %d"), IndexStride);
        return;
    }

    DynamicIndexAllocation = DynamicIndexBuffer.Allocate(NumIndices, IndexStride);
    Data.IndexData = DynamicIndexAllocation.Buffer;
    Data.IndexCount = NumIndices;

    Data.DynamicParameterData = NULL;

    if (bUsesDynamicParameter)
    {
        if (InDynamicParameterVertexStride <= 0)
        {
            UE_LOG(ELogLevel::Error, TEXT("Dynamic parameter vertex size mismatch: %d"), InDynamicParameterVertexStride);
            return;
        }

        *DynamicParameterAllocation = DynamicVertexBuffer.Allocate(InVertexCount * InDynamicParameterVertexStride);

        Data.DynamicParameterData = DynamicParameterAllocation->Buffer;
    }
}

bool FDynamicSpriteEmitterData::GetVertexAndIndexDataNonInstanced(void* VertexData, void* DynamicParameterVertexData, void* FillIndexData, FParticleOrder* ParticleOrder, const FVector& InCameraPosition, const FMatrix& InLocalToWorld, int32 NumVerticesPerParticle) const
{
    int32 ParticleCount = Source.ActiveParticleCount;
    // 'clamp' the number of particles actually drawn
    //@todo.SAS. If sorted, we really want to render the front 'N' particles...
    // right now it renders the back ones. (Same for SubUV draws)
    if ((Source.MaxDrawCount >= 0) && (ParticleCount > Source.MaxDrawCount))
    {
        ParticleCount = Source.MaxDrawCount;
    }

    // Put the camera origin in the appropriate coordinate space.
    FVector CameraPosition = InCameraPosition;
    if (Source.bUseLocalSpace)
    {
        FMatrix InvSelf = FMatrix::Inverse(InLocalToWorld);
        CameraPosition = InvSelf.TransformPosition(InCameraPosition);
    }

    // Pack the data
    int32	ParticleIndex;
    int32	ParticlePackingIndex = 0;
    int32	IndexPackingIndex = 0;

    int32 VertexStride = sizeof(FParticleSpriteVertexNonInstanced) * NumVerticesPerParticle;
    int32 VertexDynamicParameterStride = sizeof(FParticleVertexDynamicParameter) * NumVerticesPerParticle;

    uint8* TempVert = (uint8*)VertexData;
    uint8* TempDynamicParameterVert = (uint8*)DynamicParameterVertexData;
    FParticleSpriteVertexNonInstanced* FillVertex;
    FParticleVertexDynamicParameter* DynFillVertex;

    FVector4 DynamicParameterValue(1.0f, 1.0f, 1.0f, 1.0f);
    FVector ParticlePosition;
    FVector ParticleOldPosition;
    float SubImageIndex = 0.0f;

    const uint8* ParticleData = Source.DataContainer.ParticleData;
    const uint16* ParticleIndices = Source.DataContainer.ParticleIndices;
    const FParticleOrder* OrderedIndices = ParticleOrder;

    for (int32 i = 0; i < ParticleCount; i++)
    {
        ParticleIndex = OrderedIndices ? OrderedIndices[i].ParticleIndex : i;
        DECLARE_PARTICLE_CONST(Particle, ParticleData + Source.ParticleStride * ParticleIndices[ParticleIndex]);
        if (i + 1 < ParticleCount)
        {
            int32 NextIndex = OrderedIndices ? OrderedIndices[i + 1].ParticleIndex : (i + 1);
            DECLARE_PARTICLE_CONST(NextParticle, ParticleData + Source.ParticleStride * ParticleIndices[NextIndex]);
        }

        const FVector2D Size = GetParticleSize(Particle, Source);

        ParticlePosition = Particle.Location;
        ParticleOldPosition = Particle.OldLocation;

        ApplyOrbitToPosition(Particle, Source, InLocalToWorld, ParticlePosition, ParticleOldPosition);

        if (Source.CameraPayloadOffset != 0)
        {
            FVector CameraOffset = GetCameraOffsetFromPayload(Source.CameraPayloadOffset, Particle, ParticlePosition, CameraPosition);
            ParticlePosition += CameraOffset;
            ParticleOldPosition += CameraOffset;
        }

        if (Source.SubUVDataOffset > 0)
        {
            FFullSubUVPayload* SubUVPayload = (FFullSubUVPayload*)(((uint8*)&Particle) + Source.SubUVDataOffset);
            SubImageIndex = SubUVPayload->ImageIndex;
        }

        if (Source.DynamicParameterDataOffset > 0)
        {
            GetDynamicValueFromPayload(Source.DynamicParameterDataOffset, Particle, DynamicParameterValue);
        }

        FillVertex = (FParticleSpriteVertexNonInstanced*)TempVert;

        const FVector2D* SubUVVertexData = nullptr;

        //if (Source.RequiredModule->bCutoutTexureIsValid)
        //{
        //    const int32 SubImageIndexInt = FMath::TruncToInt(SubImageIndex);
        //    int32 FrameIndex = SubImageIndexInt % Source.RequiredModule->NumFrames;

        //    if (SubImageIndexInt < 0)
        //    {
        //        // Mod operator returns remainder toward zero, not toward negative which is what we want
        //        FrameIndex = Source.RequiredModule->NumFrames - SubImageIndexInt;
        //    }

        //    SubUVVertexData = &Source.RequiredModule->FrameData[FrameIndex];
        //}

        //const bool bHasUVVertexData = SubUVVertexData && Source.RequiredModule->bCutoutTexureIsValid;

        for (int32 VertexIndex = 0; VertexIndex < NumVerticesPerParticle; ++VertexIndex)
        {
            //if (bHasUVVertexData)
            //{
            //    // Warning: not supporting UV flipping with cutout geometry in the non-instanced path
            //    FillVertex[VertexIndex].UV = SubUVVertexData[VertexIndex];
            //}
            //else
            //{
                if (VertexIndex == 0)
                {
                    FillVertex[VertexIndex].UV = FVector2D(0.0f, 0.0f);
                }
                if (VertexIndex == 1)
                {
                    FillVertex[VertexIndex].UV = FVector2D(0.0f, 1.0f);
                }
                if (VertexIndex == 2)
                {
                    FillVertex[VertexIndex].UV = FVector2D(1.0f, 1.0f);
                }
                if (VertexIndex == 3)
                {
                    FillVertex[VertexIndex].UV = FVector2D(1.0f, 0.0f);
                }
            //}

            FillVertex[VertexIndex].Position = FVector(ParticlePosition);
            FillVertex[VertexIndex].RelativeTime = Particle.RelativeTime;
            FillVertex[VertexIndex].OldPosition = FVector(ParticleOldPosition);
            // Create a floating point particle ID from the counter, map into approximately 0-1
            FillVertex[VertexIndex].ParticleId = (Particle.Flags & STATE_CounterMask) / 10000.0f;
            //FillVertex[VertexIndex].Size = FVector2D(GetParticleSizeWithUVFlipInSign(Particle, Size));
            FillVertex[VertexIndex].Size = FVector2D(1.0f, 1.0f); // Size Module 없어서 임시적으로 1.0f로 설정
            FillVertex[VertexIndex].Rotation = Particle.Rotation;
            FillVertex[VertexIndex].SubImageIndex = SubImageIndex;
            FillVertex[VertexIndex].Color = Particle.Color;
        }

        if (bUsesDynamicParameter)
        {
            DynFillVertex = (FParticleVertexDynamicParameter*)TempDynamicParameterVert;

            for (int32 VertexIndex = 0; VertexIndex < NumVerticesPerParticle; ++VertexIndex)
            {
                DynFillVertex[VertexIndex].DynamicValue[0] = DynamicParameterValue.X;
                DynFillVertex[VertexIndex].DynamicValue[1] = DynamicParameterValue.Y;
                DynFillVertex[VertexIndex].DynamicValue[2] = DynamicParameterValue.Z;
                DynFillVertex[VertexIndex].DynamicValue[3] = DynamicParameterValue.W;
            }
            TempDynamicParameterVert += VertexDynamicParameterStride;
        }
        TempVert += VertexStride;
    }

    return true;
}


void FDynamicSpriteEmitterDataBase::SortSpriteParticles(int32 SortMode, bool bLocalSpace,
    int32 ParticleCount, const uint8* ParticleData, int32 ParticleStride, const uint16* ParticleIndices,
    const FEditorViewportClient* View, const FMatrix& LocalToWorld, FParticleOrder* ParticleOrder) const
{
    if (SortMode == PSORTMODE_ViewProjDepth)
    {
        for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
        {
            DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
            float InZ;
            if (bLocalSpace)
            {
                InZ = (View->View * View->Projection).TransformFVector4(LocalToWorld.TransformPosition(Particle.Location)).W;
            }
            else
            {
                InZ = (View->View * View->Projection).TransformFVector4(Particle.Location).W;
            }
            ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;

            ParticleOrder[ParticleIndex].Z = InZ;
        }
        std::sort(
            ParticleOrder,
            ParticleOrder + ParticleCount,
            [](const FParticleOrder& A, const FParticleOrder& B)
            {
                return A.Z > B.Z;  // Z 내림차순
            }
        );
    }
    else if (SortMode == PSORTMODE_DistanceToView)
    {
        for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
        {
            DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
            float InZ;
            FVector Position;
            if (bLocalSpace)
            {
                Position = LocalToWorld.TransformPosition(Particle.Location);
            }
            else
            {
                Position = Particle.Location;
            }
            InZ = (View->GetCameraLocation() - Position).SizeSquared();
            ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
            ParticleOrder[ParticleIndex].Z = InZ;
        }
        std::sort(
            ParticleOrder,
            ParticleOrder + ParticleCount,
            [](const FParticleOrder& A, const FParticleOrder& B)
            {
                return A.Z > B.Z;  // Z 내림차순
            }
        );
    }
    else if (SortMode == PSORTMODE_Age_OldestFirst)
    {
        for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
        {
            DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
            ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
            ParticleOrder[ParticleIndex].C = Particle.Flags & STATE_CounterMask;
        }
        std::sort(
            ParticleOrder,
            ParticleOrder + ParticleCount,
            [](const FParticleOrder& A, const FParticleOrder& B)
            {
                return A.C > B.C;  // Z 내림차순
            }
        );
    }
    else if (SortMode == PSORTMODE_Age_NewestFirst)
    {
        for (int32 ParticleIndex = 0; ParticleIndex < ParticleCount; ParticleIndex++)
        {
            DECLARE_PARTICLE(Particle, ParticleData + ParticleStride * ParticleIndices[ParticleIndex]);
            ParticleOrder[ParticleIndex].ParticleIndex = ParticleIndex;
            ParticleOrder[ParticleIndex].C = (~Particle.Flags) & STATE_CounterMask;
        }
        std::sort(
            ParticleOrder,
            ParticleOrder + ParticleCount,
            [](const FParticleOrder& A, const FParticleOrder& B)
            {
                return A.C > B.C;  // Z 내림차순
            }
        );
    }
}
