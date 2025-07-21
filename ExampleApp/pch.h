#pragma once

#include "../JamEngine/JamEngine.h"
using namespace jam;

#ifdef _DEBUG
#pragma comment(lib, "../JamEngine/bin/Debug/JamEngine")
#else
#pragma comment(lib, "../JamEngine/bin/Release/JamEngine")
#endif
