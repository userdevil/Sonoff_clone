// main/wifi_manager.c

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_smartconfig.h"
#include "wifi_manager.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define SMARTCONFIG_DONE_BIT BIT2

static const char *TAG = "WIFI_MANAGER";
static EventGroupHandle_t wifi_event_group;
static bool is_connected = false;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data) {

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Starting WiFi...");
        esp_wifi_connect();
    }

    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        is_connected = false;
        ESP_LOGW(TAG, "Disconnected. Trying to reconnect...");
        esp_wifi_connect();
    }

    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        is_connected = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "Got IP");
    }

    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;

        wifi_config_t wifi_config;
        memset(&wifi_config, 0, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));

        ESP_LOGI(TAG, "Got WiFi credentials via SmartConfig: SSID=%s", evt->ssid);
        esp_wifi_disconnect();
        esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
        esp_wifi_connect();

        // Save credentials to NVS
        nvs_handle_t nvs;
        if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
            nvs_set_str(&nvs, "ssid", (const char*)evt->ssid);
            nvs_set_str(&nvs, "pass", (const char*)evt->password);
            nvs_commit(&nvs);
            nvs_close(nvs);
        }

        xEventGroupSetBits(wifi_event_group, SMARTCONFIG_DONE_BIT);
    }
}

static void smartconfig_task(void *param) {
    ESP_ERROR_CHECK(esp_smartconfig_start(SC_TYPE_ESPTOUCH, NULL));
    ESP_LOGI(TAG, "Waiting for SmartConfig...");

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT | SMARTCONFIG_DONE_BIT,
                                           true, false, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WiFi Connected");
    }

    if (bits & SMARTCONFIG_DONE_BIT) {
        ESP_LOGI(TAG, "SmartConfig Complete");
        esp_smartconfig_stop();
    }

    vTaskDelete(NULL);
}

void wifi_manager_init(void) {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Check if credentials exist
    nvs_handle_t nvs;
    char ssid[32] = {0}, pass[64] = {0};
    size_t ssid_len = sizeof(ssid), pass_len = sizeof(pass);
    esp_err_t ssid_err, pass_err;

    ssid_err = nvs_open("storage", NVS_READONLY, &nvs);
    if (ssid_err == ESP_OK) {
        ssid_err = nvs_get_str(nvs, "ssid", ssid, &ssid_len);
        pass_err = nvs_get_str(nvs, "pass", pass, &pass_len);
        nvs_close(nvs);
    }

    if (ssid_err == ESP_OK && pass_err == ESP_OK) {
        ESP_LOGI(TAG, "Found saved credentials, connecting to %s", ssid);
        wifi_config_t wifi_config;
        memset(&wifi_config, 0, sizeof(wifi_config));
        strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
        strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        esp_wifi_connect();
    } else {
        ESP_LOGW(TAG, "No saved WiFi. Starting SmartConfig...");
        xTaskCreate(smartconfig_task, "smartconfig_task", 4096, NULL, 3, NULL);
    }
}

bool wifi_manager_is_connected(void) {
    return is_connected;
}
