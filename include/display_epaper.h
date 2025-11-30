#ifndef DISPLAY_EPAPER_H
#define DISPLAY_EPAPER_H

#include <zephyr/kernel.h>

/**
 * @brief Display rotation angles
 */
enum display_rotation {
	DISPLAY_ROTATION_0 = 0,    /* Normal orientation */
	DISPLAY_ROTATION_90 = 90,  /* Rotated 90 degrees clockwise */
	DISPLAY_ROTATION_180 = 180, /* Rotated 180 degrees */
	DISPLAY_ROTATION_270 = 270  /* Rotated 270 degrees clockwise */
};

/**
 * @brief Initialize the E-Paper display
 *
 * @return 0 on success, negative errno on failure
 */
int display_epaper_init(void);

/**
 * @brief Update the display with current sensor data
 *
 * @param temp_celsius Temperature in Celsius * 100 (e.g., 2250 = 22.50°C)
 * @param humidity_percent Humidity in percent * 100 (e.g., 5500 = 55.00%)
 */
void display_update_sensors(int16_t temp_celsius, uint16_t humidity_percent);

/**
 * @brief Show a message on the display
 *
 * @param message Message to display
 */
void display_show_message(const char *message);

/**
 * @brief Set display rotation
 *
 * @param rotation Rotation angle (0, 90, 180, or 270 degrees)
 * @return 0 on success, negative errno on failure
 */
int display_set_rotation(enum display_rotation rotation);

#endif /* DISPLAY_EPAPER_H */
