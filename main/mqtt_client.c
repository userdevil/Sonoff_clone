// main/mqtt_client.c

#include "mqtt_client.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "gpio_control.h"
#include "cJSON.h"

#define BROKER_URI      "mqtts://your-cloud.broker.com"  // Replace with your domain
#define MQTT_USERNAME   "device_user"                     // Use real credentials from cloud
#define MQTT_PASSWORD   "device_pass"
#define CMD_TOPIC_FMT   "devices/%s/cmd"
#define STATUS_TOPIC_FMT "devices/%s/status"

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t client = NULL;
static char device_id[32];
static char topic_cmd[64];
static char topic_status[64];

static void mqtt_publish_status() {
    cJSON *status = cJSON_CreateObject();
    cJSON_AddBoolToObject(status, "relay", gpio_control_get_relay_state());
    char *msg = cJSON_PrintUnformatted(status);
    esp_mqtt_client_publish(client, topic_status, msg, 0, 1, 0);
    cJSON_free(msg);
    cJSON_Delete(status);
}

static void handle_command(const char *data) {
    cJSON *cmd = cJSON_Parse(data);
    if (!cmd) return;

    cJSON *action = cJSON_GetObjectItem(cmd, "action");
    if (action && cJSON_IsString(action)) {
        if (strcmp(action->valuestring, "on") == 0) {
            gpio_control_set_relay(true);
        } else if (strcmp(action->valuestring, "off") == 0) {
            gpio_control_set_relay(false);
        }
        mqtt_publish_status();
    }

    cJSON_Delete(cmd);
}

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event) {
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Connected to MQTT broker");
            esp_mqtt_client_subscribe(client, topic_cmd, 1);
            mqtt_publish_status();
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT Data received [%.*s]: %.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);
            handle_command(event->data);
            break;

        default:
            break;
    }
    return ESP_OK;
}

void mqtt_app_start(const char *id) {
    strncpy(device_id, id, sizeof(device_id));
    snprintf(topic_cmd, sizeof(topic_cmd), CMD_TOPIC_FMT, device_id);
    snprintf(topic_status, sizeof(topic_status), STATUS_TOPIC_FMT, device_id);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = BROKER_URI,
        .username = MQTT_USERNAME,
        .password = MQTT_PASSWORD,
        .cert_pem = NULL, // Optional: add TLS cert here
        .event_handle = mqtt_event_handler_cb,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void mqtt_publish_relay_state(bool state) {
    mqtt_publish_status();
}

void mqtt_publish_heartbeat() {
    esp_mqtt_client_publish(client, topic_status, "{\"status\":\"alive\"}", 0, 1, 0);
}
