#ifndef BLE_RGB_SERVICE_H
#define BLE_RGB_SERVICE_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize the RGB LED service
 *
 * This initializes the PWM hardware and sets up the RGB LED
 *
 * @return 0 on success, negative errno on failure
 */
int ble_rgb_service_init(void);

/**
 * @brief Set RGB LED color directly
 *
 * @param red Red value (0-255)
 * @param green Green value (0-255)
 * @param blue Blue value (0-255)
 */
void rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue);

#endif /* BLE_RGB_SERVICE_H */
