#include "display_epaper.h"
#include "ble_rgb_service.h"
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(display, LOG_LEVEL_INF);

static const struct device *display_dev;
static enum display_rotation current_rotation = DISPLAY_ROTATION_180;

int display_epaper_init(void)
{
	int ret;
	uint16_t rows, cols;

	/* Yellow LED - Display init starting */
	rgb_led_set_color(255, 255, 0);
	k_sleep(K_MSEC(500));

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

	if (!device_is_ready(display_dev)) {
		LOG_ERR("Display device not ready");
		rgb_led_set_color(255, 0, 0);
		return -ENODEV;
	}

	/* Set rotation to 180 degrees */
	display_set_rotation(DISPLAY_ROTATION_180);

	/* Initialize CFB */
	ret = cfb_framebuffer_init(display_dev);
	if (ret != 0) {
		LOG_ERR("CFB init failed: %d", ret);
		rgb_led_set_color(255, 0, 0);
		return ret;
	}

	/* Get display dimensions */
	rows = cfb_get_display_parameter(display_dev, CFB_DISPLAY_ROWS);
	cols = cfb_get_display_parameter(display_dev, CFB_DISPLAY_COLS);
	LOG_INF("CFB dimensions: %d rows x %d cols", rows, cols);

	/* Also log pixel dimensions */
	struct display_capabilities caps;
	display_get_capabilities(display_dev, &caps);
	LOG_INF("Pixel dimensions: %d x %d", caps.x_resolution, caps.y_resolution);

	cfb_framebuffer_clear(display_dev, false);

	/* Turn off blanking (enable display) */
	ret = display_blanking_off(display_dev);
	if (ret != 0) {
		LOG_ERR("Failed to turn off blanking: %d", ret);
		rgb_led_set_color(255, 0, 0);
		return ret;
	}

	/* Invert display for black text on white background */
	cfb_framebuffer_invert(display_dev);

	/* Draw Hello World in middle of display */
	/* With 120px height and 8px font, center is at row 7 (rows/2 = 15/2 = 7) */
	ret = cfb_print(display_dev, "Hello World!", 0, 8);
	if (ret != 0) {
		LOG_ERR("CFB print failed: %d", ret);
	}
	
	cfb_framebuffer_finalize(display_dev);
	
	LOG_INF("E-Paper display initialized with CFB");

	/* GREEN LED - Success! */
	rgb_led_set_color(0, 255, 0);
	k_sleep(K_SECONDS(2));
	rgb_led_set_color(0, 0, 0);

	return 0;
}

void display_show_message(const char *message)
{
	uint16_t rows;

	if (!message) {
		return;
	}

	/* Clear to white and invert for black text */
	cfb_framebuffer_clear(display_dev, false);

	/* Get display rows and center text vertically */
	rows = cfb_get_display_parameter(display_dev, CFB_DISPLAY_ROWS);

	/* Position text in middle of display */
	cfb_print(display_dev, message, 0, 8);

	/* Finalize to update display */
	cfb_framebuffer_finalize(display_dev);

	LOG_INF("Displayed: %s", message);
}

void display_init_sensor_labels(void)
{
	/* Clear and setup initial display with labels */
	cfb_framebuffer_clear(display_dev, false);

	/* Draw static labels */
	cfb_print(display_dev, "Temp:", 0, 8);
	cfb_print(display_dev, "Humid:", 0, 24);
	cfb_print(display_dev, "Batt:", 0, 40);

	cfb_framebuffer_finalize(display_dev);

	LOG_INF("Sensor labels initialized");
}

void display_update_sensors(int16_t temp_celsius, uint16_t humidity_percent)
{
	char temp_buf[32];
	char humid_buf[32];

	/* Format only the values */
	snprintf(temp_buf, sizeof(temp_buf), "%d.%02d C",
		 temp_celsius / 100, abs(temp_celsius % 100));
	snprintf(humid_buf, sizeof(humid_buf), "%d.%02d %%",
		 humidity_percent / 100, humidity_percent % 100);

	/* Don't clear - just overwrite the values */
	cfb_print(display_dev, temp_buf, 60, 8);      /* Print temp value at offset */
	cfb_print(display_dev, humid_buf, 60, 24);    /* Print humidity value below */

	cfb_framebuffer_finalize(display_dev);

	LOG_INF("Updated: Temp=%s, Humidity=%s", temp_buf, humid_buf);
}

void display_update_battery(uint16_t voltage_mv, uint8_t percentage)
{
	char batt_buf[32];

	/* Format battery info: voltage and percentage */
	snprintf(batt_buf, sizeof(batt_buf), "%d.%02dV %d%%",
		 voltage_mv / 1000, (voltage_mv % 1000) / 10, percentage);

	/* Print battery value */
	cfb_print(display_dev, batt_buf, 60, 40);

	cfb_framebuffer_finalize(display_dev);

	LOG_INF("Updated: Battery=%s", batt_buf);
}

int display_set_rotation(enum display_rotation rotation)
{
	int ret;
	enum display_orientation orientation;

	/* Map rotation enum to display orientation */
	switch (rotation) {
	case DISPLAY_ROTATION_0:
		orientation = DISPLAY_ORIENTATION_NORMAL;
		break;
	case DISPLAY_ROTATION_90:
		orientation = DISPLAY_ORIENTATION_ROTATED_90;
		break;
	case DISPLAY_ROTATION_180:
		orientation = DISPLAY_ORIENTATION_ROTATED_180;
		break;
	case DISPLAY_ROTATION_270:
		orientation = DISPLAY_ORIENTATION_ROTATED_270;
		break;
	default:
		LOG_ERR("Invalid rotation: %d", rotation);
		return -EINVAL;
	}

	/* Set the orientation */
	ret = display_set_orientation(display_dev, orientation);
	if (ret != 0) {
		LOG_ERR("Failed to set orientation: %d", ret);
		return ret;
	}

	current_rotation = rotation;
	LOG_INF("Display rotation set to %d degrees", rotation);

	/* Re-initialize CFB after rotation change */
	cfb_framebuffer_clear(display_dev, false);
	cfb_framebuffer_finalize(display_dev);

	return 0;
}
