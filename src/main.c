#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/logging/log.h>

#include "../include/ble_rgb_service.h"
#include "../include/ble_ess_service.h"
#include "../include/display_epaper.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* BLE advertising data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      0x1A, 0x18,  /* Environmental Sensing Service (0x181A) - little endian */
		      0xE0, 0xFF), /* RGB LED Service (0xFFE0) - little endian */
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

/* Connection callbacks */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)", err);
	} else {
		LOG_INF("Connected");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02x)", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void)
{
	int err;

	LOG_INF("=== BLE H&T Sensor Starting ===");

	/* Initialize RGB LED service */
	err = ble_rgb_service_init();
	if (err) {
		LOG_ERR("RGB service init failed (err %d)", err);
		return 0;
	}

	/* Initialize Environmental Sensing Service */
	err = ble_ess_service_init();
	if (err) {
		LOG_ERR("ESS init failed (err %d)", err);
		return 0;
	}

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}
	LOG_INF("Bluetooth initialized");

	/* Start BLE advertising */
	struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.sid = 0,
		.secondary_max_skip = 0,
		.options = BT_LE_ADV_OPT_CONN,
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
		.peer = NULL,
	};

	err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return 0;
	}

	LOG_INF("Advertising started as '%s'", CONFIG_BT_DEVICE_NAME);
	LOG_INF("Services ready:");
	LOG_INF("  - Environmental Sensing Service (0x181A)");
	LOG_INF("  - RGB LED Service (0xFFE0)");

	/* Start automatic sensor data updates */
	ess_start_auto_update();

	/* Initialize E-Paper Display */
	err = display_epaper_init();
	if (err) {
		LOG_ERR("Display init failed (err %d)", err);
		return 0;
	}

	/* Main loop - just sleep */
	while (1) {
		k_sleep(K_FOREVER);
	}

	return 0;
}
