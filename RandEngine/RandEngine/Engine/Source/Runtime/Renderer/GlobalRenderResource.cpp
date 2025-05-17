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
    ID3D11DeviceContext* D3DContext = GDynamicVertexBufferPool.Context;
    // 1) 이번 프레임에 Allocate() 된 모든 버퍼를 Unmap()
    for (FDynamicVertexBuffer* VB : VertexBuffers)
    {
        if (VB && VB->MappedBuffer)
        {
            // Context 는 전역이나 멤버로 보관하신 DeviceContext 를 사용하세요
            VB->Unmap(D3DContext);
        }
    }

    // 2) 다음 프레임을 위해 할당 기록 리셋
    VertexBuffers.Empty();
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
    ID3D11DeviceContext* D3DContext = GDynamicIndexBufferPool.Context;

    // 16비트 인덱스 버퍼 정리
    for (FDynamicIndexBuffer* IB : IndexBuffers16)
    {
        if (IB && IB->MappedBuffer)
        {
            IB->Unmap(D3DContext);
        }
    }
    IndexBuffers16.Empty();

    // 32비트 인덱스 버퍼 정리
    for (FDynamicIndexBuffer* IB : IndexBuffers32)
    {
        if (IB && IB->MappedBuffer)
        {
            IB->Unmap(D3DContext);
        }
    }
    IndexBuffers32.Empty();
}

