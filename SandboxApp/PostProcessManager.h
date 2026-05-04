#pragma once

enum class ePostProcess : UInt32
{
    Blit,
    Mix,
    Convolution,
};

class PostProcessManager
{
    struct Param
    {
        Vec4  fparams;
        Vec4i iparams;
    };

    struct Kernel
    {
        Vec4 weights[4];
    };


public:
    PostProcessManager(const PostProcessManager&)                = delete;
    PostProcessManager& operator=(const PostProcessManager&)     = delete;
    PostProcessManager(const PostProcessManager&&) noexcept      = delete;
    PostProcessManager& operator=(PostProcessManager&&) noexcept = delete;

    [[nodiscard]] static PostProcessManager& GetInstance();
    static void                              CreateInstance();
    static void                              DestroyInstance();

    [[nodiscard]] ProgramHandle GetProgram(ePostProcess _type);
    void                        SubmitScreenTriangle() const;

private:
    PostProcessManager();
    ~PostProcessManager();

    Ref<Mesh>                              m_pScreenTriangle = nullptr;
    EnumArray<ePostProcess, ProgramHandle> m_programs        = {};
    EnumArray<ePostProcess, String>        m_programPaths    = {};

    inline static PostProcessManager* s_instance = nullptr;
};

[[nodiscard]] inline PostProcessManager& GetPostProcessManager()
{
    return PostProcessManager::GetInstance();
}