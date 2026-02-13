#include "pch.h"

#include "CommandQueue.h"

namespace Jam
{

CommandQueue::CommandQueue()
{
}

CommandQueue::~CommandQueue()
{
}

void CommandQueue::Submit(
    const Delegate<void()>& _command)
{
    JAM_ASSERT(_command, "Command cannot be null");

    std::lock_guard<std::mutex> lock(m_mutex);
    m_commands.emplace_back(_command);
}

void CommandQueue::Execute()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingCommands.swap(m_commands);
    }

    for (const Delegate<void()>& command: m_pendingCommands)
    {
        JAM_ASSERT(command, "Command cannot be null");
        command.Call();
    }

    m_pendingCommands.clear();
}

}   // namespace Jam