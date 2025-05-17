#include "GlobalRenderResource.h"
#include "HAL/PlatformMemory.h"

TDynamicBufferPool<FDynamicVertexBuffer> GDynamicVertexBufferPool;
TDynamicBufferPool<FDynamicIndexBuffer> GDynamicIndexBufferPool;

FGlobalDynamicVertexBuffer::FAllocation FGlobalDynamicVertexBuffer::Allocate(uint32 SizeInBytes)
{
    FAllocation Allocation;

    if (VertexBuffers.IsEmpty() || VertexBuffers.Last()->AllocatedByteCount + SizeInBytes > VertexBuffers.Last()->BufferSize)
    {
        VertexBuffers.Emplace(GDynamicVertexBufferPool.Acquire(SizeInBytes, 0));
    }

    FDynamicVertexBuffer* VertexBuffer = VertexBuffers.Last();

    if (!(VertexBuffer->AllocatedByteCount + SizeInBytes <= VertexBuffer->BufferSize))
    {
        UE_LOG(ELogLevel::Error, TEXT("Global vertex buffer allocation failed : BufferSize = % d AllocatedByteCount = % d SizeInBytes = % d"), VertexBuffer->BufferSize, VertexBuffer->AllocatedByteCount, SizeInBytes);
        return Allocation;
    }

    Allocation.Buffer = VertexBuffer->MappedBuffer + VertexBuffer->AllocatedByteCount;
    Allocation.VertexBuffer = VertexBuffer->GPUBuffer;
    Allocation.VertexOffset = VertexBuffer->AllocatedByteCount;
    VertexBuffer->AllocatedByteCount += Align(SizeInBytes, 16);
    return Allocation;
}

void FGlobalDynamicVertexBuffer::Commit()
{
    //// 1) 이번 프레임에 Allocate() 된 모든 버퍼를 Unmap()
    //for (FDynamicVertexBuffer* VB : VertexBuffers)
    //{
    //    if (VB && VB->GPUBuffer && VB->MappedBuffer)
    //    {
    //        GD3DContext->Unmap(VB->GPUBuffer, 0);
    //        VB->MappedBuffer = nullptr;
    //    }
    //}

    //// 2) 버퍼 목록 초기화 → 다음 프레임에 새로 Allocate 시작
    //VertexBuffers.Reset();
}

FGlobalDynamicIndexBuffer::FAllocation FGlobalDynamicIndexBuffer::Allocate(uint32 NumIndices, uint32 IndexStride)
{
    FAllocation Allocation;

    if (IndexStride != 2 && IndexStride != 4)
    {
        return Allocation;
    }

    const uint32 SizeInBytes = NumIndices * IndexStride;

    TArray<FDynamicIndexBuffer*>& IndexBuffers = (IndexStride == 2)
        ? IndexBuffers16
        : IndexBuffers32;

    if (IndexBuffers.IsEmpty() || IndexBuffers.Last()->AllocatedByteCount + SizeInBytes > IndexBuffers.Last()->BufferSize)
    {
        IndexBuffers.Emplace(GDynamicIndexBufferPool.Acquire(SizeInBytes, IndexStride));
    }

    FDynamicIndexBuffer* IndexBuffer = IndexBuffers.Last();

    if (!(IndexBuffer->AllocatedByteCount + SizeInBytes <= IndexBuffer->BufferSize))
    {
        UE_LOG(ELogLevel::Error, TEXT("Global index buffer allocation failed : BufferSize = % d AllocatedByteCount = % d SizeInBytes = % d"), IndexBuffer->BufferSize, IndexBuffer->AllocatedByteCount, SizeInBytes);
        return Allocation;
    }
    Allocation.Buffer = IndexBuffer->MappedBuffer + IndexBuffer->AllocatedByteCount;
    Allocation.IndexBuffer = IndexBuffer->GPUBuffer;
    Allocation.FirstIndex = IndexBuffer->AllocatedByteCount / IndexStride;
    IndexBuffer->AllocatedByteCount += Align(SizeInBytes, 16);
    return Allocation;
}

void FGlobalDynamicIndexBuffer::Commit()
{
}

