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

#ifndef SLEEP_TIME
#define SLEEP_TIME (5 * 60) /* in seconds; -1 to disable */
#endif

static saml21_extwake_t extwake = EXTWAKE;
static lora_state_t lora;
static h10_adc_t h10_adc_dev;

//  ----------------------------------------------------------------------
//                      START SENSOR SECTION
//
//  This Section Is Specific for The Selected Sensor
//      NODE-SENSEAIR
//  Sensor : Senseair Sunlight CO2
//

//  Includes
#include "fram.h"
#include "senseair.h"
#include "senseair_params.h"

//  Defines
#define NODE_PACKET_SIZE            NODE_SENSEAIR_SIZE
#define NODE_CLASS                  NODE_SENSEAIR_CLASS
#define SENSEAIR_STATE_FRAM_ADDR    (0)
#define USES_FRAM                   (1)

//  Variables
static senseair_t dev;
static senseair_abc_data_t abc_data;
static node_senseair node_data;

//  Sensor specific functions declaration
//
//  Read Sensor
void seaseair_sensor_read(void);
//  Format Sensor Data
void format_seaseair_info(int bln_hex, char *msg);

//  Sensor specific functions pointers
//
//  Read Sensor
void (*sensor_read_ptr)(void) = &seaseair_sensor_read;
//  Format Sensor Data
void (*format_info_ptr)(int, char *) = &format_seaseair_info;

//  Sensor specific functions code
//

//  Read Sensor
void seaseair_sensor_read(void)
{
    uint16_t conc_ppm;
    int16_t temperature;

    if (gpio_init(ACME_BUS_POWER_PIN, GPIO_OUT)) {
        puts("[ACME Bus] enable failed.");
        return;
    }
    gpio_set(ACME_BUS_POWER_PIN);

    if (senseair_init(&dev, &senseair_params[0]) != SENSEAIR_OK) {
        puts("[SENSOR Senseair] init failed.");
        gpio_clear(ACME_BUS_POWER_PIN);
        return;
    }

    memset(&abc_data, 0, sizeof(abc_data));
    if (fram_read(SENSEAIR_STATE_FRAM_ADDR, &abc_data, sizeof(abc_data))) {
        puts("[FRAM] read failed.");
    }
    else {
        if (senseair_write_abc_data(&dev, &abc_data) == SENSEAIR_OK) {
            puts("[SENSOR Senseair] ABC data restored to sensor.");
        }
        else {
            puts("[SENSOR Senseair] ABC data restoration failed.");
        }
    }

    if (senseair_read(&dev, &conc_ppm, &temperature) == SENSEAIR_OK) {
        printf("[SENSOR Data] Concentration: %d ppm\n", conc_ppm);
        printf("[SENSOR Data] Temperature: %4.2f °C\n", (temperature / 100.));
    }
    else {
        puts("[SENSOR Senseair] read data failed.");
    }

    if (senseair_read_abc_data(&dev, &abc_data) == SENSEAIR_OK) {
        puts("[SENSOR Senseair] Saving calibration data to FRAM.");
        if (fram_write(SENSEAIR_STATE_FRAM_ADDR, (uint8_t *)&abc_data, sizeof(abc_data))) {
            puts("[FRAM] write failed.");
        }
    }
    else {
        puts("[SENSOR Senseair] read abc data failed.");
    }



    node_data.conc_ppm = conc_ppm;
    node_data.temperature = temperature;

    gpio_clear(ACME_BUS_POWER_PIN);
}
//  Format Sensor Data
void format_seaseair_info(int bln_hex, char *msg)
{

    if (bln_hex) {
        sprintf(msg,
                "PPM : %04X, TEMP: %04X",
                node_data.conc_ppm, node_data.temperature
                );
    }
    else {
        sprintf(msg,
                "PPM : %05d, TEMP : %05d",
                node_data.conc_ppm, node_data.temperature
                );
    }
}

//
//                      END SENSOR SECTION
//  ----------------------------------------------------------------------

//  Inits and fills the lora structure with default values
void init_lora_setup(void)
{
    memset(&lora, 0, sizeof(lora));
    lora.bandwidth = DEFAULT_LORA_BANDWIDTH;
    lora.spreading_factor = DEFAULT_LORA_SPREADING_FACTOR;
    lora.coderate = DEFAULT_LORA_CODERATE;
    lora.channel = DEFAULT_LORA_CHANNEL;
    lora.power = DEFAULT_LORA_POWER;
    lora.boost = DEFAULT_LORA_BOOST;
}

//  Prints lora settings
void report_lora_setup(void)
{
    printf("LORA - Setup\n");
    printf("BW %d\n", lora.bandwidth);
    printf("SF %d\n", lora.spreading_factor);
    printf("CR %d\n", lora.coderate);
    printf("CH %ld\n", lora.channel);
    printf("PW %d\n", lora.power);
    printf("BS %d\n", lora.boost);

}

//  Reads and fills node fields with board voltages
void read_vcc_vpanel(void)
{
    int32_t vcc;
    int32_t vpanel;

    h10_adc_init(&h10_adc_dev, &h10_adc_params[0]);

    //  Super Cap
    vcc = h10_adc_read_vcc(&h10_adc_dev);
    node_data.header.vcc = (uint16_t)(vcc);
    printf("\n[BOARD vcc] READ: %5d mV\n", node_data.header.vcc);

    ztimer_sleep(ZTIMER_USEC, 100);

    //  Solar panel
    vpanel = h10_adc_read_vpanel(&h10_adc_dev);
    node_data.header.vpanel = (uint16_t)(vpanel);
    printf("[BOARD vpanel] READ: %5d mV\n", node_data.header.vpanel);

    h10_adc_deinit(&h10_adc_dev);
}

//  Clears node data structure and fills header fields
void set_node_header(void)
{
    //  Reset node structure
    memset(&node_data, 0, sizeof(node_data));

    //  Fill header
    node_data.header.signature = ACME_SIGNATURE;
    cpuid_get((void *)(node_data.header.cpuid));
    node_data.header.s_class = NODE_CLASS;
    read_vcc_vpanel();
    node_data.header.node_power = lora.power;
    node_data.header.node_boost = lora.boost;
    node_data.header.sleep_time = SLEEP_TIME;
}

//  Formats the string used to report header info
void format_header_info(int bln_hex, char *msg)
{
    char cpuid[CPUID_LEN * 2 + 1];

    fmt_bytes_hex(cpuid, node_data.header.cpuid, CPUID_LEN);
    cpuid[CPUID_LEN * 2] = 0;

    if (bln_hex) {
        sprintf(msg,
                "SIGNATURE: %04X, NODE_CLASS: %02X, CPUID: %s, VCC: %04X, VPANEL: %04X, BOOST: %01X, POWER: %02X, SLEEP: %04X",
                node_data.header.signature,
                node_data.header.s_class, cpuid,
                node_data.header.vcc, node_data.header.vpanel,
                node_data.header.node_boost, node_data.header.node_power, node_data.header.sleep_time
                );
    }
    else {
        sprintf(msg,
                "SIGNATURE: %04d, NODE_CLASS: %03d, CPUID: %s, VCC: %04d, VPANEL: %04d, BOOST: %01d, POWER: %03d, SLEEP: %05d",
                node_data.header.signature,
                node_data.header.s_class, cpuid,
                node_data.header.vcc, node_data.header.vpanel,
                node_data.header.node_boost, node_data.header.node_power, node_data.header.sleep_time
                );
    }
}

//  Transfers data in the node structure in a binary array to be transmitted,
void transmit_packet(void)
{
    char payload[NODE_PACKET_SIZE * 1];

    memset(&payload, 0, sizeof(payload));
    memcpy(payload, &node_data, NODE_PACKET_SIZE);

    if (lora_init(&(lora)) != 0) {
        return;
    }

    if (payload[0] == 0) {
        return;
    }

    iolist_t packet = {
        .iol_base = payload,
        .iol_len = NODE_PACKET_SIZE,
    };

    lora_write(&packet);
    puts("Lora transmit packet.");
    lora_off();
}

//
//  Performs all necessary operation to :
//  - Init lora parms
//  - Fill header fields
//  - Fill data fields
//  - Report packet - info
//  - Transmits packet
void sensor_read(void)
{
    //  Init lora parameters
    init_lora_setup();

    //  Fill packet header
    set_node_header();

    //  Read data from sensor
    (*sensor_read_ptr)();
    puts("");

#if ACME_DEBUG
    char infomsg[200];
    //  Report packet - info
    //  Header
    format_header_info(false, infomsg);
    printf("%s, ", infomsg);
    //  Data
    (*format_info_ptr)(false, infomsg);
    printf("%s\n", infomsg);

    //  Report packet - info - hex format
    format_header_info(true, infomsg);
    printf("%s, ", infomsg);
    (*format_info_ptr)(true, infomsg);
    printf("%s\n", infomsg);

    char payload_hex[NODE_PACKET_SIZE * 2 + 1];

    //  Report packet binary - hex format
    fmt_bytes_hex(payload_hex, (uint8_t *)&node_data, NODE_PACKET_SIZE);
    payload_hex[NODE_PACKET_SIZE * 2] = 0;
    puts(payload_hex);
#endif

    transmit_packet();
}

void wakeup_task(void)
{
    puts("Wakeup task.");
    sensor_read();
}

void periodic_task(void)
{
    puts("Periodic task.");
    sensor_read();
}

void boot_task(void)
{
    struct tm time;

    puts("Boot task.");
    rtc_localtime(0, &time);
    rtc_set_time(&time);
    fram_erase();
}

//  Main function
int main(void)
{
    //  Select wake up source
    switch (saml21_wakeup_cause()) {
    //  Push button
    case BACKUP_EXTWAKE:
        wakeup_task();
        break;
    //  Rtc
    case BACKUP_RTC:
        periodic_task();
        break;
    //  Power on
    default:
        boot_task();

        #if USES_FRAM
        fram_init();
        fram_erase();
        #endif

        init_lora_setup();
        lora_init(&(lora));  // needed to set the radio in order to have minimum power consumption
        lora_off();
        printf("\n");
        printf("-------------------------------------\n");
        printf("-    Test Node-SENSEAIR Berta-H10   -\n");
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
        boot_task();
        report_lora_setup();
        sensor_read();
        break;
    }

    puts("Entering backup mode.\n");
    saml21_backup_mode_enter(0, extwake, SLEEP_TIME, 1);
    // never reached
    return 0;
}
