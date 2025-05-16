#include "scheduler.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "gpio_control.h"
#include <time.h>

#define MAX_SCHEDULE_ENTRIES 10

static const char *TAG = "SCHEDULER";
static schedule_entry_t schedule_entries[MAX_SCHEDULE_ENTRIES];
static int schedule_count = 0;
static esp_timer_handle_t periodic_timer;

static void check_schedule(void* arg) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    for (int i = 0; i < schedule_count; i++) {
        if (schedule_entries[i].hour == timeinfo.tm_hour &&
            schedule_entries[i].minute == timeinfo.tm_min) {
            gpio_control_set_relay(schedule_entries[i].state);
            ESP_LOGI(TAG, "Executed schedule: %02d:%02d -> %s",
                     schedule_entries[i].hour,
                     schedule_entries[i].minute,
                     schedule_entries[i].state ? "ON" : "OFF");
        }
    }
}

void scheduler_init(void) {
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &check_schedule,
        .name = "schedule_timer"
    };
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, 60000000); // Check every 60 seconds
}

void scheduler_add_entry(schedule_entry_t entry) {
    if (schedule_count < MAX_SCHEDULE_ENTRIES) {
        schedule_entries[schedule_count++] = entry;
        ESP_LOGI(TAG, "Added schedule: %02d:%02d -> %s",
                 entry.hour, entry.minute, entry.state ? "ON" : "OFF");
    } else {
        ESP_LOGW(TAG, "Schedule list full");
    }
}

void scheduler_clear_entries(void) {
    schedule_count = 0;
    ESP_LOGI(TAG, "Cleared all schedules");
}
