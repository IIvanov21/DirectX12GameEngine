#pragma once

#include <dx12lib/DescriptorAllocation.h>

#include <d3d12.h>  // For D3D12_SHADER_RESOURCE_VIEW_DESC and D3D12_CPU_DESCRIPTOR_HANDLE
#include <memory>   // For std::shared_ptr

namespace DX12_Library
{

class Device;
class Resource;
/*
 * This component allows the resource to be accessed by a shader.
 */
class ShaderResourceView
{
public:
    std::shared_ptr<Resource> GetResource() const
    {
        return m_Resource;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const
    {
        return m_Descriptor.GetDescriptorHandle();
    }

protected:
    ShaderResourceView( Device& device, const std::shared_ptr<Resource>& resource,
                        const D3D12_SHADER_RESOURCE_VIEW_DESC* srv = nullptr );
    virtual ~ShaderResourceView() = default;

private:
    Device&                   m_Device;
    std::shared_ptr<Resource> m_Resource;
    DescriptorAllocation      m_Descriptor;
};
}  // namespace DX12_Library
