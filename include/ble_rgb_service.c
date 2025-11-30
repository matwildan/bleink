#include "ble_rgb_service.h"
#include "display_epaper.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble_rgb, LOG_LEVEL_INF);

/* Custom RGB LED Service UUID */
#define RGB_SERVICE_UUID_VAL 0xFFE0
#define RGB_CHAR_UUID_VAL 0xFFE1
#define TEXT_CHAR_UUID_VAL 0xFFE2

#define BT_UUID_RGB_SERVICE   BT_UUID_DECLARE_16(RGB_SERVICE_UUID_VAL)
#define BT_UUID_RGB_CHAR      BT_UUID_DECLARE_16(RGB_CHAR_UUID_VAL)
#define BT_UUID_TEXT_CHAR     BT_UUID_DECLARE_16(TEXT_CHAR_UUID_VAL)

/* RGB LED data */
static uint8_t rgb_values[3] = {0, 0, 0}; /* R, G, B */

/* Text buffer for display */
#define TEXT_BUFFER_SIZE 128
static char text_buffer[TEXT_BUFFER_SIZE];

/* PWM devices */
#if DT_NODE_EXISTS(DT_NODELABEL(red_pwm_led))
static const struct pwm_dt_spec pwm_led_red = PWM_DT_SPEC_GET(DT_NODELABEL(red_pwm_led));
static const struct pwm_dt_spec pwm_led_green = PWM_DT_SPEC_GET(DT_NODELABEL(green_pwm_led));
static const struct pwm_dt_spec pwm_led_blue = PWM_DT_SPEC_GET(DT_NODELABEL(blue_pwm_led));
#else
#error "PWM LED nodes not found in device tree"
#endif

/* Set RGB LED color */
void rgb_led_set_color(uint8_t red, uint8_t green, uint8_t blue)
{
	uint32_t pulse_red, pulse_green, pulse_blue;

	/* Convert 0-255 to PWM pulse width (0-100% of period) */
	pulse_red = (pwm_led_red.period * red) / 255;
	pulse_green = (pwm_led_green.period * green) / 255;
	pulse_blue = (pwm_led_blue.period * blue) / 255;

	pwm_set_pulse_dt(&pwm_led_red, pulse_red);
	pwm_set_pulse_dt(&pwm_led_green, pulse_green);
	pwm_set_pulse_dt(&pwm_led_blue, pulse_blue);

	LOG_INF("RGB LED: R=%d, G=%d, B=%d", red, green, blue);
}

/* RGB Characteristic Write Callback */
static ssize_t write_rgb(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf, uint16_t len, uint16_t offset,
			 uint8_t flags)
{
	if (offset + len > sizeof(rgb_values)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(rgb_values + offset, buf, len);

	if (len >= 3) {
		rgb_led_set_color(rgb_values[0], rgb_values[1], rgb_values[2]);
	}

	return len;
}

/* RGB Characteristic Read Callback */
static ssize_t read_rgb(struct bt_conn *conn,
			const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 rgb_values, sizeof(rgb_values));
}

/* Text Characteristic Write Callback */
static ssize_t write_text(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf, uint16_t len, uint16_t offset,
			  uint8_t flags)
{
	if (offset + len > TEXT_BUFFER_SIZE - 1) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(text_buffer + offset, buf, len);
	text_buffer[offset + len] = '\0'; /* Null terminate */

	LOG_INF("Received text (%d bytes): %s", len, text_buffer);

	/* Update the display with the received text */
	display_show_message(text_buffer);

	return len;
}

/* Text Characteristic Read Callback */
static ssize_t read_text(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 void *buf, uint16_t len, uint16_t offset)
{
	size_t text_len = strlen(text_buffer);
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 text_buffer, text_len);
}

/* RGB LED Service Declaration */
BT_GATT_SERVICE_DEFINE(rgb_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_RGB_SERVICE),

	/* RGB Characteristic - Write RGB values (3 bytes: R, G, B) */
	BT_GATT_CHARACTERISTIC(BT_UUID_RGB_CHAR,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_rgb, write_rgb, NULL),

	/* Text Characteristic - Write text to display (up to 127 chars) */
	BT_GATT_CHARACTERISTIC(BT_UUID_TEXT_CHAR,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_text, write_text, NULL),
);

int ble_rgb_service_init(void)
{
	/* Check PWM devices */
	if (!pwm_is_ready_dt(&pwm_led_red)) {
		LOG_ERR("Red PWM device not ready");
		return -ENODEV;
	}
	if (!pwm_is_ready_dt(&pwm_led_green)) {
		LOG_ERR("Green PWM device not ready");
		return -ENODEV;
	}
	if (!pwm_is_ready_dt(&pwm_led_blue)) {
		LOG_ERR("Blue PWM device not ready");
		return -ENODEV;
	}

	/* Turn off RGB LED initially */
	rgb_led_set_color(0, 0, 0);

	/* Initialize text buffer */
	memset(text_buffer, 0, TEXT_BUFFER_SIZE);

	LOG_INF("RGB LED service initialized");
	LOG_INF("Text display characteristic available (UUID: 0x%04X)", TEXT_CHAR_UUID_VAL);

	return 0;
}
