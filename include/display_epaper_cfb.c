#include "display_epaper.h"
#include "ble_rgb_service.h"
#include "icons.h"
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(display, LOG_LEVEL_INF);

static const struct device *display_dev;
static enum display_rotation current_rotation = DISPLAY_ROTATION_180;

/* Temperature graph data */
#define GRAPH_MAX_POINTS 50  /* Number of data points to store */
#define GRAPH_X 0            /* Graph X position */
#define GRAPH_Y 72           /* Graph Y position (below icons) */
#define GRAPH_WIDTH 250      /* Graph width (full display width) */
#define GRAPH_HEIGHT 48      /* Graph height (72 to 120) */

static int16_t temp_history[GRAPH_MAX_POINTS];
static uint8_t temp_history_count = 0;
static uint8_t temp_history_index = 0;

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

	/* Draw thermometer icon on right side BEFORE finalize */
	/* x = 250 - 64 = 186, y = 24 (must be multiple of 8 for SSD16xx) */
	display_draw_image(icon_thermometer, 186, 24,
			   ICON_THERMOMETER_WIDTH, ICON_THERMOMETER_HEIGHT);

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
	/* Clear and setup initial display with icons */
	cfb_framebuffer_clear(display_dev, false);

	/* Draw temp/humidity icon on the left (0, 0) */
	display_draw_image(icon_thermometer, 0, 7,
			   ICON_THERMOMETER_WIDTH, ICON_THERMOMETER_HEIGHT);

	/* Draw battery icon on the right corner (250-24=226, 0) */
	display_draw_image(icon_full_battery, 215, 10,
			   ICON_FULL_BATTERY_WIDTH, ICON_FULL_BATTERY_HEIGHT);

	cfb_framebuffer_finalize(display_dev);

	LOG_INF("Sensor icons initialized");
}

void display_update_sensors(int16_t temp_celsius, uint16_t humidity_percent)
{
	char temp_buf[32];
	char humid_buf[32];

	/* Add temperature reading to graph history */
	display_add_temp_reading(temp_celsius);

	/* Clear entire framebuffer to redraw everything fresh */
	cfb_framebuffer_clear(display_dev, false);

	/* Redraw icons */
	display_draw_image(icon_thermometer, 0, 7,
			   ICON_THERMOMETER_WIDTH, ICON_THERMOMETER_HEIGHT);
	display_draw_image(icon_full_battery, 215, 10,
			   ICON_FULL_BATTERY_WIDTH, ICON_FULL_BATTERY_HEIGHT);

	/* Format temperature and humidity values */
	snprintf(temp_buf, sizeof(temp_buf), "%d.%02d C",
		 temp_celsius / 100, abs(temp_celsius % 100));
	snprintf(humid_buf, sizeof(humid_buf), "%d.%02d %%",
		 humidity_percent / 100, humidity_percent % 100);

	/* Display values to the right of the temp/humidity icon */
	cfb_print(display_dev, temp_buf, 70, 20);
	cfb_print(display_dev, humid_buf, 70, 40);

	/* Draw the temperature graph at the bottom */
	display_draw_graph();

	/* Finalize framebuffer - send everything to display at once */
	cfb_framebuffer_finalize(display_dev);

	LOG_INF("Updated: Temp=%s, Humidity=%s", temp_buf, humid_buf);
}

void display_update_battery(uint16_t voltage_mv, uint8_t percentage)
{
	char batt_buf[16];

	/* Format battery percentage only */
	snprintf(batt_buf, sizeof(batt_buf), "%d%%", percentage);

	/* Display percentage below the battery icon */
	/* Battery icon is at x=226, 24px wide, text centered below it */
	cfb_print(display_dev, batt_buf, 170, 15);

	cfb_framebuffer_finalize(display_dev);

	LOG_INF("Updated: Battery=%d%% (%d.%02dV)", percentage,
		voltage_mv / 1000, (voltage_mv % 1000) / 10);
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

int display_draw_image(const uint8_t *image_data, uint16_t x, uint16_t y,
                       uint16_t width, uint16_t height)
{
	int ret;

	if (!image_data) {
		LOG_ERR("Invalid image data pointer");
		return -EINVAL;
	}

	LOG_INF("Drawing %dx%d image at (%d,%d)", width, height, x, y);

	/* Draw bitmap pixel by pixel using CFB draw_point API */
	/* cfb_draw_point draws foreground pixels (black after inversion) */
	/* In source bitmap: 0 = black, 1 = white */
	struct cfb_position pos;

	for (uint16_t row = 0; row < height; row++) {
		/* Calculate source row offset */
		uint16_t src_row_offset = (row * width) / 8;

		for (uint16_t col = 0; col < width; col++) {
			/* Read pixel from source bitmap (row-major, MSB first per byte) */
			uint16_t src_byte = src_row_offset + (col / 8);
			uint8_t src_bit = 7 - (col % 8);
			uint8_t pixel = (image_data[src_byte] >> src_bit) & 0x01;

			/* Draw where we want black pixels (where source is 0) */
			if (pixel != 0) {
				continue;  /* Skip white pixels */
			}

			pos.x = x + col;
			pos.y = y + row;

			ret = cfb_draw_point(display_dev, &pos);
			if (ret != 0) {
				LOG_ERR("Failed to draw point at (%d,%d): %d", pos.x, pos.y, ret);
				return ret;
			}
		}
	}

	LOG_INF("Image drawn successfully");
	return 0;
}

void display_add_temp_reading(int16_t temp_celsius)
{
	/* Add temperature to circular buffer */
	temp_history[temp_history_index] = temp_celsius;
	temp_history_index = (temp_history_index + 1) % GRAPH_MAX_POINTS;

	if (temp_history_count < GRAPH_MAX_POINTS) {
		temp_history_count++;
	}

	LOG_DBG("Added temp reading: %d.%02d C (count=%d)",
		temp_celsius / 100, abs(temp_celsius % 100), temp_history_count);
}

void display_draw_graph(void)
{
	struct cfb_position pos;
	int ret;

	if (temp_history_count < 2) {
		LOG_DBG("Not enough data points to draw graph");
		return;  /* Need at least 2 points to draw a graph */
	}

	/* Clear the graph area by inverting it twice (or use framebuffer clear for region) */
	/* Since CFB doesn't have partial clear, we'll just overdraw */

	/* Find min and max temperature for scaling */
	int16_t min_temp = temp_history[0];
	int16_t max_temp = temp_history[0];

	for (uint8_t i = 0; i < temp_history_count; i++) {
		if (temp_history[i] < min_temp) {
			min_temp = temp_history[i];
		}
		if (temp_history[i] > max_temp) {
			max_temp = temp_history[i];
		}
	}

	/* Add some padding to min/max */
	int16_t temp_range = max_temp - min_temp;
	if (temp_range < 500) {  /* Minimum 5°C range */
		temp_range = 500;
		int16_t mid = (max_temp + min_temp) / 2;
		min_temp = mid - 250;
		max_temp = mid + 250;
	}

	LOG_INF("Drawing graph: min=%d.%02d, max=%d.%02d, points=%d",
		min_temp / 100, abs(min_temp % 100),
		max_temp / 100, abs(max_temp % 100),
		temp_history_count);

	/* Draw graph axes (border) */
	/* Top line */
	for (uint16_t x = GRAPH_X; x < GRAPH_X + GRAPH_WIDTH; x++) {
		pos.x = x;
		pos.y = GRAPH_Y;
		cfb_draw_point(display_dev, &pos);
	}

	/* Bottom line */
	for (uint16_t x = GRAPH_X; x < GRAPH_X + GRAPH_WIDTH; x++) {
		pos.x = x;
		pos.y = GRAPH_Y + GRAPH_HEIGHT - 1;
		cfb_draw_point(display_dev, &pos);
	}

	/* Left line */
	for (uint16_t y = GRAPH_Y; y < GRAPH_Y + GRAPH_HEIGHT; y++) {
		pos.x = GRAPH_X;
		pos.y = y;
		cfb_draw_point(display_dev, &pos);
	}

	/* Right line */
	for (uint16_t y = GRAPH_Y; y < GRAPH_Y + GRAPH_HEIGHT; y++) {
		pos.x = GRAPH_X + GRAPH_WIDTH - 1;
		pos.y = y;
		cfb_draw_point(display_dev, &pos);
	}

	/* Draw temperature line graph */
	uint16_t x_step = (GRAPH_WIDTH - 2) / (temp_history_count - 1);
	if (x_step == 0) {
		x_step = 1;
	}

	for (uint8_t i = 0; i < temp_history_count - 1; i++) {
		/* Calculate positions */
		uint16_t x1 = GRAPH_X + 1 + (i * x_step);
		uint16_t x2 = GRAPH_X + 1 + ((i + 1) * x_step);

		/* Scale temperature to graph height */
		int16_t temp1 = temp_history[i];
		int16_t temp2 = temp_history[i + 1];

		uint16_t y1 = GRAPH_Y + GRAPH_HEIGHT - 2 -
			      ((temp1 - min_temp) * (GRAPH_HEIGHT - 4) / temp_range);
		uint16_t y2 = GRAPH_Y + GRAPH_HEIGHT - 2 -
			      ((temp2 - min_temp) * (GRAPH_HEIGHT - 4) / temp_range);

		/* Draw line between points using Bresenham's algorithm */
		int16_t dx = abs(x2 - x1);
		int16_t dy = abs(y2 - y1);
		int16_t sx = (x1 < x2) ? 1 : -1;
		int16_t sy = (y1 < y2) ? 1 : -1;
		int16_t err = dx - dy;

		uint16_t x = x1;
		uint16_t y = y1;

		while (1) {
			pos.x = x;
			pos.y = y;
			ret = cfb_draw_point(display_dev, &pos);
			if (ret != 0) {
				LOG_ERR("Failed to draw graph point at (%d,%d)", x, y);
			}

			if (x == x2 && y == y2) {
				break;
			}

			int16_t e2 = 2 * err;
			if (e2 > -dy) {
				err -= dy;
				x += sx;
			}
			if (e2 < dx) {
				err += dx;
				y += sy;
			}
		}
	}

	LOG_INF("Graph drawn successfully");
}
