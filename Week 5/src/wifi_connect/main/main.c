#include <stdio.h>
#include <string.h>

#include "lcd1602_i2c.h"

#include "ping/ping_sock.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_netif_ip_addr.h"

static const char *TAG = "WIFI_STEP3";

#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PWD"

static void ping_success(esp_ping_handle_t hdl, void *args);
static void ping_timeout(esp_ping_handle_t hdl, void *args);
static void ping_end(esp_ping_handle_t hdl, void *args);
static void start_ping(void);
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data);

void app_main(void)
{
    // Initialize LCD

    // Step 1: Initialize I2C bus
    i2c_init();

    // Step 2: Optional - Scan for devices
    scan_i2c();

    // Step 3: Initialize LCD display
    lcd_init();

    // Step 4: Use LCD functions
    lcd_ser_curser(0, 0);
    lcd_send_buf("Welcome!", 8);
    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait 5 seconds

    // Step 5: Initialize WiFi and connect to network
    ESP_LOGI(TAG, "Starting WiFi connection step 3...");

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register WiFi event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL));

    // Register IP event handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization complete.");
}

// ping_success: Called when a ping reply is received. Logs the RTT,
// clears and updates the LCD with the reply and elapsed time.
static void ping_success(esp_ping_handle_t hdl, void *args)
{
    uint32_t elapsed_time;
    // Read the round-trip time profile from the ping session
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP,
                         &elapsed_time, sizeof(elapsed_time));

    // Log the elapsed time to the console
    ESP_LOGI("PING", "Reply received, time=%lu ms", elapsed_time);

    // Update LCD: clear display, then show a reply message
    lcd_send_cmd(0x01); // Clear display
    usleep(2000);       // Short delay to allow LCD clear to complete
    lcd_ser_curser(0, 0);
    char *msg = "Reply Received!";
    lcd_send_buf(msg, strlen(msg));

    // Show elapsed time on the second line of the LCD
    lcd_ser_curser(1, 0);
    char time_buffer[20];
    snprintf(time_buffer, sizeof(time_buffer), "Time: %lu ms", elapsed_time);
    lcd_send_buf(time_buffer, strlen(time_buffer));
}

// ping_timeout: Called when a ping request times out. Logs the event
// and displays a timeout message on the LCD.
static void ping_timeout(esp_ping_handle_t hdl, void *args)
{
    // Log a warning that the ping request timed out
    ESP_LOGW("PING", "Request timeout");

    // Update LCD to indicate timeout: clear then write message
    lcd_send_cmd(0x01); // Clear display
    usleep(2000);       // Short delay to allow LCD clear to complete
    lcd_ser_curser(0, 0);
    char *msg = "Request Timeout!";
    lcd_send_buf(msg, strlen(msg));
}

// ping_end: Called when the ping session finishes. Retrieves summary
// statistics, logs them, updates the LCD, and deletes the ping session.
static void ping_end(esp_ping_handle_t hdl, void *args)
{
    uint32_t transmitted, received;

    // Retrieve the number of requests transmitted and replies received
    esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST,
                         &transmitted, sizeof(transmitted));
    esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY,
                         &received, sizeof(received));

    // Log a summary of the ping results
    ESP_LOGI("PING", "Ping complete. Sent=%lu Received=%lu",
             transmitted, received);

    // Update LCD: clear and show a completion message plus counts
    lcd_send_cmd(0x01); // Clear display
    usleep(2000);       // Short delay to allow LCD clear to complete

    lcd_ser_curser(0, 0);
    char *msg = "Ping Complete!";
    lcd_send_buf(msg, strlen(msg));

    lcd_ser_curser(1, 0);
    char report_buffer[30];
    snprintf(report_buffer, sizeof(report_buffer), "Sent=%lu Recv=%lu", transmitted, received);
    lcd_send_buf(report_buffer, strlen(report_buffer));

    // Clean up the ping session resources
    esp_ping_delete_session(hdl);
}

// start_ping: Resolve a hostname to an IP, configure a ping session,
// and start asynchronous pinging with the defined callbacks.
static void start_ping(void)
{
    struct addrinfo hint = {
        .ai_family = AF_INET,
    };
    struct addrinfo *res;

    // Resolve google.com to an IPv4 address
    int err = getaddrinfo("google.com", NULL, &hint, &res);
    if (err != 0 || res == NULL)
    {
        ESP_LOGE("PING", "DNS lookup failed");
        return;
    }

    // Extract the IPv4 socket address from the resolver result
    struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;

    // Convert to lwIP ip_addr_t type
    ip_addr_t target_addr;
    ipaddr_aton(inet_ntoa(addr->sin_addr), &target_addr);

    freeaddrinfo(res);

    // Prepare ping configuration: target, count and interval
    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();
    config.target_addr = target_addr;
    config.count = 5;
    config.interval_ms = 1000;

    // Register callback handlers for success, timeout and end
    esp_ping_callbacks_t callbacks = {
        .on_ping_success = ping_success,
        .on_ping_timeout = ping_timeout,
        .on_ping_end = ping_end,
    };

    // Create and start the ping session
    esp_ping_handle_t ping;
    esp_ping_new_session(&config, &callbacks, &ping);
    esp_ping_start(ping);
}

// wifi_event_handler: Handle WiFi and IP events. Connects/retries on
// WiFi events and starts pinging once an IP address is obtained.
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    lcd_ser_curser(1, 0);
    char *msg = "Connecting...";
    lcd_send_buf(msg, strlen(msg));

    // Handle WiFi-related events
    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_START)
        {
            // Station started: attempt to connect to configured AP
            ESP_LOGI(TAG, "WiFi started, attempting to connect...");

            esp_wifi_connect();
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            // Station disconnected: log reason and retry connect
            wifi_event_sta_disconnected_t *event =
                (wifi_event_sta_disconnected_t *)event_data;

            ESP_LOGI(TAG, "Disconnected. Reason code: %d", event->reason);

            ESP_LOGI(TAG, "Retrying connection...");

            esp_wifi_connect();
        }
    }
    // Handle IP address acquisition event
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        // Log the assigned IP address
        ESP_LOGI(TAG, "Got IP Address: " IPSTR, IP2STR(&event->ip_info.ip));

        lcd_send_cmd(0x01); // Clear display
        usleep(2000);       // Short delay to allow LCD clear to complete
        lcd_ser_curser(0, 0);
        char *success_msg= "Connected!";
        lcd_send_buf(success_msg, strlen(success_msg));
        vTaskDelay(pdMS_TO_TICKS(2000)); // Wait 2 seconds

        // Start the ping sequence now that we have network connectivity
        start_ping();
    }
}