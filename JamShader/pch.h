#pragma once
#include "../JamEngine/JamEngine.h"
using namespace Jam;

#ifdef JAM_DEBUG
#    pragma comment(lib, "../JamEngine/bin/Debug/JamEngine.lib")
#else
#    pragma comment(lib, "../JamEngine/bin/Release/JamEngine.lib")
#endif

#include "Resources.h"

