#pragma once

#include <dx12lib/DescriptorAllocation.h>

#include <d3d12.h>  // For D3D12_CONSTANT_BUFFER_VIEW_DESC and D3D12_CPU_DESCRIPTOR_HANDLE
#include <memory> // For std::shared_ptr
/*
 * The constant buffer view contains the actual shader data. The data in here can be
 * accessed by any GPU shader. If you look at Basic Shader it contains the functionality
 * for the Lit and Unlit PS shader as for the shaders they simply enable a variable to tell
 * the engine which function to use.
 */
namespace DX12_Library
{

class ConstantBuffer;
class Device;

class ConstantBufferView
{
public:
    std::shared_ptr<ConstantBuffer> GetConstantBuffer() const
    {
        return m_ConstantBuffer;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle()
    {
        return m_Descriptor.GetDescriptorHandle();
    }

protected:
    ConstantBufferView( Device& device, const std::shared_ptr<ConstantBuffer>& constantBuffer,
                        size_t offset = 0 );
    virtual ~ConstantBufferView() = default;

private:
    Device&                         m_Device;
    std::shared_ptr<ConstantBuffer> m_ConstantBuffer;
    DescriptorAllocation            m_Descriptor;
};

}  // namespace DX12_Library
