#pragma once

#include "DescriptorAllocation.h"

#include <cstdint>

namespace DX12_Library
{

class Device;
class PipelineStateObject;
class RootSignature;
/*
 * This componenet allows to convert a cubemap to a texture and allow the use
 * of that cubemap inside the shader.
 */
// Struct used in the PanoToCubemap_CS compute shader.
struct PanoToCubemapCB
{
    // Size of the cubemap face in pixels at the current mipmap level.
    uint32_t CubemapSize;
    // The first mip level to generate.
    uint32_t FirstMip;
    // The number of mips to generate.
    uint32_t NumMips;
};

// I don't use scoped enums to avoid the explicit cast that is required to
// treat these as root indices into the root signature.
namespace PanoToCubemapRS
{
enum
{
    PanoToCubemapCB,
    SrcTexture,
    DstMips,
    NumRootParameters
};
}

class PanoToCubemapPSO
{
public:
    PanoToCubemapPSO( Device& device );

    std::shared_ptr<RootSignature> GetRootSignature() const
    {
        return m_RootSignature;
    }

    std::shared_ptr<PipelineStateObject> GetPipelineState() const
    {
        return m_PipelineState;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const
    {
        return m_DefaultUAV.GetDescriptorHandle();
    }

private:
    std::shared_ptr<RootSignature>       m_RootSignature;
    std::shared_ptr<PipelineStateObject> m_PipelineState;
    // Default (no resource) UAV's to pad the unused UAV descriptors.
    // If generating less than 5 mip map levels, the unused mip maps
    // need to be padded with default UAVs (to keep the DX12 runtime happy).
    DescriptorAllocation m_DefaultUAV;
};
}  // namespace DX12_Library