#pragma once

namespace jam
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

    void Submit(const std::function<void()>& _command);
    void Execute();

private:
    std::mutex                    m_mutex;             // mutex for thread safety
    Vector<std::function<void()>> m_commands;          // queue of commands
    Vector<std::function<void()>> m_pendingCommands;   // pending commands for execution
};

}   // namespace jam

// all arguments will be captured by value (copy)
