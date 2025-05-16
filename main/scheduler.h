#pragma once

typedef struct {
    int hour;
    int minute;
    bool state; // true: ON, false: OFF
} schedule_entry_t;

void scheduler_init(void);
void scheduler_add_entry(schedule_entry_t entry);
void scheduler_clear_entries(void);
