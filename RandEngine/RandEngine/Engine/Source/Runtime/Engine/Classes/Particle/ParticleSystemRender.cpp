#include "ParticleHelper.h"

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
