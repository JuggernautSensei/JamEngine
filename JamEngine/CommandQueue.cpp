#include "pch.h"

#include "CommandQueue.h"


namespace jam
{

CommandQueue::CommandQueue() = default;
CommandQueue::~CommandQueue() = default;

void CommandQueue::Submit(const std::function<void()>& _command)
{
    JAM_ASSERT(_command, "Command cannot be null");

    std::lock_guard<std::mutex> lock(m_mutex);
    m_commands.push_back(_command);
}

void CommandQueue::Execute()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingCommands.swap(m_commands);
    }

    for (const std::function<void()>& command: m_pendingCommands)
    {
        JAM_ASSERT(command, "Command cannot be null");
        command();
    }

    m_pendingCommands.clear();
}

}