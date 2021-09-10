#pragma once

#include <d3d12.h>  // For ID3D12CommandQueue, ID3D12Device2, and ID3D12Fence
#include <wrl.h>    // For Microsoft::WRL::ComPtr

#include <atomic>              // For std::atomic_bool
#include <condition_variable>  // For std::condition_variable.
#include <cstdint>             // For uint64_t

#include "ThreadSafeQueue.h"
/*
 * The command queue keeps an order of what command lists get executed and when they
 * get executed based on the current work load. The main goal is allow the bundelling of
 * commands and delay their execution while waiting for an event to happen.
 */
namespace DX12_Library
{

class CommandList;
class Device;

class CommandQueue
{
public:
    // Get an available command list from the command queue.
    // The command allocator is used to reserve memory for recording the GPU command. The command allocator cannot be
    // reused until all the GPU commands stored are executed on the GPU. One command allocator needed per render frame
    // but there is no need to create command allocator as the command list returned from this method will be in a state
    // that can immediatly be used to issue commands
    std::shared_ptr<CommandList> GetCommandList();

    // Execute a command list.
    // Returns the fence value to wait for for this command list.
    // Synchronization objects
    // Fence is an object used to synchonize commands issued to the Command Queue
    // It is recommended to create one fence object for each command queue to avoid problems with synchronization
    // Frame Fence Values are used to keep track of fence values that were used to single the command queue
    // IMPORTANT:If fence object doesnt reach a fence value specified for the frame the CPU thread will stall until
    // the fence value is reached which could cause drop in performance.
    uint64_t ExecuteCommandList( std::shared_ptr<CommandList> commandList );
    uint64_t ExecuteCommandLists( const std::vector<std::shared_ptr<CommandList>>& commandLists );

    uint64_t Signal();
    bool     IsFenceComplete( uint64_t fenceValue );
    void     WaitForFenceValue( uint64_t fenceValue );
    void     Flush();

    // Wait for another command queue to finish.
    void Wait( const CommandQueue& other );

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

protected:
    friend class std::default_delete<CommandQueue>;

    // Only the device can create command queues.
    CommandQueue( Device& device, D3D12_COMMAND_LIST_TYPE type );
    virtual ~CommandQueue();

private:
    // Free any command lists that are finished processing on the command queue.
    void ProccessInFlightCommandLists();

    // Keep track of command allocators that are "in-flight"
    // The first member is the fence value to wait for, the second is the
    // a shared pointer to the "in-flight" command list.
    using CommandListEntry = std::tuple<uint64_t, std::shared_ptr<CommandList>>;

    Device&                                    m_Device;
    D3D12_COMMAND_LIST_TYPE                    m_CommandListType;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
    Microsoft::WRL::ComPtr<ID3D12Fence>        m_d3d12Fence;
    std::atomic_uint64_t                       m_FenceValue;

    ThreadSafeQueue<CommandListEntry>             m_InFlightCommandLists;
    ThreadSafeQueue<std::shared_ptr<CommandList>> m_AvailableCommandLists;

    // A thread to process in-flight command lists.
    std::thread             m_ProcessInFlightCommandListsThread;
    std::atomic_bool        m_bProcessInFlightCommandLists;
    std::mutex              m_ProcessInFlightCommandListsThreadMutex;
    std::condition_variable m_ProcessInFlightCommandListsThreadCV;
};
}  // namespace DX12_Library