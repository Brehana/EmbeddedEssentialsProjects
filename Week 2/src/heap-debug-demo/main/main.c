#include <stdio.h>
#include <stdlib.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *TAG = "HEAP";

static void print_heap_info(const char *label)
{
        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t min_free_heap = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);

    ESP_LOGI(TAG, "%s", label);
    ESP_LOGI(TAG, "  Free heap: %u bytes", free_heap);
    ESP_LOGI(TAG, "  Minimum ever free heap: %u bytes", min_free_heap);
}

void app_main(void)
{
    print_heap_info("Heap at startup");

    ESP_LOGI(TAG, "Allocating 1024 bytes (leaked)...");
    void *leak = malloc(1024);

    if (leak == NULL) {
        ESP_LOGE(TAG, "malloc failed!");
        return;
    }

    print_heap_info("Heap after leaking memory");

    // Intentionally NOT freeing 'leak'
}
