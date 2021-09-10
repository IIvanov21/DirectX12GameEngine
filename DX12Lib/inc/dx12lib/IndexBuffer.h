#pragma once

#include "Buffer.h"

namespace DX12_Library
{
/*
 * This buffer contains an array of pointer that are used in the vertex buffer.
 * It allows to reorder the vertex data and reuse existing data for the vertices.
 */
class IndexBuffer : public Buffer
{
public:
    /**
     * Get the index buffer view for biding to the Input Assembler stage.
     */
    D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
    {
        return m_IndexBufferView;
    }

    size_t GetNumIndicies() const
    {
        return m_NumIndicies;
    }

    DXGI_FORMAT GetIndexFormat() const
    {
        return m_IndexFormat;
    }

protected:
    IndexBuffer( Device& device, size_t numIndicies, DXGI_FORMAT indexFormat );
    IndexBuffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource, size_t numIndicies,
                 DXGI_FORMAT indexFormat );
    virtual ~IndexBuffer() = default;

    void CreateIndexBufferView();

private:
    size_t      m_NumIndicies;
    DXGI_FORMAT m_IndexFormat;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
};
}  // namespace DX12_Library