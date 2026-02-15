#include "esp_heap_caps.h"
#include "esp_log.h"

static const char *TAG = "MEMORY";

void app_main(void) {
    size_t dram = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t iram = heap_caps_get_free_size(MALLOC_CAP_32BIT)
                - heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    ESP_LOGI(TAG, "Free DRAM: %d bytes", dram);
    ESP_LOGI(TAG, "Free IRAM: %d bytes", iram);
    ESP_LOGI(TAG, "Largest free block: %d bytes", largest);
}

