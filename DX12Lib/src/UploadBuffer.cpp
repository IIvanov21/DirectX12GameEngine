#include "DX12LibPCH.h"

#include <dx12lib/UploadBuffer.h>

#include <dx12lib/Device.h>
#include <dx12lib/Helpers.h>

using namespace DX12_Library;

UploadBuffer::UploadBuffer( Device& device, size_t pageSize )
: m_Device( device )
, m_PageSize( pageSize )
{}

UploadBuffer::~UploadBuffer() {}

// The Allocate method is used to allocate a chunk of memory from a memory page.
UploadBuffer::Allocation UploadBuffer::Allocate( size_t sizeInBytes, size_t alignment )
{
    if ( sizeInBytes > m_PageSize )
    {
        throw std::bad_alloc();
    }

    // If there is no current page, or the requested allocation exceeds the
    // remaining space in the current page, request a new page.
    if ( !m_CurrentPage || !m_CurrentPage->HasSpace( sizeInBytes, alignment ) )
    {
        m_CurrentPage = RequestPage();
    }
    // Allocate takes two arguments
    // The size of the allocation in bytes
    // The memory alignment of the allocation in bytes.
    // For example: allocation for constant buffers must be aligned to 256 bytes. 
    return m_CurrentPage->Allocate( sizeInBytes, alignment );
}

// If either the allocator or the Current Page doesnt have the available space
// a new page gets retrieved from Available Pages or a new one gets created.
std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
    std::shared_ptr<Page> page;

    if ( !m_AvailablePages.empty() )
    {
        page = m_AvailablePages.front();
        m_AvailablePages.pop_front();
    }
    else
    {
        page = std::make_shared<Page>( m_Device, m_PageSize );
        m_PagePool.push_back( page );
    }

    return page;
}

// The Reset method is used to reset all of the memory allocations so that they can be reused
// for the next frame(command list recording).
// The Upload Buffer can only be reset if all of the allocations from it are
// no longer on the command queue.
void UploadBuffer::Reset()
{
    m_CurrentPage = nullptr;
    // Reset all available pages.
    m_AvailablePages = m_PagePool;

    for ( auto page: m_AvailablePages )
    {
        // Reset the page for new allocations.
        page->Reset();
    }
}

UploadBuffer::Page::Page( Device& device, size_t sizeInBytes )
: m_Device( device )
, m_PageSize( sizeInBytes )
, m_Offset( 0 )
, m_CPUPtr( nullptr )
, m_GPUPtr( D3D12_GPU_VIRTUAL_ADDRESS( 0 ) )
{
    auto d3d12Device = m_Device.GetD3D12Device();
    // Create a commited resource that is large enough to
    // store buffer data passed in a single page 
    ThrowIfFailed( d3d12Device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD ), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer( m_PageSize ), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS( &m_d3d12Resource ) ) );

    // Get the GPU and CPU addresses and map them
    // As long as the resource is created in the Upload buffer it is
    // okay to leave them mapped.
    m_d3d12Resource->SetName( L"Upload Buffer (Page)" );

    m_GPUPtr = m_d3d12Resource->GetGPUVirtualAddress();
    m_d3d12Resource->Map( 0, nullptr, &m_CPUPtr );
}

// The Page destructor unmaps the resource memory and
// resets the CPU and GPU pointers.
UploadBuffer::Page::~Page()
{
    m_d3d12Resource->Unmap( 0, nullptr );
    m_CPUPtr = nullptr;
    m_GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS( 0 );
}

// The Page::HasSpace checks to see if the page has enough space to satisfy the
// requested allocation. Returns: True if it does.. False if it doesn't..
bool UploadBuffer::Page::HasSpace( size_t sizeInBytes, size_t alignment ) const
{
    size_t alignedSize   = Math::AlignUp( sizeInBytes, alignment );
    size_t alignedOffset = Math::AlignUp( m_Offset, alignment );

    return alignedOffset + alignedSize <= m_PageSize;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate( size_t sizeInBytes, size_t alignment )
{
    // Double check if the page size is enough to satisfy the allocation
    if ( !HasSpace( sizeInBytes, alignment ) )
    {
        // Can't allocate space from page.
        throw std::bad_alloc();
    }
    // The size and the starting address should be aligned to ensure correctness
    size_t alignedSize = Math::AlignUp( sizeInBytes, alignment );
    m_Offset           = Math::AlignUp( m_Offset, alignment );
    // The GPU and CPU addresses are written to the allocation structure
    Allocation allocation;
    allocation.CPU = static_cast<uint8_t*>( m_CPUPtr ) + m_Offset;
    allocation.GPU = m_GPUPtr + m_Offset;
    // The offset pointer gets incremented by the aligned size
    m_Offset += alignedSize;

    return allocation;
}

// Simply reset the page's pointer to 0 so it can be used again
void UploadBuffer::Page::Reset()
{
    m_Offset = 0;
}