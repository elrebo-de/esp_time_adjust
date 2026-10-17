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
