
#include "lcd1602_i2c.h"
#include <stdio.h>
#include <time.h>

/* ========================= */
/* APP MAIN                  */
/* ========================= */
void app_main(void)
{
    // Step 1: Initialize I2C bus
    i2c_init();

    // Step 2: Optional - Scan for devices
    scan_i2c();

    // Step 3: Initialize LCD display
    lcd_init();

    // Step 4: Use LCD functions
    lcd_ser_curser(0, 0);
    lcd_send_buf("Welcome", 7);

    // Main application code here

    time_t current_time;
    // Obtain current time in seconds since the Unix epoch
    time(&current_time);

    // Convert to local time string format
    char *time_string = ctime(&current_time);

    if (time_string != NULL)
    {
        printf("Current time is: %s", time_string);
        lcd_ser_curser(0, 0);
        lcd_send_buf(time_string, 16);
        lcd_ser_curser(1, 0);
        char *substr = time_string + 16;
        lcd_send_buf(substr, 9);
    }
    else
    {
        printf("Failed to obtain current time.\n");
    }
}