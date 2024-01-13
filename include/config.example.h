#define MEASUREMENT_INTERVAL_MS 30 * 1000
#define BRIGHTNESS_SAMPLE_COUNT 16
#define BRIGHTNESS_SAMPLE_DELAY_MS 50
#define TEMPERATURE_CORRECTION_C -6

// Depending on I2C device vendor, addresses might vary.
#define I2C_ADDR_BME280 0x76
#define I2C_ADDR_CCS811 0x5A
#define PIN_LIGHT_SENSOR 32

// These likely need customization:
#define MQTT_SERVER ""
#define WIFI_SSID ""
#define WIFI_PASSWORD ""