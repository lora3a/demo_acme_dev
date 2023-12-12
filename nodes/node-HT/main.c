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

#define FW_VERSION "01.00.00"

/* use { .pin=EXTWAKE_NONE } to disable */
#define EXTWAKE { \
        .pin = EXTWAKE_PIN6, \
        .polarity = EXTWAKE_HIGH, \
        .flags = EXTWAKE_IN_PU }
#define SLEEP_TIME 10 /* in seconds; -1 to disable */

static saml21_extwake_t extwake = EXTWAKE;
static hdc3020_t hdc3020;
static h10_adc_t h10_adc_dev;
static lora_state_t lora;

static node_HT node_data;

void read_vcc_vpanel(h10_adc_t *dev)
{
    h10_adc_init(dev, &h10_adc_params[0]);

    int32_t vcc = h10_adc_read_vcc(dev);

    node_data.header.vcc = (uint16_t)(vcc);
    printf("[SENSOR vcc] READ: %5d\n", node_data.header.vcc);

    ztimer_sleep(ZTIMER_USEC, 100);

    int32_t vpanel = h10_adc_read_vpanel(dev);

    node_data.header.vpanel = (uint16_t)(vpanel);
    printf("[SENSOR vpanel] READ: %5d\n", node_data.header.vpanel);

    h10_adc_deinit(dev);
}

void node_HT_print(const node_HT *node)
{
    char cpu_id[CPUID_LEN * 2 + 1];

    fmt_bytes_hex(cpu_id, node->header.cpuid, CPUID_LEN);
    cpu_id[CPUID_LEN * 2] = 0;

    printf(
        "SIGNATURE: %04d, NODE_CLASS: %02d, CPUID: %s, VCC: %04d, VPANEL: %04d, BOOST: %01d, POWER: %02d, SLEEP: %05d, TEMP: %05d, HUM: %05d\n",
        node_data.header.signature,
        node_data.header.s_class, cpu_id,
        node_data.header.vcc, node_data.header.vpanel,
        node_data.header.node_boost, node_data.header.node_power, node_data.header.sleep_time,
        node_data.temperature, node_data.humidity);

    printf(
        "SIGNATURE: %04X, NODE_CLASS: %02X, CPUID: %s, VCC: %04X, VPANEL: %04X, BOOST: %01X, POWER: %02X, SLEEP: %04X, TEMP: %04X, HUM: %04X\n",
        node_data.header.signature,
        node_data.header.s_class, cpu_id,
        node_data.header.vcc, node_data.header.vpanel,
        node_data.header.node_boost, node_data.header.node_power, node_data.header.sleep_time,
        node_data.temperature, node_data.humidity);

    char payload_hex[NODE_HT_SIZE * 2 + 1];

    fmt_bytes_hex(payload_hex, (uint8_t *)node, NODE_HT_SIZE);
    payload_hex[NODE_HT_SIZE * 2] = 0;

    printf("[HEX DATA] %s.\n", payload_hex);
}

void hdc3020_sensor_read(hdc3020_t *dev, node_HT *node_data)
{
    double temp;
    double hum;

    if (hdc3020_init(dev, hdc3020_params) != HDC3020_OK) {
        puts("[SENSOR hdc3020] FAIL INIT.");
        return;
    }
    if (hdc3020_read(dev, &temp, &hum) != HDC3020_OK) {
        puts("[SENSOR hdc3020] FAIL READ.");
        return;
    }
    puts("[SENSOR hdc3020] READ.");
    hdc3020_deinit(dev);

    node_data->temperature = (int16_t)(temp * 100.0);
    node_data->humidity = (uint16_t)(hum * 100.0);
}

void sensors_read(void)
{
    char payload[NODE_HT_SIZE * 1];

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
    node_data.header.s_class = NODE_HT_CLASS;
    node_data.header.node_power = lora.power;
    node_data.header.node_boost = lora.boost;
    node_data.header.sleep_time = SLEEP_TIME;

    hdc3020_sensor_read(&hdc3020, &node_data);

    read_vcc_vpanel(&h10_adc_dev);

    node_HT_print(&node_data);

    memcpy(payload, &node_data, NODE_HT_SIZE);

    if (lora_init(&(lora)) != 0) {
        return;
    }

    if (payload[0] == 0) {
        return;
    }

    iolist_t packet = {
        .iol_base = payload,
        .iol_len = NODE_NOSENSOR_SIZE,
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
        lora_init(&(lora));  // needed to set the radio in order to have minimum power consumption
        lora_off();
        printf("\n");
        printf("-------------------------------------\n");
        printf("-    Test Hum Temp For Berta-H10    -\n");
        printf("-           by Acme Systems         -\n");
        printf("-  Version  : %s              -\n", FW_VERSION);
        printf("-  Compiled : %s %s  -\n", __DATE__, __TIME__);
        printf("-------------------------------------\n");
        printf("\n");

        if (extwake.pin != EXTWAKE_NONE) {
            printf("GPIO PA%d will wake the board.\n", extwake.pin);
        }
        if (SLEEP_TIME > -1) {
            printf("Periodic task running every %d seconds.\n", SLEEP_TIME);
        }
        sensors_read();
        break;
    }
    puts("Entering backup mode.");
    saml21_backup_mode_enter(0, extwake, SLEEP_TIME, 1);
    // never reached
    return 0;
}
