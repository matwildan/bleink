#ifndef BATTERY_H
#define BATTERY_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize battery monitoring
 *
 * @return 0 on success, negative errno on failure
 */
int battery_init(void);

/**
 * @brief Read battery voltage
 *
 * @return Battery voltage in millivolts (mV)
 */
uint16_t battery_read_voltage(void);

/**
 * @brief Get battery percentage (0-100%)
 *
 * @param voltage_mv Battery voltage in millivolts
 * @return Battery level as percentage
 */
uint8_t battery_get_percentage(uint16_t voltage_mv);

#endif /* BATTERY_H */
