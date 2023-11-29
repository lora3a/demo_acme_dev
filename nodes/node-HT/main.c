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
#include "hdc3020.h"
#include "hdc3020_params.h"

/* use { .pin=EXTWAKE_NONE } to disable */
#define EXTWAKE { \
        .pin = EXTWAKE_PIN6, \
        .polarity = EXTWAKE_HIGH, \
        .flags = EXTWAKE_IN_PU }
#define SLEEP_TIME 30 /* in seconds; -1 to disable */

static saml21_extwake_t extwake = EXTWAKE;
static hdc3020_t hdc3020;
static h10_adc_t h10_adc_dev;

static node_HT node_data;


void sensors_read(void)
{
    char payload[NODE_HT_SIZE * 1];
    lora_state_t lora;

    memset(&payload, 0, sizeof(payload));

    node_data.header.signature = ACME_SIGNATURE;
    cpuid_get((void *)(node_data.header.cpuid));

    node_data.header.s_class = NODE_HT_CLASS;

    double temp;
    double hum;

    if (hdc3020_init(&hdc3020, hdc3020_params) != HDC3020_OK) {
        puts("[SENSOR hdc3020] FAIL INIT.");
        return;
    }
    if (hdc3020_read(&hdc3020, &temp, &hum) != HDC3020_OK) {
        puts("[SENSOR hdc3020] FAIL READ.");
        return;
    }
    puts("[SENSOR hdc3020] READ.");
    hdc3020_deinit(&hdc3020);

    node_data.temperature = (int16_t)(temp * 100.0);
    node_data.humidity = (uint16_t)(hum * 100.0);

    h10_adc_init(&h10_adc_dev, &h10_adc_params[0]);

    int32_t vcc = h10_adc_read_vcc(&h10_adc_dev);

    node_data.header.vcc = (uint16_t)(vcc);
    puts("[SENSOR vcc] READ.");

    ztimer_sleep(ZTIMER_MSEC, 10);

    int32_t vpanel = h10_adc_read_vcc(&h10_adc_dev);

    node_data.header.vpanel = (uint16_t)(vpanel);
    puts("[SENSOR vpanel] READ.");

    h10_adc_deinit(&h10_adc_dev);

    memcpy(payload, &node_data, NODE_HT_SIZE);

    char cpuid[CPUID_LEN * 2 + 1];

    fmt_bytes_hex(cpuid, node_data.header.cpuid, CPUID_LEN);
    cpuid[CPUID_LEN * 2] = 0;

    char payload_hex[NODE_HT_SIZE * 2 + 1];

    #define PAYLOAD_HEX_SIZE sizeof(payload_hex)
    fmt_bytes_hex(payload_hex, (uint8_t *)&node_data, NODE_HT_SIZE);
    payload_hex[NODE_HT_SIZE * 2] = 0;

    printf(
        "SIGNATURE: %04d, NODE_CLASS: %02d, CPUID: %s, VCC: %04d, VPANEL: %04d, TEMP: %05d, HUM: %05d\n",
        node_data.header.signature,
        node_data.header.s_class, cpuid,
        node_data.header.vcc, node_data.header.vpanel,
        node_data.temperature, node_data.humidity);

    printf(
        "SIGNATURE: %04x, NODE_CLASS: %02x, CPUID: %s, VCC: %04x, VPANEL: %04x, TEMP: %04x, HUM: %04x\n",
        node_data.header.signature,
        node_data.header.s_class, cpuid,
        node_data.header.vcc, node_data.header.vpanel,
        node_data.temperature, node_data.humidity);

    puts(payload_hex);


    memset(&lora, 0, sizeof(lora));
    lora.bandwidth = DEFAULT_LORA_BANDWIDTH;
    lora.spreading_factor = DEFAULT_LORA_SPREADING_FACTOR;
    lora.coderate = DEFAULT_LORA_CODERATE;
    lora.channel = DEFAULT_LORA_CHANNEL;
    lora.power = DEFAULT_LORA_POWER;
    lora.boost = 1;


    if (lora_init(&(lora)) != 0) {
        return;
    }

    if (payload[0] == 0) {
        return;
    }

    iolist_t packet = {
        .iol_base = payload,
        .iol_len = NODE_HT_SIZE
    };

    lora_write(&packet);
    puts("Lora send command.");
    lora_off();
}

void wakeup_task(void)
{
    puts("Wakeup task.");
    sensors_read();
}

void periodic_task(void)
{
    puts("Periodic task.");
    sensors_read();
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
        if (extwake.pin != EXTWAKE_NONE) {
            printf("GPIO PA%d will wake the board.\n", extwake.pin);
        }
        if (SLEEP_TIME > -1) {
            printf("Periodic task running every %d seconds.\n", SLEEP_TIME);
        }
        break;
    }

    puts("Entering backup mode.");
    saml21_backup_mode_enter(0, extwake, SLEEP_TIME, 1);
    // never reached
    return 0;
}
