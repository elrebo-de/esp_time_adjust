/*
 * time_adjust.hpp
 *
 *      Author: christophoberle
 *
 * this work is licenced under the Apache-2.0 licence
 */

#ifndef TIME_ADJUST_HPP_
#define TIME_ADJUST_HPP_

#include <string>
#include <time.h>
#include <sys/time.h>

/* class TimeAdjust
   Class to synchronize time with a provided UTC date and time

   The original code is inspired from the GitHub repository https://github.com/sachin42/idfcomponents.git
   from file HTTPClient/main.cpp.

   Source of Singleton class structure - https://stackoverflow.com/a/1008289
   Posted by Loki Astari, modified by community. See post 'Timeline' for change history
   Retrieved 2026-02-01, License - CC BY-SA 4.0
*/

class TimeAdjust
{
    public:
        static TimeAdjust& getInstance();
        void setTimezone(std::string timezone);
        void setSyncIntervalMs(uint32_t syncIntervalMs);

        uint32_t getSyncIntervalMs();
        bool isSynchronized();

        std::string adjustTime(std::string date, std::string time);
        void obtainTime(void);
        void printCalendar();

    private:
        TimeAdjust(); // Constructor
        time_t mySafeTimegm(struct tm*);

        std::string tag = "TimeAdjust";
        uint32_t syncIntervalMs = 300000;
        bool timeSynchronized = false;

        // Global Mutex for Timezone switch (used in mySafeTimegm)
        StaticSemaphore_t mutex_buffer;
        SemaphoreHandle_t tz_mutex;

    public:
        TimeAdjust(TimeAdjust const&) = delete;
        void operator=(TimeAdjust const&) = delete;
};

#endif /* TIME_ADJUST_HPP_ */
