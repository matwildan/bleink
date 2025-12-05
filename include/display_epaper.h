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
 * @brief Initialize sensor display with static labels
 */
void display_init_sensor_labels(void);

/**
 * @brief Update only the sensor values (not labels)
 *
 * @param temp_celsius Temperature in Celsius * 100 (e.g., 2250 = 22.50°C)
 * @param humidity_percent Humidity in percent * 100 (e.g., 5500 = 55.00%)
 */
void display_update_sensors(int16_t temp_celsius, uint16_t humidity_percent);

/**
 * @brief Update battery display
 *
 * @param voltage_mv Battery voltage in millivolts
 * @param percentage Battery percentage (0-100)
 */
void display_update_battery(uint16_t voltage_mv, uint8_t percentage);

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

/**
 * @brief Draw a monochrome image on the display
 *
 * @param image_data Pointer to image data (1 bit per pixel, row-major order)
 * @param x X coordinate (0-249)
 * @param y Y coordinate (0-119)
 * @param width Image width in pixels
 * @param height Image height in pixels
 * @return 0 on success, negative errno on failure
 */
int display_draw_image(const uint8_t *image_data, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height);

/**
 * @brief Add a temperature reading to the graph history
 *
 * @param temp_celsius Temperature in Celsius * 100 (e.g., 2250 = 22.50°C)
 */
void display_add_temp_reading(int16_t temp_celsius);

/**
 * @brief Draw the temperature graph
 */
void display_draw_graph(void);

#endif /* DISPLAY_EPAPER_H */
