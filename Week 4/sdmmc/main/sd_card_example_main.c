#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

static const char *TAG = "MSC_SD";

// Correct pins for Freenove Ultimate ESP32-S3
#define PIN_NUM_CMD   38
#define PIN_NUM_CLK   39
#define PIN_NUM_D0    40

static sdmmc_card_t *card = NULL;

void app_main(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing SDMMC host...");

    // Configure SDMMC host
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    // Manual pin configuration for your board
    sdmmc_slot_config_t slot_config = {
        .clk = PIN_NUM_CLK,
        .cmd = PIN_NUM_CMD,
        .d0  = PIN_NUM_D0,
        .d1  = -1,
        .d2  = -1,
        .d3  = -1,
        .width = 1,
        .flags = SDMMC_SLOT_FLAG_INTERNAL_PULLUP
    };

    // Initialize host
    ret = sdmmc_host_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sdmmc_host_init failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize slot
    ret = sdmmc_host_init_slot(host.slot, &slot_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sdmmc_host_init_slot failed: %s", esp_err_to_name(ret));
        return;
    }

    // Allocate memory for card structure
    card = malloc(sizeof(sdmmc_card_t));
    if (!card) {
        ESP_LOGE(TAG, "Failed to allocate memory for sdmmc_card_t");
        return;
    }

    // Initialize card
    ret = sdmmc_card_init(&host, card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "sdmmc_card_init failed: %s", esp_err_to_name(ret));
        free(card);
        return;
    }

    ESP_LOGI(TAG, "SD card initialized successfully!");
    ESP_LOGI(TAG, "Sector size: %lu bytes", card->csd.sector_size);

    uint64_t capacity_mb =
        ((uint64_t)card->csd.capacity * card->csd.sector_size) / (1024 * 1024);

    ESP_LOGI(TAG, "Capacity: %llu MB", capacity_mb);

    // Keep system alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}