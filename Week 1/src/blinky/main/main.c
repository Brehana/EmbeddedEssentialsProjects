#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

// Define the GPIO pin for the LED
#define GPIO_NUM_2 2

void app_main(void)
{
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    int isOn = 1;

    while (true) {
        isOn = !isOn;
        gpio_set_level(GPIO_NUM_2, isOn);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

