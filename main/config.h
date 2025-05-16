#pragma once

// Wi-Fi settings
#define WIFI_SSID_MAX_LEN     32
#define WIFI_PASS_MAX_LEN     64

// MQTT settings
#define MQTT_BROKER_URI       "mqtts://your-mqtt-broker.com:8883"
#define MQTT_USERNAME         "your_mqtt_username"
#define MQTT_PASSWORD         "your_mqtt_password"
#define MQTT_CLIENT_ID_PREFIX "sonoff_device_"

// GPIO Pins
#define RELAY_GPIO            GPIO_NUM_12
#define BUTTON_GPIO           GPIO_NUM_0
#define LED_GPIO              GPIO_NUM_13

// OTA update URL base
#define OTA_BASE_URL          "https://yourserver.com/firmware/"

// Device info
#define DEVICE_TYPE           "sonoff_like_device"
#define FIRMWARE_VERSION      "1.0.0"
#define DEVICE_ID_MAX_LEN     32

// Task stack sizes
#define WIFI_MANAGER_STACK_SIZE 4096
#define MQTT_CLIENT_STACK_SIZE  4096

// Debug tag
#define DEBUG_TAG             "SONOFF_FW"
