#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "periph/cpuid.h"

struct node_header {
    uint16_t signature;
    uint8_t s_class;

    uint8_t cpuid[CPUID_LEN];

    uint16_t vcc;
    uint16_t vpanel;
    int8_t node_power;
    uint8_t node_boost;
    uint16_t sleep_time;
};

typedef struct node_header node_header;
#define NODE_HEADER_SIZE sizeof(node_header)

// Node without sensors.
struct node_nosensor {
    node_header header;
};

typedef struct node_nosensor node_nosensor;
#define NODE_NOSENSOR_CLASS 0
#define NODE_NOSENSOR_SIZE sizeof(node_nosensor)

//
struct node_hdc3020 {
    node_header header;

    int16_t temperature;
    uint16_t humidity;
};

typedef struct node_hdc3020 node_hdc3020;
#define NODE_HDC3020_CLASS 1
#define NODE_HDC3020_SIZE sizeof(node_hdc3020)

//
struct node_lis2dw12 {
    node_header header;

    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t temperature;
};

typedef struct node_lis2dw12 node_lis2dw12;
#define NODE_LIS2DW12_CLASS 2
#define NODE_LIS2DW12_SIZE sizeof(node_lis2dw12)

//
struct node_senseair {
    node_header header;

    uint16_t conc_ppm;
    int16_t temperature;
};

typedef struct node_senseair node_senseair;
#define NODE_SENSEAIR_CLASS 3
#define NODE_SENSEAIR_SIZE sizeof(node_senseair)

//
struct node_D {
    node_header header;

    double distance;
};

typedef struct node_D node_D;
#define NODE_D_CLASS 4
#define NODE_D_SIZE sizeof(node_D)

//
struct node_lis2dw12_cmt {
    node_header header;

    uint8_t alert;
};

typedef struct node_lis2dw12_cmt node_lis2dw12_cmt;
#define NODE_LIS2DW12_CMT_CLASS 5
#define NODE_LIS2DW12_CMT_SIZE sizeof(node_lis2dw12_cmt)

//
struct node_bme688 {
    node_header header;

    int16_t temperature;
    int16_t humidity;
    int16_t pressure;
    uint32_t gas;
};

typedef struct node_bme688 node_bme688;
#define NODE_BME688_CLASS 6
#define NODE_BME688_SIZE sizeof(node_bme688)

//
struct node_ds118b20 {
    node_header header;

    int16_t temperature;
};

typedef struct node_ds18b20 node_ds18b20;
#define NODE_DS18B20_CLASS 7
#define NODE_DS18B20_SIZE sizeof(node_ds18b20)


//
struct gateway_data {
    int16_t rssi;
    int8_t snr;
};

typedef struct gateway_data gateway_data;
#define GATEWAY_DATA_SIZE sizeof(gateway_data)

//
const char *CLASS_LIST[] = {
    "NO SENSOR",
    "Node HDC3020 Humidity + Temperature",
    "Node LIS2DW12 Acceleration + Temperature",
    "Node SENSEAIR CO2 + Temperature",
    "Time of Flight",
    "Node LIS2DW12 Ciccio me tocca",
    "Node BME688 Temp + Hum + Press + Gas",
    "Node DS18B20 Temperature"
};
