#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON_PIN GPIO_NUM_3

// =======================
// 7-segment configuration
// =======================

const gpio_num_t segmentPins[7] = {4, 5, 6, 7, 8, 9, 10};

// Digit patterns for 0-9 (common anode: 1 = segment ON)
const int digits[10][7] = {
    {0,1,1,1,1,1,1},  // 0
    {0,0,0,1,1,0,0},  // 1
    {1,0,1,1,0,1,1},  // 2
    {1,0,1,1,1,1,0},  // 3
    {1,1,0,1,1,0,0},  // 4
    {1,1,1,0,1,1,0},  // 5
    {1,1,1,0,1,1,1},  // 6
    {0,0,1,1,1,0,0},  // 7
    {1,1,1,1,1,1,1},  // 8
    {1,1,1,1,1,1,0}   // 9
};

// Letter patterns for HELLO (approximation on 7-seg)
const int letters[5][7] = {
    {1,1,0,1,1,0,1}, // H
    {1,1,1,0,0,1,1}, // E
    {0,1,0,0,0,1,1}, // L
    {0,1,0,0,0,1,1}, // L
    {0,1,1,1,1,1,1}  // O
};

// =======================
// FreeRTOS types
// =======================

typedef enum {
    BUTTON_PRESS,
    BUTTON_RELEASE
} button_event_type_t;

typedef struct {
    button_event_type_t type;
    TickType_t timestamp;
} button_event_t;

typedef enum {
    DISPLAY_CMD_NEXT_DIGIT,
    DISPLAY_CMD_HELLO
} display_cmd_t;

// =======================
// Globals
// =======================

static QueueHandle_t button_queue;
static QueueHandle_t display_queue;

// =======================
// Display helpers
// =======================

/**
 * init_segments
 *
 * Step-by-step:
 * 1) Prepare a `gpio_config_t` structure to configure multiple GPIOs
 *    as outputs. `pin_bit_mask` will be filled with bits for each
 *    segment pin.
 * 2) Loop over `segmentPins[]` and set the corresponding bits in
 *    `pin_bit_mask` by shifting `1ULL` left by the GPIO number.
 * 3) Call `gpio_config()` to apply the configuration and make the
 *    selected pins outputs.
 * 4) Initialize the physical display by driving all segment pins to
 *    the OFF state. For this project OFF is represented by logic HIGH
 *    (common-anode wiring).
 */
void init_segments(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 0
    };

    /* Build the bitmask for all segment GPIOs */
    for (int i = 0; i < 7; i++) {
        io_conf.pin_bit_mask |= (1ULL << segmentPins[i]);
    }

    /* Apply configuration: make these pins outputs */
    gpio_config(&io_conf);

    /* Drive every segment to OFF initially (HIGH for common-anode) */
    for (int i = 0; i < 7; i++) {
        gpio_set_level(segmentPins[i], 1);
    }
}

/**
 * display_digit
 *
 * Step-by-step:
 * 1) Accept `num` (0..9) as the digit to display.
 * 2) For each of the 7 segments, read the corresponding entry in the
 *    `digits` lookup table. The table stores truthy values for segments
 *  *    that should be ON for the given digit.
 * 3) Drive the GPIO for each segment to the correct level. Because the
 *    hardware is wired as common-anode, we write logic LOW (0) to turn a
 *    segment ON, and logic HIGH (1) to turn it OFF. The ternary operator
 *    performs this inversion.
 */
void display_digit(int num)
{
    for (int i = 0; i < 7; i++) {
        /* If digits[num][i] is true -> turn segment ON (0), else OFF (1) */
        gpio_set_level(segmentPins[i], digits[num][i] ? 0 : 1);
    }
}

/**
 * display_letter
 *
 * Step-by-step:
 * 1) Accept `index` into the `letters` table (e.g. 0..4 for "HELLO").
 * 2) For each segment, consult `letters[index][i]` which indicates whether
 *    that segment should be lit for the chosen character.
 * 3) Drive the corresponding GPIO low (0) to light the segment or high
 *    (1) to turn it off — matching the common-anode wiring used here.
 */
void display_letter(int index)
{
    for (int i = 0; i < 7; i++) {
        gpio_set_level(segmentPins[i], letters[index][i] ? 0 : 1);
    }
}

// =======================
// Button ISR
// =======================

/**
 * button_isr
 *
 * Step-by-step (interrupt context):
 * 1) Create a `button_event_t` and timestamp it using the ISR-safe tick
 *    function `xTaskGetTickCountFromISR()` so the consumer can measure
 *    press durations.
 * 2) Sample the GPIO level for `BUTTON_PIN`. A LOW level indicates a
 *    press (because the input uses a pull-up), otherwise it is a release.
 * 3) Send the event to `button_queue` using `xQueueSendFromISR()`. Provide
 *    a pointer to `hp_task_woken` so the RTOS can request a context switch
 *    if a higher-priority task was unblocked.
 * 4) If `hp_task_woken` was set, yield from ISR to allow immediate task
 *    scheduling.
 */
static void IRAM_ATTR button_isr(void *arg)
{
    button_event_t evt;
    /* Timestamp the event with ISR-safe tick retrieval */
    evt.timestamp = xTaskGetTickCountFromISR();

    /* Determine whether the interrupt corresponds to a press or release */
    evt.type = gpio_get_level(BUTTON_PIN) == 0 ?
               BUTTON_PRESS : BUTTON_RELEASE;

    BaseType_t hp_task_woken = pdFALSE;
    /* Queue the event from ISR context */
    xQueueSendFromISR(button_queue, &evt, &hp_task_woken);

    /* If a higher-priority task was woken by the queue send, request a yield */
    if (hp_task_woken) {
        portYIELD_FROM_ISR();
    }
}

// =======================
// Tasks
// =======================

#define LONG_PRESS_MS   1000
#define FAST_PRESS_MS   300

/**
 * button_task
 *
 * Step-by-step:
 * 1) Block on `button_queue` waiting for ISR-generated button events.
 * 2) On a `BUTTON_PRESS` event: record the timestamp so we can measure
 *    how long the button is held.
 * 3) On a `BUTTON_RELEASE` event: compute the hold duration by subtracting
 *    the saved press timestamp from the release timestamp.
 * 4) If the hold duration exceeds `LONG_PRESS_MS` treat it as a long-press
 *    (set `long_press` flag). Reset `fast_count` since the long press breaks
 *    the fast-press sequence.
 * 5) For short releases:
 *    - If a long-press had occurred previously, increment `fast_count` and
 *      when it reaches 3 send a `DISPLAY_CMD_HELLO` command to the display
 *      task, then reset the long-press tracking variables.
 *    - If no long-press context exists, send a `DISPLAY_CMD_NEXT_DIGIT`
 *      command to advance the displayed digit once.
 */
static void button_task(void *arg)
{
    button_event_t evt;
    TickType_t press_time = 0;
    bool long_press = false;
    int fast_count = 0;

    while (1) {
        /* Wait indefinitely for the next button event from the ISR */
        if (xQueueReceive(button_queue, &evt, portMAX_DELAY)) {

            /* If the event is a press, remember when it happened */
            if (evt.type == BUTTON_PRESS) {
                press_time = evt.timestamp;
            }

            /* If the event is a release, decide what action to take */
            if (evt.type == BUTTON_RELEASE) {
                TickType_t held =
                    evt.timestamp - press_time;

                /* Long press handling */
                if (held >= pdMS_TO_TICKS(LONG_PRESS_MS)) {
                    long_press = true;
                    fast_count = 0;
                } else {
                    /* Short release handling */
                    if (long_press) {
                        /* After a long press, count short presses to trigger HELLO */
                        fast_count++;
                        if (fast_count == 3) {
                            display_cmd_t cmd = DISPLAY_CMD_HELLO;
                            xQueueSend(display_queue, &cmd, 0);
                            long_press = false;
                            fast_count = 0;
                        }
                    } else {
                        /* Regular single press: advance the digit */
                        display_cmd_t cmd = DISPLAY_CMD_NEXT_DIGIT;
                        xQueueSend(display_queue, &cmd, 0);
                    }
                }
            }
        }
    }
}

/**
 * display_task
 *
 * Step-by-step:
 * 1) Block on `display_queue` waiting for commands from the button task.
 * 2) If command is `DISPLAY_CMD_NEXT_DIGIT` display the current `digit`
 *    and then increment the index (wrapping 0..9).
 * 3) If command is `DISPLAY_CMD_HELLO` show the sequence of letters
 *    stored in `letters[]`, delaying between characters to make the
 *    message readable.
 */
static void display_task(void *arg)
{
    display_cmd_t cmd;
    int digit = 0;

    while (1) {
        /* Wait for the next display command */
        if (xQueueReceive(display_queue, &cmd, portMAX_DELAY)) {

            if (cmd == DISPLAY_CMD_NEXT_DIGIT) {
                /* Render the current digit, then advance */
                display_digit(digit);
                digit = (digit + 1) % 10;
            }

            if (cmd == DISPLAY_CMD_HELLO) {
                /* Show the HELLO sequence, pausing between letters */
                for (int i = 0; i < 5; i++) {
                    display_letter(i);
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                display_digit(digit);
            }
        }
    }
}

// =======================
// app_main
// =======================

/**
 * app_main
 *
 * Step-by-step:
 * 1) Initialize the segment GPIOs to outputs and set them to OFF.
 * 2) Configure the button GPIO as an input with internal pull-up enabled
 *    and request interrupts on any edge so presses and releases are seen.
 * 3) Create FreeRTOS queues used to pass button events and display commands
 *    between ISR, button task, and display task.
 * 4) Install the GPIO ISR service and register `button_isr` for the
 *    `BUTTON_PIN` so button edges generate events.
 * 5) Create the `button_task` and `display_task` which consume queue
 *    messages and perform higher-level logic (debouncing, long-press
 *    behavior, and rendering).
 */
void app_main(void)
{
    /* 1) Initialize segment outputs */
    init_segments();

    /* 2) Configure the button GPIO (input + pull-up + any-edge interrupts) */
    gpio_config_t btn_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&btn_conf);

    /* 3) Create queues for communication between ISR and tasks */
    button_queue  = xQueueCreate(10, sizeof(button_event_t));
    display_queue = xQueueCreate(5, sizeof(display_cmd_t));

    /* 4) Install ISR service and attach handler for the button pin */
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, button_isr, NULL);

    /* 5) Start the button and display tasks */
    xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
    xTaskCreate(display_task, "display_task", 2048, NULL, 5, NULL);
}