# Time Synchronisation Component (with Manual Adjustment)

A simple Time Synchronization (with manual adjustment) as an ESP-IDF component. It runs on any ESP32 processor and is built using the ESP-IDF build system in version 5.5+.

The component is implemented as C++ class `TimeAdjust`.

This component can be used to synchronize system time with an external time provider like a GNSS receiver.

## Connecting the Component and Usage

You need to include ```time_adjust.hpp``` and initialize the class `TimeAdjust`.

In addition you can provide a function ```timeTask``` for a task which checks for synchronization and then reports the time periodically. To enable this, you have to create the timeTask.

To synchronize the time the method `adjustTime` must be called with the current UTC date and time.

```
/*
 * Example program to use time_adjust functionality with elrebo-de/esp_time_adjust
 */

#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "time_adjust.hpp"

static const char *tag = "time adjust test";

static void timeTask(void *pc){
    TimeAdjust* timeAdjust = &TimeAdjust::getInstance();

    // wait until time is synchronized
    while(!timeAdjust->isSynchronized()) {
        timeAdjust->obtainTime();
    };

    // Print calendar every sync interval
    while (1)
    {
        timeAdjust->printCalendar();
        vTaskDelay(pdMS_TO_TICKS(timeAdjust->getSyncIntervalMs()));
    }
}

extern "C" void app_main(void)
{
    // short delay to reconnect logging
    vTaskDelay(pdMS_TO_TICKS(500)); // delay 0.5 seconds

    ESP_LOGI(tag, "Example Program");

    /* Initialize TimeAdjust class */
    ESP_LOGI(tag, "TimeAdjust");
    TimeAdjust* timeAdjust = &timeAdjust->getInstance();
    // set timezone for Germany (CET/CEST incl. rules for switching)
    // "CET-1CEST,M3.5.0,M10.5.0" means:
    // - Normal time is CET (UTC+1)
    // - Daylight saving time is CEST (UTC+2)
    // - switch to CEST: March (3), last sunday (.5.0)
    // - switch to CET: October (10), last sunday (.5.0)
    timeAdjust->setTimezone(std::string("CET-1CEST,M3.5.0,M10.5.0"));
    timeAdjust->setSyncIntervalMs(30000); // syncIntervalMs set to 30 sec, default for normal operation is 5 mins

    ESP_LOGI(tag, "Create TimeTask");
    xTaskCreate(timeTask, "timeTask", 4096, NULL, 5, NULL);

    vTaskDelay(pdMS_TO_TICKS(60000)); // delay 60 seconds
    ESP_LOGI(tag, "AdjustTime 13:01");
    std::string currentTime = timeAdjust->adjustTime(std::string("17.10.26"), std::string("13:01:00"));

    vTaskDelay(pdMS_TO_TICKS(20000)); // delay 20 seconds
    ESP_LOGI(tag, "AdjustTime 15:01");
    currentTime = timeAdjust->adjustTime(std::string("17.10.26"), std::string("15:01:00"));

    vTaskDelay(pdMS_TO_TICKS(20000)); // delay 20 seconds
    ESP_LOGI(tag, "SetTimezone UTC");
    timeAdjust->setTimezone(std::string("UTC"));

    vTaskDelay(pdMS_TO_TICKS(20000)); // delay 20 seconds
}
```

## API
The API of the component is located in the include directory ```include/time_adjust.hpp``` and defines the
C++ class ```TimeAdjust```. 

```TimeAdjust``` is a Singleton class.

With this call you get a pointer to the Singleton instance:

```C++
    TimeAdjust* timeAdjust = &timeAdjust->getInstance();
```
To initialize the class, you use these methods:
* `setTimezone()` to set the timezone. The default is the current value of the environment variable TZ.
* `setSyncIntervalMs()` to set the synchronization interval. The default is 5 minutes. You can use this value through method `getSyncIntervalMs()` in your own code which calls method `adjustTime()` to adjust the system time.
* Method `obtainTime` can be used to wait for synchronization and set the privat flag `timeSynchronized`. This flag can be retrieved by method `isSynchronized()`.
In addition there is a method `printCalendar()` to report the current system time.

### External Effects

* Usage of method `adjustTime()` changes the system time.
* Usage of method `setTimezone()` changes the environment variable `TZ`.

```C++
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
```


# License
This component is provided under the Apache-2.0 license.
