// main/mqtt_client.h

#pragma once

void mqtt_app_start(const char *device_id);
void mqtt_publish_relay_state(bool state);
void mqtt_publish_heartbeat(void);
