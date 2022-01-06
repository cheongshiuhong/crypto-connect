#ifndef UTILS_DATETIME_H
#define UTILS_DATETIME_H

#include <cstdint>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <string>
#include <stdio.h>
#include <time.h>
#include <date/date.h>

namespace Utils::Constants
{
    /* Constants for finer units of time in a minute */
    static uint64_t const c_sInMinute = 60;
    static uint64_t const c_msInMinute = 60000;
    static uint64_t const c_usInMinute = 60000000;
    static uint64_t const c_nsInMinute = 60000000000;
}

namespace Utils::Datetime
{

    template <class T>
    inline uint64_t isostringToEpoch(std::string const &&timeString)
    {
        std::istringstream ss(timeString);
        date::sys_time<T> timePoint;
        ss >> date::parse("%FT%T", timePoint);

        return timePoint.time_since_epoch().count();
    }

    template <class T>
    inline uint64_t isostringToEpoch(std::string const &timeString)
    {
        std::istringstream ss(timeString);
        date::sys_time<T> timePoint;
        ss >> date::parse("%FT%T", timePoint);

        return timePoint.time_since_epoch().count();
    }

    inline std::string epochToIsostring(uint64_t const &epochTime)
    {
        char buffer[80];
        std::time_t rawtime(epochTime);
        struct tm *timeinfo = std::gmtime(&rawtime);
        std::strftime(buffer, 80, "%Y-%m-%dT%H:%M:%S", timeinfo);

        return buffer;
    }

    template <class T>
    inline uint64_t epochNow()
    {
        return std::chrono::duration_cast<T>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }
}

#endif
