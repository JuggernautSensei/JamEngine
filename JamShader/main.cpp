#include "pch.h"
#include "MainScene.h"

class ShaderApp : public Application
{
public:
    explicit ShaderApp(
        const StringView _appName,
        const bool       _bEnableEditor)
        : Application(_appName, _bEnableEditor)
    {
    }

    void Init(Int32, char**) override
    {
        SetVsync(true);
        JAM_WIN.AdjustClientRect(100, 100, 1600, 900);

        const Ref<MainScene> pScene = MakeRef<MainScene>();
        JAM_SCENE_MGR.LoadScene(pScene);
    }
};

JAM_MAIN(ShaderApp, "JamShader", false);