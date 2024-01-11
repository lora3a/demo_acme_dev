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
//      NODE-LIS2DW12-DS18B20
//  Sensor : STmicroelectronics LIS2DW12, DALLAS DS18B20
//

//  Includes
#include "lis2dw12.h"
#include "lis2dw12_params.h"
#include "lis2dw12_util.h"
#include "ds18.h"
#include "ds18_params.h"


//  Defines
#define NODE_PACKET_SIZE        NODE_LIS2DW12_DS18B20_SIZE
#define NODE_CLASS              NODE_LIS2DW12_DS18B20_CLASS
#define SCALE_ACCELEROMETER     1.0
#define USES_FRAM               0
#define SENSOR_1_POWER_PIN      LIS2DW12_POWER_PIN
#define SENSOR_2_POWER_PIN      DS18_PWR_PIN

//  Variables
static lis2dw12_t lis2dw12;
static ds18_t ds18;
static node_lis2dw12_ds18b20 node_data;
static accelerometer acc;
static rotation_matrix rot_matrix;


//  Sensor specific functions declaration
//
//  Read Sensor
void lis2dw12_ds18b20_sensor_read(void);
//  Format Sensor Data
void format_lis2dw12_ds18b20_info(int bln_hex, char *msg);

//  Sensor specific functions pointers
//
//  Read Sensor
void (*sensor_read_ptr)(void) = &lis2dw12_ds18b20_sensor_read;
//  Format Sensor Data
void (*format_info_ptr)(int, char *) = &format_lis2dw12_ds18b20_info;

//  Sensor specific functions code
//

//  Read Sensor
void lis2dw12_ds18b20_sensor_read(void)
{
    int16_t temperature;

    if (lis2dw12_init(&lis2dw12, &lis2dw12_params[0]) != LIS2DW12_OK) {
        puts("[SENSOR lis2dw12] INIT FAILED.");
        return;
    }
    ztimer_sleep(ZTIMER_MSEC, 20);

    //  Dummy read to be discarded - to be checked on Datasheet
    if (lis2dw12_read(&lis2dw12, &acc.x_mg, &acc.y_mg, &acc.z_mg, &acc.t_c) != LIS2DW12_OK) {
        puts("[SENSOR lis2dw12] READ FAILED.");
        return;
    }

    //ztimer_sleep(ZTIMER_MSEC, 200);
    if (lis2dw12_read(&lis2dw12, &acc.x_mg, &acc.y_mg, &acc.z_mg, &acc.t_c) != LIS2DW12_OK) {
        puts("[SENSOR lis2dw12] READ FAILED.");
        return;
    }

    float g_total = accelerometer_magnitude(&acc);

    calculate_rotation(&rot_matrix, &acc, g_total);

    printf(
        "[SENSOR Data] X: %.3f mg, Y: %.3f mg, Z: %.3f mg, Temperature: %.2f °C\n",
        acc.x_mg, acc.y_mg, acc.z_mg,
        acc.t_c);

    rotation_matrix_print(&rot_matrix);

    node_data.acc_x = (int16_t)(acc.x_mg * SCALE_ACCELEROMETER);
    node_data.acc_y = (int16_t)(acc.y_mg * SCALE_ACCELEROMETER);
    node_data.acc_z = (int16_t)(acc.z_mg * SCALE_ACCELEROMETER);
    node_data.temp_lis2dw12 = (int16_t)(acc.t_c * 100.);
    node_data.pitch = (int16_t)(rot_matrix.pitch * 100);
    node_data.roll = (int16_t)(rot_matrix.roll * 100);
    node_data.yaw = (int16_t)(rot_matrix.yaw * 100);

    gpio_init(DS18_PWR_PIN, GPIO_OUT);
    gpio_set(DS18_PWR_PIN);

    if (ds18_init(&ds18, &ds18_params[0]) != DS18_OK) {
        puts("[SENSOR ds18b20] FAIL INIT.");
        return;
    }

    if (ds18_get_temperature(&ds18, &temperature) != DS18_OK) {
        puts("[SENSOR ds18b20] FAIL READ.");
        return;
    }

    gpio_clear(DS18_PWR_PIN);

    printf("[SENSOR DS18B20] Temperature: %.2f °C\n", temperature / 100.);

    node_data.temp_ds18b20 = temperature;


}

//  Format Sensor Data
void format_lis2dw12_ds18b20_info(int bln_hex, char *msg)
{

    if (bln_hex) {
        sprintf(msg,
                "ACC X: %04X, ACC Y: %04X, ACC Z: %4X, TEMP: %04X, PITCH: %04X, ROLL: %04X, YAW: %04X, TEMP: %04X",
                node_data.acc_x, node_data.acc_y, node_data.acc_z, node_data.temp_lis2dw12,
                node_data.pitch, node_data.roll, node_data.yaw,
                node_data.temp_ds18b20
                );
    }
    else {
        sprintf(msg,
                "ACC X: %05d, ACC Y: %05d, ACC Z: %5d, TEMP: %05d, PITCH: % 3.2f, ROLL: % 3.2f, YAW: % 3.2f, TEMP: %05d",
                node_data.acc_x, node_data.acc_y, node_data.acc_z, node_data.temp_lis2dw12,
                node_data.pitch / 100., node_data.roll / 100., node_data.yaw / 100.,
                node_data.temp_ds18b20
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

    //  Switch off sensors
    gpio_clear(SENSOR_1_POWER_PIN);
    gpio_clear(SENSOR_2_POWER_PIN);

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
        printf("-  Node-LIS2DW12-DS18B20 Berta-H10  -\n");
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

        report_lora_setup();
        sensor_read();
        break;
    }

    puts("Entering backup mode.\n");

    saml21_backup_mode_enter(0, extwake, SLEEP_TIME, 1);
    // never reached
    return 0;
}
