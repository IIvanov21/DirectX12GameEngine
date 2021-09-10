#pragma once

#include <d3d12.h>       // For D3D12_PIPELINE_STATE_STREAM_DESC, and ID3D12PipelineState
#include <wrl/client.h>  // For Microsoft::WRL::ComPtr

namespace DX12_Library
{

class Device;
/*
 * This class create an object which allows the developer to change the
 * pipeline state. A good example is switching between HDR to SDR, this transition
 * requires the application to switch between an SDR and HDR pipeline which is done using
 * the PipelineStateObject.
 */
class PipelineStateObject
{
public:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> GetD3D12PipelineState() const
    {
        return m_d3d12PipelineState;
    }

protected:
    PipelineStateObject( Device& device, const D3D12_PIPELINE_STATE_STREAM_DESC& desc );
    virtual ~PipelineStateObject() = default;

private:
    Device&                                     m_Device;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_d3d12PipelineState;
};
}  // namespace DX12_Library
