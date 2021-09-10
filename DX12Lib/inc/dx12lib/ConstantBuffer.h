#pragma once

#include "Buffer.h"

#include <d3d12.h> // For ID3D12Resource
#include <wrl/client.h> // For ComPtr
/*
 * A constant buffer is structure which allows you to
 * constantly supply shader constants data to the GPU/pipeline.
 * You can use constant buffer to store data that is manipulate by the user and utilized by the GPU.
 */
namespace DX12_Library
{
class ConstantBuffer : public Buffer
{
public:
    /*
     * It is essential to manage data in bytes when transferring data to GPU memory.
     */
    size_t GetSizeInBytes() const
    {
        return m_SizeInBytes;
    }

protected:
    ConstantBuffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource );
    virtual ~ConstantBuffer();

private:
    size_t               m_SizeInBytes;
};
}  // namespace DX12_Library