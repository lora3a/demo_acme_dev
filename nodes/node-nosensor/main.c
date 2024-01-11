#include <stdio.h>
#include <string.h>

#include "periph_conf.h"
#include "periph/i2c.h"
#include "periph/adc.h"
#include "periph/cpuid.h"
#include "periph/rtc.h"
#include "rtc_utils.h"
#include "fmt.h"

#include "acme_lora.h"
#include "saml21_backup_mode.h"

#include "nodes_data.h"
#include "h10_adc.h"
#include "h10_adc_params.h"

#define FW_VERSION "01.00.00"

/* use { .pin=EXTWAKE_NONE } to disable */
#define EXTWAKE \
    { .pin = EXTWAKE_PIN6, \
      .polarity = EXTWAKE_HIGH, \
      .flags = EXTWAKE_IN_PU \
    }
#define SLEEP_TIME (5 * 60) /* in seconds; -1 to disable */

#define USES_FRAM               0


static saml21_extwake_t extwake = EXTWAKE;
static h10_adc_t h10_adc_dev;

static node_nosensor node_data;

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

void voltages_read(void)
{
    char payload[NODE_NOSENSOR_SIZE * 1];

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

    node_data.header.s_class = NODE_NOSENSOR_CLASS;

    node_data.header.node_power = lora.power;
    node_data.header.node_boost = lora.boost;
    node_data.header.sleep_time = SLEEP_TIME;

    read_vcc_vpanel();

    char cpuid[CPUID_LEN * 2 + 1];

    memcpy(payload, &node_data, NODE_NOSENSOR_SIZE);
    fmt_bytes_hex(cpuid, node_data.header.cpuid, CPUID_LEN);
    cpuid[CPUID_LEN * 2] = 0;

    char payload_hex[NODE_NOSENSOR_SIZE * 2 + 1];

    #define PAYLOAD_HEX_SIZE sizeof(payload_hex)
    fmt_bytes_hex(payload_hex, (uint8_t *)&node_data, NODE_NOSENSOR_SIZE);
    payload_hex[NODE_NOSENSOR_SIZE * 2] = 0;

    printf(
        "SIGNATURE: %04d, NODE_CLASS: %02d, CPUID: %s, VCC: %04d, VPANEL: %04d, BOOST: %01d, POWER: %02d, SLEEP: %05d.\n",
        node_data.header.signature,
        node_data.header.s_class, cpuid,
        node_data.header.vcc, node_data.header.vpanel,
        node_data.header.node_boost, node_data.header.node_power, node_data.header.sleep_time
        );

    printf(
        "SIGNATURE: %04X, NODE_CLASS: %02X, CPUID: %s, VCC: %04X, VPANEL: %04X, BOOST: %01X, POWER: %02X, SLEEP: %04X.\n",
        node_data.header.signature,
        node_data.header.s_class, cpuid,
        node_data.header.vcc, node_data.header.vpanel,
        node_data.header.node_boost, node_data.header.node_power, node_data.header.sleep_time
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
        .iol_len = NODE_NOSENSOR_SIZE,
    };

    lora_write(&packet);
    puts("Lora send command.");
    lora_off();
}

void wakeup_task(void)
{
    puts("Wakeup task.");
    voltages_read();
}

void periodic_task(void)
{
    puts("Periodic task.");
    voltages_read();
}

void boot_task(void)
{
    struct tm time;

    puts("Boot task.");
    rtc_localtime(0, &time);
    rtc_set_time(&time);
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
        boot_task();

        #if USES_FRAM
        fram_init();
        fram_erase();
        #endif

        lora_init(&(lora));  // needed to set the radio in order to have minimum power consumption
        lora_off();
        printf("\n");
        printf("--------------------------------------\n");
        printf("- Test without sensors For Berta-H10 -\n");
        printf("-           by Acme Systems          -\n");
        printf("-  Version  : %s               -\n", FW_VERSION);
        printf("-  Compiled : %s %s   -\n", __DATE__, __TIME__);
        printf("--------------------------------------\n");
        printf("\n");

        if (extwake.pin != EXTWAKE_NONE) {
            printf("PUSH BUTTON P2 will wake the board.\n");
        }
        if (SLEEP_TIME > -1) {
            printf("Periodic task running every %d seconds.\n", SLEEP_TIME);
        }
        voltages_read();
        break;
    }

    puts("Entering backup mode.");
    saml21_backup_mode_enter(0, extwake, SLEEP_TIME, 1);
    // never reached
    return 0;
}
