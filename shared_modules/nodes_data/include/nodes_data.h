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
};


typedef struct node_header node_header;
#define NODE_HEADER_SIZE sizeof(node_header)

struct node_HT {
    node_header header;

    int16_t temperature;
    uint16_t humidity;
};


typedef struct node_HT node_HT;
#define NODE_HT_CLASS 1
#define NODE_HT_SIZE sizeof(node_HT)

struct node_AT {
    node_header header;

    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t temperature;
};


typedef struct node_AT node_AT;
#define NODE_AT_CLASS 2
#define NODE_AT_SIZE sizeof(node_AT)

struct node_CO2T {
    node_header header;

    uint16_t conc_ppm;
    int16_t temp_cC;
    int16_t temperature;
};


typedef struct node_CO2T node_CO2T;
#define NODE_CO2T_CLASS 3
#define NODE_CO2T_SIZE sizeof(node_CO2T)

struct node_D {
    node_header header;

    double distance;
};


typedef struct node_D node_D;
#define NODE_D_CLASS 4
#define NODE_D_SIZE sizeof(node_D)


struct node_CMT {
    node_header header;

    uint8_t alert;
};


typedef struct node_CMT node_CMT;
#define NODE_CMT_CLASS 5
#define NODE_CMT_SIZE sizeof(node_CMT)

struct node_BME688 {
    node_header header;

    int16_t temperature;
    int16_t humidity;
    int16_t pression;
    uint32_t gas;
};


typedef struct node_BME688 node_BME688;
#define NODE_BME688_CLASS 6
#define NODE_BME688_SIZE sizeof(node_BME688)

struct gateway_data {
    int16_t rssi;
    int8_t snr;
};

typedef struct gateway_data gateway_data;
#define GATEWAY_DATA_SIZE sizeof(gateway_data)


const char *CLASS_LIST[] = {
    "UNDEFINED",
    "Umidity + Temperature",
    "Acceleration + Temperature",
    "CO2 + Temperature",
    "Time of Flight",
    "Ciccio me tocca",
    "Node BME688 Temp + Hum + Press + Gas"
};
