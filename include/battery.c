#include "battery.h"
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(battery, LOG_LEVEL_INF);

/* ADC settings for nRF52840 */
#define ADC_DEVICE_NODE DT_NODELABEL(adc)
#define ADC_RESOLUTION 12  /* 12-bit resolution */
#define ADC_GAIN ADC_GAIN_1_6
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_CHANNEL_ID 7  /* Use channel 7 for AIN7 (P0.31) */

/* XIAO nRF52840 battery monitoring pins */
#define VBAT_ENABLE_PIN 14  /* P0.14 - enables voltage divider when LOW */
#define VBAT_ADC_PIN 31     /* P0.31 - AIN7 - battery voltage input */

/* Voltage divider: 1M + 510k resistors = (1000 + 510) / 510 ≈ 2.96 */
#define VBAT_DIVIDER_NUMERATOR 1510
#define VBAT_DIVIDER_DENOMINATOR 510

/* Calibration factor to compensate for ADC/resistor tolerances */
/* Adjust this based on multimeter readings: (actual_voltage / measured_voltage) * 1000 */
#define VBAT_CALIBRATION_FACTOR 1029  /* 1.029 * 1000 - adjusted for 4.00V target */

/* Moving average filter settings */
#define VBAT_SAMPLE_COUNT 8  /* Number of samples to average */
static uint16_t voltage_samples[VBAT_SAMPLE_COUNT];
static uint8_t sample_index = 0;
static bool samples_filled = false;

static const struct device *adc_dev;

static struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_CHANNEL_ID,
	.input_positive = SAADC_CH_PSELP_PSELP_AnalogInput7,  /* P0.31 / AIN7 */
};

static int16_t sample_buffer;
static struct adc_sequence sequence = {
	.buffer = &sample_buffer,
	.buffer_size = sizeof(sample_buffer),
	.resolution = ADC_RESOLUTION,
};

int battery_init(void)
{
	int ret;
	const struct device *gpio_dev;

	/* Get GPIO device for P0.14 (VBAT_ENABLE) */
	gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	if (!device_is_ready(gpio_dev)) {
		LOG_ERR("GPIO device not ready");
		return -ENODEV;
	}

	/* Configure P0.14 as output and set LOW to enable voltage divider */
	ret = gpio_pin_configure(gpio_dev, VBAT_ENABLE_PIN, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Failed to configure VBAT_ENABLE pin: %d", ret);
		return ret;
	}

	/* Set P0.14 LOW to enable battery voltage reading */
	ret = gpio_pin_set(gpio_dev, VBAT_ENABLE_PIN, 0);
	if (ret != 0) {
		LOG_ERR("Failed to set VBAT_ENABLE LOW: %d", ret);
		return ret;
	}

	/* Initialize ADC */
	adc_dev = DEVICE_DT_GET(ADC_DEVICE_NODE);
	if (!device_is_ready(adc_dev)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	ret = adc_channel_setup(adc_dev, &channel_cfg);
	if (ret != 0) {
		LOG_ERR("ADC channel setup failed: %d", ret);
		return ret;
	}

	/* Set channel mask for the sequence */
	sequence.channels = BIT(ADC_CHANNEL_ID);

	LOG_INF("Battery monitoring initialized (P0.31/AIN7)");
	return 0;
}

uint16_t battery_read_voltage(void)
{
	int ret;
	int32_t val_mv;
	int32_t adc_voltage;
	uint32_t average_voltage = 0;

	ret = adc_read(adc_dev, &sequence);
	if (ret != 0) {
		LOG_ERR("ADC read failed: %d", ret);
		return 0;
	}

	LOG_INF("ADC raw value: %d", sample_buffer);

	/* Convert ADC value to millivolts */
	/* With internal reference (0.6V) and gain 1/6: */
	/* ADC voltage = sample * (600mV * 6) / 4096 */
	val_mv = sample_buffer;
	adc_voltage = (val_mv * 600 * 6) / 4096;

	LOG_INF("ADC voltage (before divider): %d mV", adc_voltage);

	/* Apply voltage divider ratio to get actual battery voltage */
	/* Battery voltage = ADC voltage * (1510 / 510) */
	val_mv = (adc_voltage * VBAT_DIVIDER_NUMERATOR) / VBAT_DIVIDER_DENOMINATOR;

	/* Apply calibration factor */
	val_mv = (val_mv * VBAT_CALIBRATION_FACTOR) / 1000;

	LOG_INF("Battery voltage (before averaging): %d mV", val_mv);

	/* Store sample in circular buffer */
	voltage_samples[sample_index] = (uint16_t)val_mv;
	sample_index = (sample_index + 1) % VBAT_SAMPLE_COUNT;

	/* Mark buffer as filled after first full cycle */
	if (sample_index == 0) {
		samples_filled = true;
	}

	/* Calculate moving average */
	uint8_t samples_to_average = samples_filled ? VBAT_SAMPLE_COUNT : sample_index;
	if (samples_to_average == 0) {
		samples_to_average = 1;  /* At least use the current sample */
	}

	for (uint8_t i = 0; i < samples_to_average; i++) {
		average_voltage += voltage_samples[i];
	}
	average_voltage /= samples_to_average;

	LOG_INF("Battery voltage (after averaging %d samples): %d mV",
		samples_to_average, average_voltage);

	return (uint16_t)average_voltage;
}

uint8_t battery_get_percentage(uint16_t mv)
{
    // Li-ion voltage curve (4.20V to 3.30V)
    const uint16_t voltage_table[] = {
        4200, 4100, 4000, 3900, 3800, 3700, 3600, 3500, 3400, 3300
    };

    const uint8_t percent_table[] = {
        100,  96,   90,   80,   60,   40,   25,   10,    5,    0
    };

    // Above max → 100%
    if (mv >= 4000) return 100;
    // Below min → 0%
    if (mv <= 3300) return 0;

    // Piecewise linear interpolation
    for (int i = 0; i < 9; i++) {
        if (mv >= voltage_table[i+1]) {
            uint16_t v1 = voltage_table[i];
            uint16_t v2 = voltage_table[i+1];
            uint8_t p1 = percent_table[i];
            uint8_t p2 = percent_table[i+1];

            // Linear interpolation between table points
            return p1 - ((p1 - p2) * (v1 - mv) / (v1 - v2));
        }
    }

    return 0; // should not happen
}

