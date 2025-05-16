// main/gpio_control.c

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "gpio_control.h"

#define RELAY_GPIO      5     // Change as per your hardware
#define LED_GPIO        2     // Status LED
#define BUTTON_GPIO     0     // Physical push button

#define LONG_PRESS_MS   3000

static const char *TAG = "GPIO_CTRL";
static bool relay_state = false;
static TimerHandle_t button_timer;
static bool long_press_triggered = false;

static void update_led() {
    gpio_set_level(LED_GPIO, relay_state ? 1 : 0);
}

void gpio_control_set_relay(bool on) {
    relay_state = on;
    gpio_set_level(RELAY_GPIO, on ? 1 : 0);
    update_led();
    ESP_LOGI(TAG, "Relay %s", on ? "ON" : "OFF");
}

bool gpio_control_get_relay_state(void) {
    return relay_state;
}

void gpio_control_handle_button_event(button_event_t event) {
    if (event == BUTTON_SHORT_PRESS) {
        gpio_control_set_relay(!relay_state); // Toggle relay
    } else if (event == BUTTON_LONG_PRESS) {
        ESP_LOGI(TAG, "LONG press detected, start pairing");
        esp_restart(); // Restart to enter SmartConfig mode
    }
}

static void IRAM_ATTR button_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTimerStartFromISR(button_timer, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void button_timer_callback(TimerHandle_t xTimer) {
    static int64_t pressed_time = 0;
    static bool button_down = false;

    int level = gpio_get_level(BUTTON_GPIO);
    if (level == 0 && !button_down) {
        button_down = true;
        pressed_time = esp_timer_get_time();
    } else if (level == 1 && button_down) {
        button_down = false;
        int64_t elapsed_ms = (esp_timer_get_time() - pressed_time) / 1000;

        if (elapsed_ms >= LONG_PRESS_MS) {
            gpio_control_handle_button_event(BUTTON_LONG_PRESS);
        } else {
            gpio_control_handle_button_event(BUTTON_SHORT_PRESS);
        }
    }
}

void gpio_control_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO) | (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(RELAY_GPIO, 0);
    gpio_set_level(LED_GPIO, 0);

    // Button configuration
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = 1,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&btn_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr_handler, NULL);

    button_timer = xTimerCreate("btn_timer", pdMS_TO_TICKS(10), pdTRUE, NULL, button_timer_callback);
    xTimerStart(button_timer, 0);

    ESP_LOGI(TAG, "GPIO control initialized");
}
