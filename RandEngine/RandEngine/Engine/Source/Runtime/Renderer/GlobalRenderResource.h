#pragma once
#include "RenderResources.h"

template <typename BufferType>
class TDynamicBuffer;

using FDynamicVertexBuffer = TDynamicBuffer<ID3D11Buffer>;

/**
 * An individual dynamic vertex buffer.
 */
template <typename BufferType>
class TDynamicBuffer
{
public:
    /** The aligned size of all dynamic vertex buffers. */
    enum { ALIGNMENT = (1 << 16) }; // 64KB
    /** Pointer to the vertex buffer mapped in main memory. */
    uint8* MappedBuffer = nullptr;
    /** The GPU buffer handle. */
    BufferType* GPUBuffer = nullptr;
    /** Size of the vertex buffer in bytes. */
    uint32 BufferSize = 0;
    /** Number of bytes currently allocated from the buffer. */
    uint32 AllocatedByteCount = 0;
    /** Stride of the buffer in bytes. */
    uint32 Stride = 0;
    /** Last render thread frame this resource was used in. */
    uint64 LastUsedFrame = 0;

    /** Default constructor. */
    explicit TDynamicBuffer(uint32 InMinBufferSize, uint32 InStride)
        : MappedBuffer(NULL)
        , GPUBuffer(NULL)
        , BufferSize(FMath::Max<uint32>(Align(InMinBufferSize, ALIGNMENT), ALIGNMENT))
        , AllocatedByteCount(0)
        , Stride(InStride)
    {
    }

    virtual ~TDynamicBuffer() = default;

    void Init(ID3D11Device* Device, ID3D11DeviceContext* Context)
    {
        // (1) 기존 리소스 있으면 해제
        if (GPUBuffer)
        {
            // unmap if still mapped
            if (MappedBuffer)
            {
                Context->Unmap(GPUBuffer, 0);
                MappedBuffer = nullptr;
            }
            GPUBuffer->Release();
            GPUBuffer = nullptr;
        }

        // (2) 버퍼 설명 설정
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = BufferSize;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        // 인덱스용이라면 D3D11_BIND_INDEX_BUFFER 로 바꿔주세요
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        Desc.MiscFlags = 0;
        Desc.StructureByteStride = Stride;

        // (3) GPU 버퍼 생성
        HRESULT hr = Device->CreateBuffer(&Desc, nullptr, &GPUBuffer);
        if (!SUCCEEDED(hr))
        {
            UE_LOG(ELogLevel::Error, TEXT("CreateBuffer failed"));
            return;
        }

        // (4) 바로 Map 하여 쓰기 가능한 포인터 얻기
        D3D11_MAPPED_SUBRESOURCE MappedRes;
        Context->Map(GPUBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedRes);
        MappedBuffer = reinterpret_cast<uint8*>(MappedRes.pData);

        // (5) 할당 카운트 초기화
        AllocatedByteCount = 0;
    }

    /** Commit 직전에 호출해서 Unmap */
    void Unmap(ID3D11DeviceContext* Context)
    {
        if (GPUBuffer && MappedBuffer)
        {
            Context->Unmap(GPUBuffer, 0);
            MappedBuffer = nullptr;
        }
    }
};

template <typename DynamicBufferType>
struct TDynamicBufferPool
{
    TArray<DynamicBufferType> PoolBuffers;
    DynamicBufferType* CurrentBuffer = nullptr;

    ID3D11Device* Device;
    ID3D11DeviceContext* Context;

    TDynamicBufferPool() = default;

    void Initialize(ID3D11Device* InDevice, ID3D11DeviceContext* InContext)
    {
        Device = InDevice;
        Context = InContext;
    }

    DynamicBufferType* Acquire(uint32 SizeInBytes, uint32 Stride)
    {
        const uint32 MinimumBufferSize = 65536u;
        SizeInBytes = FMath::Max(SizeInBytes, MinimumBufferSize);

        // (1) 현재 버퍼가 유효하지 않거나, 크기가 작거나, 스트라이드가 다르면
        if (!CurrentBuffer
            || CurrentBuffer->BufferSize < SizeInBytes
            || CurrentBuffer->Stride != Stride)
        {
            // (2) 재사용 가능한 버퍼 탐색
            for (DynamicBufferType& Buf : PoolBuffers)
            {
                if (Buf.BufferSize >= SizeInBytes && Buf.Stride == Stride)
                {
                    CurrentBuffer = &Buf;

                    D3D11_MAPPED_SUBRESOURCE MappedRes;
                    Context->Map(CurrentBuffer->GPUBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedRes);
                    CurrentBuffer->MappedBuffer = reinterpret_cast<uint8*>(MappedRes.pData);

                    CurrentBuffer->AllocatedByteCount = 0;
                    break;
                }
            }

            // (3) 못 찾았으면 새로 생성
            if (!CurrentBuffer
                || CurrentBuffer->BufferSize < SizeInBytes
                || CurrentBuffer->Stride != Stride)
            {
                PoolBuffers.Emplace(SizeInBytes, Stride);
                CurrentBuffer = &PoolBuffers.Last();

                // GPU 버퍼 생성 + 맵
                CurrentBuffer->Init(Device, Context);
                CurrentBuffer->AllocatedByteCount = 0;
            }
        }

        return CurrentBuffer;
    }
};

// 인덱스 전용으로 Init() 동작만 바꾼 서브클래스
class FDynamicIndexBuffer : public TDynamicBuffer<ID3D11Buffer>
{
public:
    // 부모 템플릿 생성자 호출
    explicit FDynamicIndexBuffer(uint32 InMinBufferSize, uint32 InStride)
        : TDynamicBuffer<ID3D11Buffer>(InMinBufferSize, InStride)
    {
    }

    // 정점용 Init() 대신 인덱스용 Init() 구현
    void Init(ID3D11Device* Device, ID3D11DeviceContext* Context)
    {
        // (1) 기존 리소스 있으면 해제
        if (GPUBuffer)
        {
            if (MappedBuffer)
            {
                Context->Unmap(GPUBuffer, 0);
                MappedBuffer = nullptr;
            }
            GPUBuffer->Release();
            GPUBuffer = nullptr;
        }

        // (2) 버퍼 설명 – BindFlags 만 INDEX_BUFFER 로 변경
        D3D11_BUFFER_DESC Desc = {};
        Desc.ByteWidth = BufferSize;
        Desc.Usage = D3D11_USAGE_DYNAMIC;
        Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        Desc.MiscFlags = 0;
        Desc.StructureByteStride = Stride;  // 보통 sizeof(uint16) or sizeof(uint32)

        // (3) GPU 버퍼 생성
        HRESULT hr = Device->CreateBuffer(&Desc, nullptr, &GPUBuffer);
        if (!SUCCEEDED(hr))
        {
            UE_LOG(ELogLevel::Error, TEXT("Index CreateBuffer failed"));
            return;
        }

        // (4) 즉시 Map 해서 쓰기 포인터 확보
        D3D11_MAPPED_SUBRESOURCE MappedRes;
        Context->Map(GPUBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedRes);
        MappedBuffer = reinterpret_cast<uint8*>(MappedRes.pData);

        // (5) 할당 카운트 초기화
        AllocatedByteCount = 0;
    }
};

extern TDynamicBufferPool<FDynamicVertexBuffer> GDynamicVertexBufferPool;
extern TDynamicBufferPool<FDynamicIndexBuffer>  GDynamicIndexBufferPool;

struct FGlobalDynamicVertexBufferAllocation
{
    /** The location of the buffer in main memory. */
    uint8* Buffer = nullptr;

    /** The vertex buffer to bind for draw calls. */
    ID3D11Buffer* VertexBuffer = nullptr;

    /** The offset in to the vertex buffer. */
    uint32 VertexOffset = 0;

    /** Returns true if the allocation is valid. */
    FORCEINLINE bool IsValid() const
    {
        return Buffer != nullptr;
    }
};

/**
 * A system for dynamically allocating GPU memory for vertices.
 */
class FGlobalDynamicVertexBuffer
{
public:
    using FAllocation = FGlobalDynamicVertexBufferAllocation;

    FGlobalDynamicVertexBuffer() = default;

    ~FGlobalDynamicVertexBuffer()
    {
        Commit();
    }

    /**
     * Allocates space in the global vertex buffer.
     * @param SizeInBytes - The amount of memory to allocate in bytes.
     * @returns An FAllocation with information regarding the allocated memory.
     */
    FAllocation Allocate(uint32 SizeInBytes);

    /**
     * Commits allocated memory to the GPU.
     *		WARNING: Once this buffer has been committed to the GPU, allocations
     *		remain valid only until the next call to Allocate!
     */
    void Commit();

private:
    TArray<FDynamicVertexBuffer*> VertexBuffers;
};

struct FGlobalDynamicIndexBufferAllocation
{
    /** The location of the buffer in main memory. */
    uint8* Buffer = nullptr;

    /** The vertex buffer to bind for draw calls. */
    ID3D11Buffer* IndexBuffer = nullptr;

    /** The offset in to the index buffer. */
    uint32 FirstIndex = 0;

    /** Returns true if the allocation is valid. */
    FORCEINLINE bool IsValid() const
    {
        return Buffer != nullptr;
    }
};

struct FGlobalDynamicIndexBufferAllocationEx : public FGlobalDynamicIndexBufferAllocation
{
    FGlobalDynamicIndexBufferAllocationEx(const FGlobalDynamicIndexBufferAllocation& InRef, uint32 InNumIndices, uint32 InIndexStride)
        : FGlobalDynamicIndexBufferAllocation(InRef)
        , NumIndices(InNumIndices)
        , IndexStride(InIndexStride)
    {
    }

    /** The number of indices allocated. */
    uint32 NumIndices = 0;
    /** The allocation stride (2 or 4 bytes). */
    uint32 IndexStride = 0;
    /** The maximum value of the indices used. */
    uint32 MaxUsedIndex = 0;
};

/**
 * A system for dynamically allocating GPU memory for indices.
 */
class FGlobalDynamicIndexBuffer
{
public:
    using FAllocation = FGlobalDynamicIndexBufferAllocation;
    using FAllocationEx = FGlobalDynamicIndexBufferAllocationEx;

    FGlobalDynamicIndexBuffer() = default;

    ~FGlobalDynamicIndexBuffer()
    {
        Commit();
    }

    /**
     * Allocates space in the global index buffer.
     * @param NumIndices - The number of indices to allocate.
     * @param IndexStride - The size of an index (2 or 4 bytes).
     * @returns An FAllocation with information regarding the allocated memory.
     */
    FAllocation Allocate(uint32 NumIndices, uint32 IndexStride);

    /**
     * Helper function to allocate.
     * @param NumIndices - The number of indices to allocate.
     * @returns an FAllocation with information regarding the allocated memory.
     */
    template <typename IndexType>
    FORCEINLINE FAllocationEx Allocate(uint32 NumIndices)
    {
        return FAllocationEx(Allocate(NumIndices, sizeof(IndexType)), NumIndices, sizeof(IndexType));
    }

    /**
     * Commits allocated memory to the GPU.
     *		WARNING: Once this buffer has been committed to the GPU, allocations
     *		remain valid only until the next call to Allocate!
     */
    void Commit();

private:
    TArray<FDynamicIndexBuffer*> IndexBuffers16;
    TArray<FDynamicIndexBuffer*> IndexBuffers32;
};
