#include <stdio.h>
#include <string.h>

#include "periph_conf.h"
#include "periph/i2c.h"
#include "periph/adc.h"
#include "periph/cpuid.h"
#include "fmt.h"

#include "acme_lora.h"
#include "saml21_backup_mode.h"

#include "nodes_data.h"
#include "h10_adc.h"
#include "h10_adc_params.h"
#include "bme680.h"
#include "bme680_params.h"

#define FW_VERSION "01.00.00"

/* use { .pin=EXTWAKE_NONE } to disable */
#define EXTWAKE \
    { .pin = EXTWAKE_PIN6, \
      .polarity = EXTWAKE_HIGH, \
      .flags = EXTWAKE_IN_PU \
    }
#define SLEEP_TIME 10 /* in seconds; -1 to disable */


static saml21_extwake_t extwake = EXTWAKE;
static bme680_t bme688_dev;
static h10_adc_t h10_adc_dev;

static node_BME688 node_data;

static int16_t _temp;
static int16_t _press;
static int16_t _hum;
static uint32_t _gas;

static lora_state_t lora;

void read_vcc_vpanel(void)
{
    h10_adc_init(&h10_adc_dev, &h10_adc_params[0]);

    int32_t vcc = h10_adc_read_vcc(&h10_adc_dev);

    node_data.header.vcc = (uint16_t)(vcc);
    printf("[SENSOR vcc] READ: %5d\n", node_data.header.vcc);

    ztimer_sleep(ZTIMER_USEC, 100);

    int32_t vpanel = h10_adc_read_vpanel(&h10_adc_dev);

    node_data.header.vpanel = (uint16_t)(vpanel);
    printf("[SENSOR vpanel] READ: %5d\n", node_data.header.vpanel);

    h10_adc_deinit(&h10_adc_dev);
}

void bme688_sensor_read(void)
{
    char payload[NODE_BME688_SIZE * 1];

    memset(&payload, 0, sizeof(payload));
    memset(&lora, 0, sizeof(lora));

    lora.bandwidth = DEFAULT_LORA_BANDWIDTH;
    lora.spreading_factor = DEFAULT_LORA_SPREADING_FACTOR;
    lora.coderate = DEFAULT_LORA_CODERATE;
    lora.channel = DEFAULT_LORA_CHANNEL;
    lora.power = DEFAULT_LORA_POWER;
    lora.boost = DEFAULT_LORA_BOOST;

    node_data.header.signature = ACME_SIGNATURE;
    cpuid_get((void *)(node_data.header.cpuid));

    node_data.header.s_class = NODE_BME688_CLASS;

    node_data.header.node_boost = lora.boost;
    node_data.header.node_power = lora.power;
    node_data.header.sleep_time = SLEEP_TIME;

    int duration;

    if (bme680_init(&bme688_dev, &bme680_params[0]) != BME680_OK) {
        puts("[ERROR] Initialization of BME688.");
        return;
    }

    if (bme680_force_measurement(&bme688_dev) != BME680_OK) {
        puts("[ERROR] Forse Measurement.");
        return;
    }

    if ((duration = bme680_get_duration(&bme688_dev)) < 0) {
        puts("[ERROR] Get Duration.");
        return;
    }
    ztimer_sleep(ZTIMER_MSEC, duration);

    bme680_field_data_t data;

    if (bme680_get_data(&bme688_dev, &data) != BME680_OK) {
        puts("[ERROR] Get Data.");
        return;
    }

#if MODULE_BME680_FP
    _temp = data.temperature * 100;
    _press = data.pressure / 100;
    _hum = data.humidity * 100;
#else
    _temp = data.temperature;
    _press = data.pressure / 100;
    _hum = data.humidity / 10;
#endif
    _gas = (data.status & BME680_GASM_VALID_MSK) ? data.gas_resistance : 0;

    printf("[DATA] TEMP: %.2f °C, PRESS: %d Pa, HUM: %.1f %%, GAS: %ld Ohm.\n",
           _temp / 100., _press, _hum / 100., _gas);

    node_data.temperature = _temp;
    node_data.humidity = _hum;
    node_data.pressure = _press;
    node_data.gas = _gas;

    read_vcc_vpanel();

    char cpuid[CPUID_LEN * 2 + 1];
    memcpy(payload, &node_data, NODE_BME688_SIZE);
    fmt_bytes_hex(cpuid, node_data.header.cpuid, CPUID_LEN);
    cpuid[CPUID_LEN * 2] = 0;

    char payload_hex[NODE_BME688_SIZE * 2 + 1];

    #define PAYLOAD_HEX_SIZE sizeof(payload_hex)
    fmt_bytes_hex(payload_hex, (uint8_t *)&node_data, NODE_BME688_SIZE);
    payload_hex[NODE_BME688_SIZE * 2] = 0;

    printf(
        "SIGNATURE: %04d, NODE_CLASS: %02d, CPUID: %s, VCC: %04d, VPANEL: %04d, BOOST: %01d, POWER: %02d, SLEEP: %05d, TEMP: %05d, HUM: %05d, PRESS: %4d, GAS: % 9ld.\n",
        node_data.header.signature,
        node_data.header.s_class, cpuid,
        node_data.header.vcc, node_data.header.vpanel,
        node_data.header.node_boost, node_data.header.node_power, node_data.header.sleep_time,
        node_data.temperature, node_data.humidity,
        node_data.pressure, node_data.gas
        );

    printf(
        "SIGNATURE: %04X, NODE_CLASS: %02X, CPUID: %s, VCC: %04X, VPANEL: %04X, BOOST: %01X, POWER: %02X, SLEEP: %04X, TEMP: %04X, HUM: %04X, PRESS: %X, GAS: %lX.\n",
        node_data.header.signature,
        node_data.header.s_class, cpuid,
        node_data.header.vcc, node_data.header.vpanel,
        node_data.header.node_boost, node_data.header.node_power, node_data.header.sleep_time,
        node_data.temperature, node_data.humidity,
        node_data.pressure, node_data.gas
        );

    puts(payload_hex);

    if (lora_init(&(lora)) != 0) {
        return;
    }

    if (payload[0] == 0) {
        return;
    }

    iolist_t packet = {
        .iol_base = payload,
        .iol_len = NODE_BME688_SIZE,
    };

    lora_write(&packet);
    puts("Lora send command.");
    lora_off();
}

void wakeup_task(void)
{
    puts("Wakeup task.");
    bme688_sensor_read();
}

void periodic_task(void)
{
    puts("Periodic task.");
    bme688_sensor_read();
}

int main(void)
{
    switch (saml21_wakeup_cause()) {
    case BACKUP_EXTWAKE:
        wakeup_task();
        break;
    case BACKUP_RTC:
        periodic_task();
        break;
    default:
        lora_init(&(lora));  // needed to set the radio in order to have minimum power consumption
        lora_off();
        printf("\n");
        printf("-------------------------------------\n");
        printf("-      Test BME688 For Berta-H10    -\n");
        printf("-           by Acme Systems         -\n");
        printf("-  Version  : %s              -\n", FW_VERSION);
        printf("-  Compiled : %s %s  -\n", __DATE__, __TIME__);
        printf("-------------------------------------\n");
        printf("\n");

        if (extwake.pin != EXTWAKE_NONE) {
            printf("PUSH BUTTON P2 will wake the board.\n");
        }
        if (SLEEP_TIME > -1) {
            printf("Periodic task running every %d seconds.\n", SLEEP_TIME);
        }

        bme688_sensor_read();
        break;
    }

    puts("Entering backup mode.");
    saml21_backup_mode_enter(0, extwake, SLEEP_TIME, 1);
    // never reached
    return 0;
}
