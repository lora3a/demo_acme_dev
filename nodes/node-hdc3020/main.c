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
#define EXTWAKE { \
        .pin = EXTWAKE_PIN6, \
        .polarity = EXTWAKE_HIGH, \
        .flags = EXTWAKE_IN_PU }
#define SLEEP_TIME 10 /* in seconds; -1 to disable */

static saml21_extwake_t extwake = EXTWAKE;
static h10_adc_t h10_adc_dev;
static lora_state_t lora;

//  ----------------------------------------------------------------------
//                      START SENSOR SECTION
//
//  This Section Is Specific for The Selected Sensor
//      NODE-LIS2DW12
//  Sensor : STmicroelectronics LIS2DW12
//

//  Includes
#include "hdc3020.h"
#include "hdc3020_params.h"

//  Defines
#define NODE_PACKET_SIZE        NODE_HDC3020_SIZE
#define NODE_CLASS              NODE_HDC3020_CLASS
#define USES_FRAM               0

//  Variables
static hdc3020_t hdc3020;
static node_hdc3020 node_data;

//  Sensor specific functions declaration
//
//  Read Sensor
void hdc3020_sensor_read(void);
//  Format Sensor Data
void format_lis2dw12_info(int bln_hex, char *msg);

//  Sensor specific functions pointers
//
//  Read Sensor
void (*sensor_read_ptr)(void) = &hdc3020_sensor_read;
//  Format Sensor Data
void (*format_info_ptr)(int, char *) = &format_lis2dw12_info;



void hdc3020_sensor_read(void)
{
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
    printf("[SENSOR Data] Temperature : %5.2f, Humidity : %6.2f\n", temp, hum);
    hdc3020_deinit(&hdc3020);

    node_data.temperature = (int16_t)(temp * 100.0);
    node_data.humidity = (uint16_t)(hum * 100.0);
}

void format_hdc3020_info(int bln_hex, char *msg)
{
    if (bln_hex) {
    sprintf(msg,
        "TEMP: %05d, HUM: %05d\n",
        node_data.temperature, node_data.humidity);
    }
    else {
    sprintf(msg,
        "TEMP: %04X, HUM: %04X\n",
        node_data.temperature, node_data.humidity);
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
    printf("[BOARD vcc] READ: %5d\n", node_data.header.vcc);

    ztimer_sleep(ZTIMER_USEC, 100);

    //  Solar panel
    vpanel = h10_adc_read_vpanel(&h10_adc_dev);
    node_data.header.vpanel = (uint16_t)(vpanel);
    printf("[BOARD vpanel] READ: %5d\n", node_data.header.vpanel);

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
    char payload_hex[NODE_PACKET_SIZE * 2 + 1];
    char infomsg[200];

    //  Init lora parameters
    init_lora_setup();

    //  Fill packet header
    set_node_header();

    //  Read data from sensor
    (*sensor_read_ptr)();

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

    //  Report packet binary - hex format
    fmt_bytes_hex(payload_hex, (uint8_t *)&node_data, NODE_PACKET_SIZE);
    payload_hex[NODE_PACKET_SIZE * 2] = 0;
    puts(payload_hex);

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

        init_lora_setup();
        lora_init(&(lora));  // needed to set the radio in order to have minimum power consumption
        lora_off();
        printf("\n");
        printf("-------------------------------------\n");
        printf("-  Test Node-HDC3020 For Berta-H10  -\n");
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
        report_lora_setup();
        sensor_read();
        break;
    }
    puts("Entering backup mode.");
    saml21_backup_mode_enter(0, extwake, SLEEP_TIME, 1);
    // never reached
    return 0;
}
