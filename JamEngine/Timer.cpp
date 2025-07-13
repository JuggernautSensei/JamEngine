#include "Base.h"

#include "Timer.h"

namespace jam
{

TimeStamp TimeStamp::Create()
{
    using namespace std::chrono;
    auto        now = system_clock::now();
    auto        ms  = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t   = system_clock::to_time_t(now);
    std::tm     tm;
    TimeStamp   stamp;

    if (localtime_s(&tm, &t) == 0)
    {
        stamp.year        = 1900 + tm.tm_year;
        stamp.month       = 1 + tm.tm_mon;
        stamp.day         = tm.tm_mday;
        stamp.hour        = tm.tm_hour;
        stamp.minute      = tm.tm_min;
        stamp.second      = tm.tm_sec;
        stamp.millisecond = static_cast<int>(ms.count());
    }

    return stamp;
}

}   // namespace jam