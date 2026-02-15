idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    # Common component names for LittleFS: littlefs, esp_vfs_littlefs, vfs_littlefs
                    REQUIRES littlefs)
