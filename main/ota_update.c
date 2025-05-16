#include "ota_update.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_log.h"

#define BUFFSIZE 1024
static const char *TAG = "OTA";

void perform_ota_update(const char *url) {
    esp_http_client_config_t config = {
        .url = url,
        .timeout_ms = 10000,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return;
    }

    if (esp_http_client_open(client, 0) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection");
        esp_http_client_cleanup(client);
        return;
    }

    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
    if (!update_partition) {
        ESP_LOGE(TAG, "No OTA partition found");
        esp_http_client_cleanup(client);
        return;
    }

    ESP_LOGI(TAG, "Writing to partition: %s", update_partition->label);
    esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed");
        esp_http_client_cleanup(client);
        return;
    }

    char buffer[BUFFSIZE];
    int binary_file_len = 0;
    while (1) {
        int data_read = esp_http_client_read(client, buffer, BUFFSIZE);
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            break;
        } else if (data_read > 0) {
            err = esp_ota_write(update_handle, buffer, data_read);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "esp_ota_write failed");
                break;
            }
            binary_file_len += data_read;
        } else if (data_read == 0) {
            break;
        }
    }

    if (esp_ota_end(update_handle) == ESP_OK) {
        if (esp_ota_set_boot_partition(update_partition) == ESP_OK) {
            ESP_LOGI(TAG, "OTA Update successful, rebooting...");
            esp_restart();
        } else {
            ESP_LOGE(TAG, "Failed to set boot partition");
        }
    } else {
        ESP_LOGE(TAG, "OTA update failed");
    }

    esp_http_client_cleanup(client);
}
