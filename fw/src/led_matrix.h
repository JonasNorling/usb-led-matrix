#pragma once

#define LED_COUNT 64

void led_matrix_init(void);
void led_matrix_start(void);
void led_matrix_set(const uint8_t *data, size_t len);
