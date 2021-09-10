#include "DX12LibPCH.h"

#include <dx12lib/DescriptorAllocation.h>

#include <dx12lib/DescriptorAllocatorPage.h>

using namespace DX12_Library;
// The construct simply initialized the variables
DescriptorAllocation::DescriptorAllocation()
    : m_Descriptor{ 0 }
    , m_NumHandles( 0 )
    , m_DescriptorSize( 0 )
    , m_Page( nullptr )
{}


DescriptorAllocation::DescriptorAllocation( D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numHandles, uint32_t descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page )
    : m_Descriptor( descriptor )
    , m_NumHandles( numHandles )
    , m_DescriptorSize( descriptorSize )
    , m_Page( page )
{}


DescriptorAllocation::~DescriptorAllocation()
{
    Free();
}

// The move consturctor allows the descriptor allocation to be moved.
// The original DescriptorAllocation is made invalid but the allocation is not freed.
DescriptorAllocation::DescriptorAllocation( DescriptorAllocation&& allocation ) noexcept
    : m_Descriptor(allocation.m_Descriptor)
    , m_NumHandles(allocation.m_NumHandles)
    , m_DescriptorSize(allocation.m_DescriptorSize)
    , m_Page(std::move(allocation.m_Page))
{
    allocation.m_Descriptor.ptr = 0;
    allocation.m_NumHandles = 0;
    allocation.m_DescriptorSize = 0;
}

// The move assignment operator is the same as the move constructor
// but the allocation must be freed.
DescriptorAllocation& DescriptorAllocation::operator=( DescriptorAllocation&& other ) noexcept
{
    // Free this descriptor if it points to anything.
    Free();

    m_Descriptor = other.m_Descriptor;
    m_NumHandles = other.m_NumHandles;
    m_DescriptorSize = other.m_DescriptorSize;
    m_Page = std::move( other.m_Page );

    other.m_Descriptor.ptr = 0;
    other.m_NumHandles = 0;
    other.m_DescriptorSize = 0;

    return *this;
}

// If the descriptor allocation either goes out of scope or is replace by another
// descriptor. It must be freed. The Free method returns the Descriptor Allocation
// back to the Descriptor Allocator Page.
void DescriptorAllocation::Free()
{
    if ( !IsNull() && m_Page )
    {
        m_Page->Free( std::move( *this ) );
        
        m_Descriptor.ptr = 0;
        m_NumHandles = 0;
        m_DescriptorSize = 0;
        m_Page.reset();
    }
}

// Check if the Descriptor Handle is valid
bool DescriptorAllocation::IsNull() const
{
    return m_Descriptor.ptr == 0;
}

// Get a descriptor at a particular offset in the allocation.
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle( uint32_t offset ) const
{
    assert( offset < m_NumHandles );
    return { m_Descriptor.ptr + ( m_DescriptorSize * offset ) };
}

uint32_t DescriptorAllocation::GetNumHandles() const
{
    return m_NumHandles;
}


std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocation::GetDescriptorAllocatorPage() const
{
    return m_Page;
}
