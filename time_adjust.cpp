/*
 * time_adjust.cpp
 *
 *      Author: christophoberle
 *
 * this work is licenced under the Apache 2.0 licence
 */

#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "esp_log.h"

#include "time_adjust.hpp"

#include <time.h>
#include <sys/time.h>

TimeAdjust::TimeAdjust()
{
    tz_mutex = NULL;
}

TimeAdjust& TimeAdjust::getInstance()
{
    static TimeAdjust instance; // Guaranteed to be destroyed. Instantiated on first use.
    return instance;
}

// Function to set syncIntervalMs
void TimeAdjust::setSyncIntervalMs(uint32_t syncIntervalMs)
 {
     ESP_LOGI(tag.c_str(), "Set SyncIntervalMs: %lu", syncIntervalMs);
     this->syncIntervalMs = syncIntervalMs;
 }

// Function to get syncIntervalMs
uint32_t TimeAdjust::getSyncIntervalMs()
{
    return this->syncIntervalMs;
}

// Function to test whether time is synchronized
bool TimeAdjust::isSynchronized()
{
    return this->timeSynchronized;
}

// Function to adjust time to a given UTC date and time
std::string TimeAdjust::adjustTime(std::string date, std::string time)
{
    ESP_LOGI(tag.c_str(), "Adjust Time: %s %s UTC", date.c_str(), time.c_str());

    if(date.length() < 8 || time.length() < 8) {
        return std::string("");
    }

    // get UTC time from given date and time
    struct tm utc_tm = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 0,
        .tm_mday = 0,
        .tm_mon = 0,
        .tm_year = 0,
        .tm_wday = 0,
        .tm_yday = 0,
        .tm_isdst = 0,
    };
    utc_tm.tm_year = 2000 - 1900 + std::stoi(date.substr(6,2)); // years since 1900
    utc_tm.tm_mon  = std::stoi(date.substr(3,2)) - 1;           // Month (0 = Januar, 7 = August)
    utc_tm.tm_mday = std::stoi(date.substr(0,2));
    utc_tm.tm_hour = std::stoi(time.substr(0,2));
    utc_tm.tm_min  = std::stoi(time.substr(3,2));
    utc_tm.tm_sec  = std::stoi(time.substr(6,2));

    char utc_buf[64];
    strftime(utc_buf, sizeof(utc_buf), "%d.%m.%Y %H:%M:%S %Z", &utc_tm);

    // 3. transform UTC structure to a Unix timestamp
    // my_timegm_safe() uses UTC
    time_t utc_timestamp = mySafeTimegm(&utc_tm);

    // define Unix timestamp
    struct timeval tv;
    tv.tv_sec = utc_timestamp;  // Seconds since 1.1.1970
    tv.tv_usec = 0;             // Microseconds

    // 2. set system time directly
    settimeofday(&tv, NULL);

    // translate Timestamp to local time
    struct tm local_tm;
    if(NULL == localtime_r(&utc_timestamp, &local_tm)) {
        return std::string("");
    };

    // set internal state
    this->obtainTime();

    // format result
    char buf[64];
    strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S %Z", &local_tm);

    return std::string(buf);
}

// Function to obtain and print time
void TimeAdjust::obtainTime(void)
{
    ESP_LOGI(tag.c_str(), "Obtain Time:");

    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {
                           0, // tm_sec	int	seconds after the minute	0-60*
                           0, // tm_min	int	minutes after the hour	0-59
                           0, // tm_hour	int	hours since midnight	0-23
                           0, // tm_mday	int	day of the month	1-31
                           0, // tm_mon	int	months since January	0-11
                           0, // tm_year	int	years since 1900
                           0, // tm_wday	int	days since Sunday	0-6
                           0, // tm_yday	int	days since January 1	0-365
                           0  // tm_isdst	int	Daylight Saving Time flag
                         };
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2025 - 1900) && ++retry < retry_count)
    {
        ESP_LOGI(tag.c_str(), "  ObtainTime: Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (retry == retry_count)
    {
        ESP_LOGE(tag.c_str(), "  Obtain Time: Failed to obtain time.");
        return;
    }

    ESP_LOGI(tag.c_str(), "  Obtain Time: Time synchronized.");
    this->timeSynchronized = true;
}

// Function to set timezone
void TimeAdjust::setTimezone(std::string timezone)
{
    ESP_LOGI(tag.c_str(), "Set Timezone: %s", timezone.c_str());
    setenv("TZ", timezone.c_str(), 1); // Set the TZ environment variable
    tzset(); // Apply the timezone change
}

// Print current calendar information
void TimeAdjust::printCalendar()
{
    time_t now;
    struct tm timeinfo;
    char buffer[64];

    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(buffer, sizeof(buffer), "Date: %Y-%m-%d, Time: %H:%M:%S %Z", &timeinfo);
    ESP_LOGI(tag.c_str(), "Print Calendar:");
    ESP_LOGI(tag.c_str(), "  %s", buffer);
    ESP_LOGI(tag.c_str(), "  Weekday: %d (0=Sunday, 6=Saturday)", timeinfo.tm_wday);
    ESP_LOGI(tag.c_str(), "  Day of the year: %d", timeinfo.tm_yday + 1);
}

time_t TimeAdjust::mySafeTimegm(struct tm *tm)
{
    // initialize Mutex lazily at first call
    if (tz_mutex == NULL) {
        tz_mutex = xSemaphoreCreateMutexStatic(&mutex_buffer);
    }

    time_t time_val = -1;

    // lock Mutex (waits forever until Mutex is free
    if (xSemaphoreTake(tz_mutex, portMAX_DELAY) == pdTRUE) {
        char *tz = getenv("TZ");

        // switch to UTC temporarily
        unsetenv("TZ");
        tzset();

        // UTC has no DST
        tm->tm_isdst = 0;
        time_val = mktime(tm);

        // reset original timezone
        if (tz) {
            setenv("TZ", tz, 1);
        } else {
            unsetenv("TZ");
        }
        tzset();

        // unlock Mutex
        xSemaphoreGive(tz_mutex);
    }
    return time_val;
}

