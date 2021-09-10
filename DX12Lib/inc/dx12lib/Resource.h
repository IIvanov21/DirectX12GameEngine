#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <string>

namespace DX12_Library
{

class Device;
/*
 * A resource such as a render target is a memory buffer. The difference
 * between resources is how you operate with it and how the GPU sees it i.e.
 * be seen as Vertex Buffer, Index Buffer, PSO etc..
 */
class Resource
{
public:
    /**
     * Get the Device that was used to create this resource.
     */
    Device& GetDevice() const {
        return m_Device;
    }

    // Get access to the underlying D3D12 resource
    Microsoft::WRL::ComPtr<ID3D12Resource> GetD3D12Resource() const
    {
        return m_d3d12Resource;
    }

    D3D12_RESOURCE_DESC GetD3D12ResourceDesc() const
    {
        D3D12_RESOURCE_DESC resDesc = {};
        if ( m_d3d12Resource )
        {
            resDesc = m_d3d12Resource->GetDesc();
        }

        return resDesc;
    }

    /**
     * Set the name of the resource. Useful for debugging purposes.
     */
    void                SetName( const std::wstring& name );
    const std::wstring& GetName() const
    {
        return m_ResourceName;
    }

    /**
     * Check if the resource format supports a specific feature.
     */
    bool CheckFormatSupport( D3D12_FORMAT_SUPPORT1 formatSupport ) const;
    bool CheckFormatSupport( D3D12_FORMAT_SUPPORT2 formatSupport ) const;

protected:
//    friend class CommandList;

    // Resource creation should go through the device.
    Resource( Device& device, const D3D12_RESOURCE_DESC& resourceDesc,
                       const D3D12_CLEAR_VALUE* clearValue = nullptr );
    Resource( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
              const D3D12_CLEAR_VALUE* clearValue = nullptr );

    virtual ~Resource() = default;

    // The device that is used to create this resource.
    Device& m_Device;

    // The underlying D3D12 resource.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12Resource;
    D3D12_FEATURE_DATA_FORMAT_SUPPORT      m_FormatSupport;
    std::unique_ptr<D3D12_CLEAR_VALUE>     m_d3d12ClearValue;
    std::wstring                           m_ResourceName;

private:
    // Check the format support and populate the m_FormatSupport structure.
    void CheckFeatureSupport();
};
}  // namespace DX12_Library