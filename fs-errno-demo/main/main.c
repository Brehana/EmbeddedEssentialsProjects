#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>
#include "esp_vfs_littlefs.h"
#include "esp_log.h"

static const char *TAG = "littlefs_demo";

void app_main(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount or format filesystem: %s", esp_err_to_name(ret));
        return;
    }

    struct statvfs s;
    if (statvfs(conf.base_path, &s) == 0) {
        size_t total = (size_t)s.f_frsize * s.f_blocks;
        size_t free = (size_t)s.f_frsize * s.f_bfree;
        ESP_LOGI(TAG, "LittleFS mounted at '%s' (total: %u, free: %u)", conf.base_path, (unsigned)total, (unsigned)free);
    }

    FILE *f = fopen("/littlefs/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
    } else {
        fprintf(f, "Hello LittleFS!\n");
        fclose(f);
        ESP_LOGI(TAG, "Wrote hello.txt");

        f = fopen("/littlefs/hello.txt", "r");
        if (f) {
            char buf[64];
            if (fgets(buf, sizeof(buf), f)) {
                ESP_LOGI(TAG, "Read: %s", buf);
            }
            fclose(f);
        }
    }

    // Keep the filesystem mounted for the application lifetime. To unmount:
    // esp_vfs_littlefs_unregister(conf.partition_label);
}
