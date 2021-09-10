#pragma once

#include "Buffer.h"
#include "DescriptorAllocation.h"

#include <d3dx12.h>

namespace DX12_Library
{
/*
 * This buffer allows the engine to address the contents of the buffer
 * by byte offset. This often used with Descriptors to ensure the correct elements get
 * passed in the correct slots. Such as a correct Texture type get's passed to the correct Texture slot in the shader. 
 */
class Device;

class ByteAddressBuffer : public Buffer
{
public:
    size_t GetBufferSize() const
    {
        return m_BufferSize;
    }

protected:
    ByteAddressBuffer( Device& device, const D3D12_RESOURCE_DESC& resDesc );
    ByteAddressBuffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource );
    virtual ~ByteAddressBuffer() = default;

private:
    size_t m_BufferSize;
};
}  // namespace DX12_Library