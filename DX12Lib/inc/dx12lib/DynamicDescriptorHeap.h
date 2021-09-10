#pragma once

#include "d3dx12.h"

#include <wrl.h>

#include <cstdint>
#include <memory>
#include <queue>
/*
 * A dynamic descriptor is a shader visible(resource ready to render) descriptor heap, divided into two parts.
 * These descriptors are used for some transition resources that their descriptor table cannot reuse due to being
 * too small or needed to be dumped.
 */
namespace DX12_Library
{

class Device;
class CommandList;
class RootSignature;

class DynamicDescriptorHeap
{
public:
    // The constructor takes two params: descriptor heap type and
    // number of descriptors to allocate per heap.
    DynamicDescriptorHeap( Device& device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptorsPerHeap = 1024 );

    virtual ~DynamicDescriptorHeap();

    /*
     * Stage Descriptors method is used to copy any number of contiguous CPU visible descriptors to the
     * DynamicDescriptorHeap. This method copies only the descriptor handles but not it's contents. Due to this CPU
     * visible descriptors can't be reused or overwritten until CommitStageDescriptors method is invoked.
     */
    void StageDescriptors( uint32_t rootParameterIndex,//The index of root parameter to copy the descriptors to. Configure to DESCRIPTOR TABLE in the current bound root signature.
        uint32_t offset, //Offset withing the descriptor table to copy the the descriptors to. Value ranges: Offset+numDescriptors
        uint32_t numDescriptors,//Number of contiguous descriptors to copy starting from srcDescriptors
        const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptors );//The base descriptors to start copying descriptors from.

    /**
     * Stage an inline CBV descriptor.
     */
    void StageInlineCBV( uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation );

    /**
     * Stage an inline SRV descriptor.
     */
    void StageInlineSRV( uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation );

    /**
     * Stage an inline UAV descriptor.
     */
    void StageInlineUAV( uint32_t rootParamterIndex, D3D12_GPU_VIRTUAL_ADDRESS bufferLocation );


    /*
     * Copy all of the staged descriptors to the GPU visible descriptor heap and
     * bind the descriptor heap and the descriptor tables to the command list.
     * The function object is used to set the GPU visible descriptors on the command list.
     * The functions before Draw: SetGraphicsRootDescriptorTable is used to bind the descriptor to the graphics
     * pipeline. The function before Dispatch: SetComputeRootDescriptorTable is used to bind the descriptors to the
     * compute pipeline. Since DynamicDescriptorHeap can't determine which to use they are passed in as argument to the
     * function.
     */
    void CommitStagedDescriptorsForDraw( CommandList& commandList );
    void CommitStagedDescriptorsForDispatch( CommandList& commandList );

    /*
     * The CopyDescriptor is used to copy a single CPU visible descriptor into a GPU visible descriptor heap.
     * This method accepts a CommandList as its only argument in case the currently bound descriptor heap needs to
     * be updated on the command list as a result of copying the descriptor.
     * This is useful for the ClearUnorderedAccessView _Floar or _UInt methods as they both require both a CPU and GPU
     * visible descriptors for UAV resource.
     */
    D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor( CommandList& comandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor );

    /*
     * Parse the DynamicDescriptorHeap to inform it of any chages to the currently bound root signature on the command
     * list. This method updates the layput of the descriptors in the descriptors cache to match the descriptor layout
     * in the root signature.
     */
    void ParseRootSignature( const std::shared_ptr<RootSignature>& rootSignature );

    /*
     * The Reset method is used to reset the allocated descriptor heaps and descriptor cache after
     * the command queue is finished processing any commands that are referenced in DynamicDescriptorHeap.
     */
    void Reset();

protected:
private:
    // Request a descriptor heap if one is available.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RequestDescriptorHeap();
    // Create a new descriptor heap of no descriptor heap is available.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap();

    // Compute the number of stale descriptors that need to be copied
    // to GPU visible descriptor heap.
    uint32_t ComputeStaleDescriptorCount() const;

    /**
     * Copy all of the staged descriptors to the GPU visible descriptor heap and
     * bind the descriptor heap and the descriptor tables to the command list.
     * The passed-in function object is used to set the GPU visible descriptors
     * on the command list. Two possible functions are:
     *   * Before a draw    : ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable
     *   * Before a dispatch: ID3D12GraphicsCommandList::SetComputeRootDescriptorTable
     *
     * Since the DynamicDescriptorHeap can't know which function will be used, it must
     * be passed as an argument to the function.
     */
    void CommitDescriptorTables(
        CommandList&                                                                         commandList,
        std::function<void( ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE )> setFunc );
    void CommitInlineDescriptors(
        CommandList& commandList, const D3D12_GPU_VIRTUAL_ADDRESS* bufferLocations, uint32_t& bitMask,
        std::function<void( ID3D12GraphicsCommandList*, UINT, D3D12_GPU_VIRTUAL_ADDRESS )> setFunc );

    // The MaxDescriptorTables constant represents the maximum number of descriptor tables that can exist in the
    // root signature. For now 32 bit bitmask is used to indicate which entries of the root signature uses a descriptor
    // table.
    static const uint32_t MaxDescriptorTables = 32;

    // A structure that represents a descriptor table entry in the root signature.
    // Each entry in the descriptor cache stores the number of descriptors in the descriptor tbale and a pointer to the
    // descriptor handle in the DHC.
    struct DescriptorTableCache
    {
        DescriptorTableCache()
        : NumDescriptors( 0 )
        , BaseDescriptor( nullptr )
        {}

        // Reset the table cache.
        void Reset()
        {
            NumDescriptors = 0;
            BaseDescriptor = nullptr;
        }

        // The number of descriptors in this descriptor table.
        uint32_t NumDescriptors;
        // The pointer to the descriptor in the descriptor handle cache.
        D3D12_CPU_DESCRIPTOR_HANDLE* BaseDescriptor;
    };

    // The device that is used to create this descriptor heap.
    Device& m_Device;

    // Describes the type of descriptors that can be staged using this
    // dynamic descriptor heap.
    // Valid values are:
    //   * D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    //   * D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
    // This parameter also determines the type of GPU visible descriptor heap to
    // create.
    D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorHeapType;

    // The number of descriptors to allocate in new GPU visible descriptor heaps.
    uint32_t m_NumDescriptorsPerHeap;

    // The increment size of a descriptor.
    uint32_t m_DescriptorHandleIncrementSize;

    // The descriptor handle cache.
    std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> m_DescriptorHandleCache;

    // Descriptor handle cache per descriptor table.
    DescriptorTableCache m_DescriptorTableCache[MaxDescriptorTables];

    // Inline CBV
    D3D12_GPU_VIRTUAL_ADDRESS m_InlineCBV[MaxDescriptorTables];
    // Inline SRV
    D3D12_GPU_VIRTUAL_ADDRESS m_InlineSRV[MaxDescriptorTables];
    // Inline UAV
    D3D12_GPU_VIRTUAL_ADDRESS m_InlineUAV[MaxDescriptorTables];

    // Each bit in the bit mask represents the index in the root signature
    // that contains a descriptor table.
    uint32_t m_DescriptorTableBitMask;
    // Each bit set in the bit mask represents a descriptor table
    // in the root signature that has changed since the last time the
    // descriptors were copied.
    uint32_t m_StaleDescriptorTableBitMask;
    uint32_t m_StaleCBVBitMask;
    uint32_t m_StaleSRVBitMask;
    uint32_t m_StaleUAVBitMask;

    using DescriptorHeapPool = std::queue<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>>;

    DescriptorHeapPool m_DescriptorHeapPool;
    DescriptorHeapPool m_AvailableDescriptorHeaps;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CurrentDescriptorHeap;
    CD3DX12_GPU_DESCRIPTOR_HANDLE                m_CurrentGPUDescriptorHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE                m_CurrentCPUDescriptorHandle;

    uint32_t m_NumFreeHandles;
};
}  // namespace DX12_Library