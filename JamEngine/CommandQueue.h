#pragma once

namespace Jam
{

class CommandQueue
{
public:
    CommandQueue();
    ~CommandQueue();

    CommandQueue(const CommandQueue&)                = delete;
    CommandQueue& operator=(const CommandQueue&)     = delete;
    CommandQueue(CommandQueue&&) noexcept            = delete;
    CommandQueue& operator=(CommandQueue&&) noexcept = delete;

    void Submit(const Delegate<void()>& _command);
    void Execute();

private:
    std::mutex               m_mutex           = {}; // mutex for thread safety
    Vector<Delegate<void()>> m_commands        = {}; // queue of commands
    Vector<Delegate<void()>> m_pendingCommands = {}; // pending commands for execution
};

}   // namespace Jam

// all arguments will be captured by m_value (copy)
