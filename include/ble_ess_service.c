#include "ble_ess_service.h"
#include "display_epaper.h"
#include "battery.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_ess, LOG_LEVEL_INF);

/* Environmental Sensing Service UUID */
#define ESS_UUID_VAL 0x181A
#define TEMPERATURE_UUID_VAL 0x2A6E
#define HUMIDITY_UUID_VAL 0x2A6F

#define BT_UUID_ESS           BT_UUID_DECLARE_16(ESS_UUID_VAL)
#define BT_UUID_TEMPERATURE   BT_UUID_DECLARE_16(TEMPERATURE_UUID_VAL)
#define BT_UUID_HUMIDITY      BT_UUID_DECLARE_16(HUMIDITY_UUID_VAL)

/* Sensor data (dummy values) */
static int16_t temperature = 2250;  /* 22.50°C (value * 0.01) */
static uint16_t humidity = 5500;     /* 55.00% (value * 0.01) */

/* Update functions */
void ess_update_temperature(int16_t temp_celsius)
{
	temperature = temp_celsius;
	LOG_DBG("Temperature updated: %d.%02d°C", temperature / 100, temperature % 100);
}

void ess_update_humidity(uint16_t humidity_percent)
{
	humidity = humidity_percent;
	LOG_DBG("Humidity updated: %d.%02d%%", humidity / 100, humidity % 100);
}

/* Temperature Characteristic Read Callback */
static ssize_t read_temperature(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	int16_t temp_value = sys_cpu_to_le16(temperature);

	LOG_INF("Temperature read: %d.%02d°C", temperature / 100, temperature % 100);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &temp_value, sizeof(temp_value));
}

/* Humidity Characteristic Read Callback */
static ssize_t read_humidity(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	uint16_t humidity_value = sys_cpu_to_le16(humidity);

	LOG_INF("Humidity read: %d.%02d%%", humidity / 100, humidity % 100);

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &humidity_value, sizeof(humidity_value));
}

/* Environmental Sensing Service Declaration */
BT_GATT_SERVICE_DEFINE(ess_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ESS),

	/* Temperature Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_TEMPERATURE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       read_temperature, NULL, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	/* Humidity Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       read_humidity, NULL, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* Auto-update timer and work */
static void update_sensor_data(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(sensor_update_work, update_sensor_data);

static void update_sensor_data(struct k_work *work)
{
	/* Generate dummy temperature: 20.00°C to 25.00°C */
	static int16_t temp_offset = 0;
	temperature = 2200 + (temp_offset % 500);
	temp_offset += 50;

	/* Generate dummy humidity: 45.00% to 65.00% */
	static int16_t hum_offset = 0;
	humidity = 5000 + (hum_offset % 2000);
	hum_offset += 200;

	LOG_INF("Sensor updated - Temp: %d.%02d°C, Humidity: %d.%02d%%",
		temperature / 100, temperature % 100,
		humidity / 100, humidity % 100);

	/* Update display */
	display_update_sensors(temperature, humidity);

	/* Update battery display */
	uint16_t voltage = battery_read_voltage();
	uint8_t battery_pct = battery_get_percentage(voltage);
	display_update_battery(voltage, battery_pct);

	/* Schedule next update in 10 seconds */
	k_work_schedule(&sensor_update_work, K_SECONDS(10));
}

void ess_start_auto_update(void)
{
	LOG_INF("Starting automatic sensor updates");
	k_work_schedule(&sensor_update_work, K_SECONDS(10));
}

int ble_ess_service_init(void)
{
	LOG_INF("ESS initialized - Temp: %d.%02d°C, Humidity: %d.%02d%%",
		temperature / 100, temperature % 100,
		humidity / 100, humidity % 100);
	return 0;
}
