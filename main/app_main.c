// main/app_main.c

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_manager.h"
#include "mqtt_client.h"
#include "gpio_control.h"
#include "ota_update.h"

static const char *TAG = "APP_MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "Starting Sonoff-like Smart Device...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Initialize GPIOs (relay, LED, button)
    gpio_control_init();

    // Start WiFi with SmartConfig (if needed)
    wifi_manager_init();

    // Start MQTT client (once connected to WiFi)
    mqtt_client_start();

    // Monitor OTA in background
    ota_update_start();
}
