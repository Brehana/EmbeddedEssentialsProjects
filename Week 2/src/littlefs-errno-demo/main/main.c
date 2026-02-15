#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "esp_littlefs.h"

static const char *TAG = "FS_ERRNO";

void app_main(void)
{
    ESP_LOGI(TAG, "Mounting LittleFS...");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount LittleFS: %s",
                 esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "LittleFS mounted");

    // Trigger a POSIX error: file does not exist
    FILE *f = fopen("/storage/does_not_exist.txt", "r");

    if (f == NULL) {
        ESP_LOGE(TAG,
                 "fopen failed: errno=%d (%s)",
                 errno,
                 strerror(errno));
    }

    // Optional: trigger a close error
    int ret_close = fclose(NULL);

    if (ret_close != 0) {
        ESP_LOGE(TAG,
                 "fclose failed: errno=%d (%s)",
                 errno,
                 strerror(errno));
    }
}