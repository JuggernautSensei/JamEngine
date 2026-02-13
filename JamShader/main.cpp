#include "../JamEngine/JamEngine.h"
using namespace Jam;

#ifdef JAM_DEBUG
#    pragma comment(lib, "../JamEngine/bin/Debug/JamEngine.lib")
#else
#    pragma comment(lib, "../JamEngine/bin/Release/JamEngine.lib")
#endif

JAM_MAIN();

class ShaderApp : public Application
{
public:
    explicit ShaderApp(
        const ApplicationCreateInfo& _info)
        : Application(_info)
    {
    }


};

class MainScene : public Scene
{
};