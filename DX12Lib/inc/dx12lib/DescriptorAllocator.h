#pragma once

#include "DescriptorAllocation.h"

#include "d3dx12.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

namespace DX12_Library
{

    class DescriptorAllocatorPage;
class Device;

class DescriptorAllocator
{
public:
    /*The Allocate method allocates a number contiguous descriptors from a descriptor heap.
     * By default only a single descriptor is allocated, but more can be specified if required.*/
    DX12_Library::DescriptorAllocation Allocate( uint32_t numDescriptors = 1 );

    /**
     * When the frame has completed, the stale descriptors can be released.
     */
    void ReleaseStaleDescriptors();

protected:
    friend class std::default_delete<DescriptorAllocator>;

    /*The constructor takes two parameters.
     * The first param is the type of descriptors that will be allocated:
     * CBV_SRV_UAV,RTV, SAMPLER or DSV.
     * The second param is the number of descriptors per heap. Default value:256
     */
    DescriptorAllocator( Device& device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256 );
    virtual ~DescriptorAllocator();

private:
    // Alias of std::vector of Descriptor Allocator Pages
    using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

    // Internal method that is used to create a new allocator page if
    // there are no pages in the allocator pool to satisfy the allocation request.
    std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

    // The device that was use to create this DescriptorAllocator.
    Device&                    m_Device;
    D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType;
    uint32_t                   m_NumDescriptorsPerHeap;

    DescriptorHeapPool m_HeapPool;

    // Set of indices of available pages in the HeapPool
    // If all Descriptor Allocator Pages have been exhausted then the index of that page
    // in HeapPool vector is removed from Available Heap to help skip empty pages when looking for
    // DescriptorAllocatorPage that can satisfy the requested allocation.
    std::set<size_t> m_AvailableHeaps;

    std::mutex m_AllocationMutex;
};
}  // namespace DX12_Library