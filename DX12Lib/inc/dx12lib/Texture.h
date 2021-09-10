#pragma once

#include "DescriptorAllocation.h"
#include "Resource.h"

#include "d3dx12.h"

#include <mutex>
#include <unordered_map>

namespace DX12_Library
{

class Device;
/*
 * The texture component allows the developer to import custom textures
 * or textures attached to a model.
 */
class Texture : public Resource
{
public:
    /**
     * Resize the texture.
     */
    void Resize( uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1 );

    /**
     * Get the RTV for the texture.
     */
    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const;

    /**
     * Get the DSV for the texture.
     */
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;

    /**
     * Get the default SRV for the texture.
     */
    D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView() const;

    /**
     * Get the UAV for the texture at a specific mip level.
     * Note: Only only supported for 1D and 2D textures.
     */
    D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView( uint32_t mip ) const;

    bool CheckSRVSupport() const
    {
        return CheckFormatSupport( D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE );
    }

    bool CheckRTVSupport() const
    {
        return CheckFormatSupport( D3D12_FORMAT_SUPPORT1_RENDER_TARGET );
    }

    bool CheckUAVSupport() const
    {
        return CheckFormatSupport( D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW ) &&
               CheckFormatSupport( D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD ) &&
               CheckFormatSupport( D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE );
    }

    bool CheckDSVSupport() const
    {
        return CheckFormatSupport( D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL );
    }

    /**
     * Check to see if the image format has an alpha channel.
     */
    bool HasAlpha() const;

    /**
     * Check the number of bits per pixel.
     */
    size_t BitsPerPixel() const;

    static bool IsUAVCompatibleFormat( DXGI_FORMAT format );
    static bool IsSRGBFormat( DXGI_FORMAT format );
    static bool IsBGRFormat( DXGI_FORMAT format );
    static bool IsDepthFormat( DXGI_FORMAT format );

    // Return a typeless format from the given format.
    static DXGI_FORMAT GetTypelessFormat( DXGI_FORMAT format );
    // Return an sRGB format in the same format family.
    static DXGI_FORMAT GetSRGBFormat( DXGI_FORMAT format );
    static DXGI_FORMAT GetUAVCompatableFormat( DXGI_FORMAT format );

protected:
    Texture( Device& device, const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr );
    Texture( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
             const D3D12_CLEAR_VALUE* clearValue = nullptr );
    virtual ~Texture();

    /**
     * Create SRV and UAVs for the resource.
     */
    void CreateViews();

private:
    DescriptorAllocation m_RenderTargetView;
    DescriptorAllocation m_DepthStencilView;
    DescriptorAllocation m_ShaderResourceView;
    DescriptorAllocation m_UnorderedAccessView;
};
}  // namespace DX12_Library