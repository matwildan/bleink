#ifndef BLE_ESS_SERVICE_H
#define BLE_ESS_SERVICE_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize the Environmental Sensing Service
 *
 * @return 0 on success, negative errno on failure
 */
int ble_ess_service_init(void);

/**
 * @brief Update temperature value
 *
 * @param temp_celsius Temperature in Celsius * 100 (e.g., 2250 = 22.50°C)
 */
void ess_update_temperature(int16_t temp_celsius);

/**
 * @brief Update humidity value
 *
 * @param humidity_percent Humidity in percent * 100 (e.g., 5500 = 55.00%)
 */
void ess_update_humidity(uint16_t humidity_percent);

/**
 * @brief Start automatic sensor data updates (dummy data)
 *
 * This will update the sensor values every 10 seconds with dummy data
 */
void ess_start_auto_update(void);

#endif /* BLE_ESS_SERVICE_H */
