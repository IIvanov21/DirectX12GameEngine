#pragma once

#include "Buffer.h"

#include "ByteAddressBuffer.h"

namespace DX12_Library
{

class Device;

/*
 * A structured buffer is a buffer that contains elements of equal sizes. Use a structure with one or
 * more member types to define an element. It also ensure the data is aligned properly inside the buffer.
 */
class StructuredBuffer : public Buffer
{

public:
    /**
     * Get the number of elements contained in this buffer.
     */
    virtual size_t GetNumElements() const
    {
        return m_NumElements;
    }

    /**
     * Get the size in bytes of each element in this buffer.
     */
    virtual size_t GetElementSize() const
    {
        return m_ElementSize;
    }

    std::shared_ptr<ByteAddressBuffer> GetCounterBuffer() const
    {
        return m_CounterBuffer;
    }

protected:
    StructuredBuffer( Device& device, size_t numElements,
                      size_t elementSize );
    StructuredBuffer( Device& device, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
                      size_t numElements, size_t elementSize );

    virtual ~StructuredBuffer() = default;

private:
    size_t m_NumElements;
    size_t m_ElementSize;

    // A buffer to store the internal counter for the structured buffer.
    std::shared_ptr<ByteAddressBuffer> m_CounterBuffer;
};
}  // namespace DX12_Library