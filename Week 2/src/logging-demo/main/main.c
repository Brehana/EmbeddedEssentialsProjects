#include <stdio.h>

#include "esp_log.h"
#include "esp_err.h"
#include "nvs_flash.h"

static const char *TAG = "LOG_DEMO";

esp_err_t open_nvs_without_init(void)
{
    nvs_handle_t handle;

    // This will FAIL because NVS has not been initialized
    return nvs_open("demo", NVS_READONLY, &handle);
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting real error logging demo");

    esp_err_t err = open_nvs_without_init();

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Error in function %s: %s",
            __func__,
            esp_err_to_name(err)
        );
    }

    ESP_LOGI(TAG, "Demo complete");
}