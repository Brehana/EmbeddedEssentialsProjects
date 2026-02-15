#include <stdio.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SLEEP";

static void print_wakeup_reason(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:
        ESP_LOGI(TAG, "Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_UNDEFINED:
        ESP_LOGI(TAG, "Normal boot (not from sleep)");
        break;
    default:
        ESP_LOGI(TAG, "Wakeup cause: %d", cause);
        break;
    }
}

static void demo_light_sleep(void)
{
    ESP_LOGI(TAG, "Entering light sleep for 5 seconds...");
    esp_sleep_enable_timer_wakeup(5 * 1000000);
    esp_light_sleep_start();
    ESP_LOGI(TAG, "Woke up from light sleep");
}

static void demo_deep_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep for 5 seconds...");
    ESP_LOGI(TAG, "USB serial will disconnect now");

    esp_sleep_enable_timer_wakeup(5 * 1000000);

    vTaskDelay(pdMS_TO_TICKS(1000));

    esp_deep_sleep_start();
}

void app_main(void)
{
    print_wakeup_reason();

    ESP_LOGI(TAG, "Booting...");

    vTaskDelay(pdMS_TO_TICKS(2000));

    demo_light_sleep();

    vTaskDelay(pdMS_TO_TICKS(2000));

    demo_deep_sleep();
}
