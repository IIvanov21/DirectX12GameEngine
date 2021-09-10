#pragma once

#include "Resource.h"

namespace DX12_Library
{

//class Device;
/*
 * The main buffer class structure
 * which all other buffers derive from.
 */
class Buffer : public Resource
{
public:
protected:
    Buffer( Device& device, const D3D12_RESOURCE_DESC& resDesc );
    Buffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource );
};
}  // namespace DX12_Library