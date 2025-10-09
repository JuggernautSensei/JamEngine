#include "pch.h"

#include "MathUtilities.h"
#include <Random>
using namespace jam;

namespace jam::details
{
std::mt19937 g_randomGenerator = std::mt19937 {};
UInt32       g_randomSeed      = 0;
}