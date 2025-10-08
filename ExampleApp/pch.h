#pragma once

#include "../JamEngine/JamEngine.h"
using namespace jam;

#ifdef JAM_DEBUG
#pragma comment(lib, "../JamEngine/bin/Debug/JamEngine")
#else
#pragma comment(lib, "../JamEngine/bin/Release/JamEngine")
#endif
