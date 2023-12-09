#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "od.h"
#include "fmt.h"
#include "thread.h"
#include "acme_lora.h"

#include "xtimer.h"
#include "msg.h"
#include "ringbuffer.h"

#include "nodes_data.h"

#define FW_VERSION "01.00.00"

#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE    !FALSE
#endif


/* =========================================================== */
/* UART : INCLUDES, DEFINES, STRUCTS, FUNCTIONS                */
/* =========================================================== */
#include "periph/uart.h"
#include "periph/gpio.h"


#define UART_PORT           UART_DEV(1)
#define UART_SPEED          115200UL
#define UART_BUF_SIZE       (128U)

typedef struct {
    char rx_mem[UART_BUF_SIZE];
    ringbuffer_t rx_buf;
} uart_ctx_t;

#define MAX_JSON_MSG_LEN 1000
#define MAX_JSON_NODE_LEN 100

char jNode[MAX_JSON_NODE_LEN];
char cpuid_hex[CPUID_LEN * 2 + 1];
char strJasonMsg[MAX_JSON_MSG_LEN];
char * pstrBlob;
int lenBlob;
char *s_id;

static uart_ctx_t u_ctx;

static void rx_cb(void *arg, uint8_t data)
{
    (void)arg;
    ringbuffer_add_one(&u_ctx.rx_buf, data);
}

void b_init(void)
{
    /* add pullups to UART0 pins */
    PORT->Group[PA].PINCFG[22].bit.PULLEN = 1;
    PORT->Group[PA].PINCFG[23].bit.PULLEN = 1;
    gpio_init(GPIO_PIN(PA, 27), GPIO_OUT);
    //	Led Port
    int i;

    for (i = 0; i < 3; i++) {
        gpio_write(GPIO_PIN(PA, 27), 0);
        xtimer_usleep(250000);
        gpio_write(GPIO_PIN(PA, 27), 1);
        xtimer_usleep(250000);
    }

}


int8_t int8(const char *ptr)
{
    int8_t result = ptr[0];

    return result;
}


uint8_t uint8(const char *ptr)
{
    uint8_t result = ptr[0];

    return result;
}

uint16_t uint16(const char *ptr)
{
    uint16_t result = ptr[0] + (ptr[1] << 8);

    return result;
}

int16_t int16(const char *ptr)
{
    int16_t result = ptr[0] + (ptr[1] << 8);

    return result;
}

uint32_t uint32(const char *ptr)
{
    //printf("%2x %2x %2x  %2x\n",ptr[0],ptr[1],ptr[2],ptr[3]);
    uint32_t result = (ptr[0] & 0x000000ff) + ((ptr[1] << 8) & 0x0000ff00) + ((ptr[2] << 16) & 0x00ff0000) + ((ptr[3] << 24) & 0xff000000);

    return result;
}




void *combine(void *o1, size_t s1, void *o2, size_t s2)
{
    void *result = malloc(s1 + s2);

    if (result != NULL) {
        memcpy(result, o1, s1);
        memcpy((result + s1), o2, s2);
    }
    else {
        free(result);
    }
    return result;
}

void setNumericStringJnode(char *name, char *value, char *jnode)
{
    sprintf(jnode, "\"%s\":%s,", name, value);
}

void setStringJnode(char *name, char *value, char *jnode)
{
    sprintf(jnode, "\"%s\":\"%s\",", name, value);
}
void setNumericJnode(char *name, int value, char *jnode)
{
    sprintf(jnode, "\"%s\":%d,", name, value);
}

//
//	Converts the info part of the binary array in a json formatted string specifically for SENSEAIR Nodes
//
void binaryToJsonSENSEAIR(const char *message, char *json)
{
    char *p = (char *)message;
    char strFmt[10];
    uint16_t conc_ppm;
    int16_t temperature;

    p += NODE_HEADER_SIZE;
    conc_ppm = uint16(p);
    sprintf(strFmt, "%5d", conc_ppm);
    printf("conc_ppm      : %s\n", strFmt);
    setNumericStringJnode("conc_ppm", strFmt, jNode);
    strcat(json, jNode);

    p += 2;
    temperature = int16(p);
    sprintf(strFmt, "%6.2f", ((double)temperature) / 100.);
    printf("temperature   : %s\n", strFmt);
    setNumericStringJnode("temperature", strFmt, jNode);
    strcat(json, jNode);
}

//
//	Converts the info part of the binary array in a json formatted string specifically for AT Nodes
//
void binaryToJsonAT(const char *message, char *json)
{
    char *p = (char *)message;
    char strFmt[10];
    int16_t acc_x;
    int16_t acc_y;
    int16_t acc_z;
    int16_t temperature;

    p += NODE_HEADER_SIZE;
    acc_x = int16(p);
    sprintf(strFmt, "%5d", acc_x);
    printf("acc_x         : %s\n", strFmt);
    setNumericStringJnode("acc_x", strFmt, jNode);
    strcat(json, jNode);

    p += 2;
    acc_y = int16(p);
    sprintf(strFmt, "%5d", acc_y);
    printf("acc_y         : %s\n", strFmt);
    setNumericStringJnode("acc_y", strFmt, jNode);
    strcat(json, jNode);

    p += 2;
    acc_z = int16(p);
    sprintf(strFmt, "%5d", acc_z);
    printf("acc_z         : %s\n", strFmt);
    setNumericStringJnode("acc_z", strFmt, jNode);
    strcat(json, jNode);

    p += 2;
    temperature = int16(p);
    sprintf(strFmt, "%6.2f", ((double)temperature) / 100.);
    printf("temperature   : %s\n", strFmt);
    setNumericStringJnode("temperature", strFmt, jNode);
    strcat(json, jNode);

}


//
//	Converts the info part of the binary array in a json formatted string specifically for HT Nodes
//
void binaryToJsonHT(const char *message, char *json)
{
    char *p = (char *)message;
    char strFmt[10];
    int16_t temperature;
    uint16_t humidity;
    double temp;
    double hum;

    p += NODE_HEADER_SIZE;
    temperature = int16(p);
    temp = (double)temperature / 100.;
    sprintf(strFmt, "%6.2f", temp);
    printf("temperature   : %s\n", strFmt);
    setStringJnode("temperature", strFmt, jNode);
    strcat(json, jNode);

    p += 2;
    humidity = uint16(p);
    hum = (double)humidity / 100.;
    sprintf(strFmt, "%6.2f", hum);
    printf("humidity      : %s\n", strFmt);
    setStringJnode("humidity", strFmt, jNode);
    strcat(json, jNode);

}


//
//	Converts the info part of the binary array in a json formatted string specifically for CMT Nodes
//
void binaryToJsonCMT(const char *message, char *json)
{
    char *p = (char *)message;
    char strFmt[10];
    uint16_t alert;

    p += NODE_HEADER_SIZE;
    alert = (uint16_t )*p;
    printf("Alert         : %04x\n", alert);
    sprintf(strFmt, "%04x", alert);
    setStringJnode("alert", strFmt, jNode);
    strcat(json, jNode);
}

//
//	Converts the info part of the binary array in a json formatted string specifically for CMT Nodes
//
void binaryToJsonBME688(const char *message, char *json)
{
    char *p = (char *)message;
    char strFmt[10];
    double temp;
    double hum;   
    int16_t temperature;
    int16_t humidity;
    int16_t pressure;
    uint32_t gas;

    p += NODE_HEADER_SIZE;
    temperature = int16(p);
    temp = (double)temperature / 100.;
    sprintf(strFmt, "%6.2f", temp);
    printf("temperature   : %s\n", strFmt);
    setStringJnode("temperature", strFmt, jNode);
    strcat(json, jNode);

    p += 2;
    humidity = uint16(p);
    hum = (double)humidity / 100.;
    sprintf(strFmt, "%6.2f", hum);
    printf("humidity      : %s\n", strFmt);
    setStringJnode("humidity", strFmt, jNode);
    strcat(json, jNode);

    p += 2;
    pressure = int16(p);
    sprintf(strFmt, "%d", pressure);
    printf("pressure      : %s\n", strFmt);
    setStringJnode("pressure", strFmt, jNode);
    strcat(json, jNode);


    //  Solve structure allignement +4 instead of +22
    p += 4;
    gas = uint32(p);
    sprintf(strFmt, "%ld", gas);
    printf("gas           : %s\n", strFmt);
    setStringJnode("gas", strFmt, jNode);
    strcat(json, jNode);
}



//
//	Converts the binary data of unknown origin in a json formatted string blob
//
void binaryToJsonBlob(int16_t *rssi, int8_t *snr, char * data, int lendata, char *json)
{
    int availJson;
    setNumericJnode("rssi", (int)*rssi, jNode);
    strcat(json, jNode);

    setNumericJnode("snr", (int)*snr, jNode);
    strcat(json, jNode);

 
    strcat(json, "\"blob\":\"");
    availJson  = MAX_JSON_MSG_LEN-strlen(json)-1;
    lenBlob = (lendata * 2 )+ 1;
    if(lenBlob<availJson){
        pstrBlob = malloc(lenBlob);
        if (pstrBlob!=NULL) {
            memset(pstrBlob,0,lenBlob);
            fmt_bytes_hex(pstrBlob, (const uint8_t *)data, lendata);
            strcat(json, pstrBlob);
            free(pstrBlob);
        }
        else{
            sprintf(jNode, "Cannot Allocate %d Bytes to Send Hex Blob\n", lenBlob);
            strcat(json, jNode);
       }
    }
    else{
        sprintf(jNode, "Hex Blob %d > Bigger Than Available Jason String %d \n", lenBlob, availJson);
        strcat(json, jNode);
    }
     strcat(json, "\",");

}

//
//	Converts the binary array of the header in a json formatted string for all Nodes
//
void binaryToJsonHeader(int16_t *rssi, int8_t *snr, uint16_t signature, uint8_t class,
                        char *cpuid_hex, double vcc, double vpanel,
                        int8_t node_power, uint8_t node_boost, uint16_t sleep_time, char *json)
{
    char strFmt[10];

    setNumericJnode("rssi", (int)*rssi, jNode);
    strcat(json, jNode);

    setNumericJnode("snr", (int)*snr, jNode);
    strcat(json, jNode);

    sprintf(strFmt, "%04x", signature);
    setStringJnode("signature", strFmt, jNode);
    strcat(json, jNode);

    setNumericJnode("s_class", (int)class, jNode);
    strcat(json, jNode);

    setStringJnode("cpuid", cpuid_hex, jNode);
    strcat(json, jNode);

    sprintf(strFmt, "%5.3f", vcc);
    setNumericStringJnode("vcc", strFmt, jNode);
    strcat(json, jNode);

    sprintf(strFmt, "%5.3f", vpanel);
    setNumericStringJnode("vpanel", strFmt, jNode);
    strcat(json, jNode);


    sprintf(strFmt, "%3d", node_power);
    setNumericStringJnode("node_power", strFmt, jNode);
    strcat(json, jNode);

    sprintf(strFmt, "%3d", node_boost);
    setNumericStringJnode("node_boost", strFmt, jNode);
    strcat(json, jNode);

    sprintf(strFmt, "%5d", sleep_time);
    setNumericStringJnode("sleep_time", strFmt, jNode);
    strcat(json, jNode);
}

void dump_message(const char *message, size_t len, int16_t *rssi, int8_t *snr)
{
    char *ptr = (char *)message;
    uint16_t signature;
    uint8_t s_class;
    int res = FALSE;
    uint8_t d_size;
    uint16_t vcc;
    uint16_t vpanel;
    int8_t node_power;
    uint8_t node_boost;
    uint16_t sleep_time;

    gpio_write(GPIO_PIN(PA, 27), 0);
    memset(strJasonMsg, 0, sizeof(strJasonMsg));
    sprintf(strJasonMsg, "\nRx Pkt  : %u bytes, RSSI: %i, SNR: %i\n", len, *rssi, *snr);
    printf("%s", strJasonMsg);
    signature =  uint16(message);
    if (signature == ACME_SIGNATURE) {
        if (len >= NODE_HEADER_SIZE) {
            ptr += 2;
            s_class = *ptr;
            printf("Class         : %03d\n", s_class);
            ptr += 1;

            memset(cpuid_hex, 0, sizeof(cpuid_hex));
            s_id = ptr;
            fmt_bytes_hex(cpuid_hex, (const uint8_t *)s_id, CPUID_LEN);
            ptr += CPUID_LEN;

            ptr += 1;
            vcc = uint16(ptr);
            ptr += 2;
            vpanel = uint16(ptr);
            ptr += 2;
            node_power = int8(ptr);
            ptr += 1;
            node_boost = uint8(ptr);
            ptr += 1;
            sleep_time = uint16(ptr);
            switch (s_class) {
            case NODE_NOSENSOR_CLASS:
                res = TRUE;
                d_size = NODE_NOSENSOR_SIZE;
                break;
            case NODE_HT_CLASS:
                res = TRUE;
                d_size = NODE_HT_SIZE;
                break;
            case NODE_AT_CLASS:
                res = TRUE;
                d_size = NODE_AT_SIZE;
                break;
            case NODE_SENSEAIR_CLASS:
                res = TRUE;
                d_size = NODE_SENSEAIR_SIZE;
                break;
            case NODE_CMT_CLASS:
                res = TRUE;
                d_size = NODE_CMT_SIZE;
                break;
            case NODE_BME688_CLASS:
                res = TRUE;
                d_size = NODE_BME688_SIZE;
                break;
            default:
                printf("Unknown Class, Blob Packet Sent!\n");
                res = FALSE;
            }
        }
        else {
            printf("Packet Too Short, Blob Packet Sent!\n");
        }
    }
    else {
        printf("Wrong Signature, Blob Packet Sent!\n");
    }

    if (res) {
        printf("Sensors       : %s\n", CLASS_LIST[s_class]);
        if (d_size == len) {
            printf("Cpu Id        : %s\n", cpuid_hex);
            printf("Voltage       : vcc = %5.3f, vpanel = %5.3f\n", ((double)vcc / 1000.),
                   ((double)vpanel / 1000.));
            printf("Node Power    : %d\n", node_power);
            printf("Node Boost    : %d\n", node_boost);
            printf("Sleep Time    : %d\n", sleep_time);
            memset(strJasonMsg, 0, sizeof(strJasonMsg));
            sprintf(strJasonMsg, "{");
            binaryToJsonHeader(rssi, snr, signature, s_class, cpuid_hex, ((double)vcc / 1000.),
                               ((double)vpanel / 1000.), node_power, node_boost, sleep_time, strJasonMsg);
            switch (s_class) {
            case NODE_NOSENSOR_CLASS:
                break;
            case NODE_HT_CLASS:
                binaryToJsonHT(message, strJasonMsg);
                break;
            case NODE_AT_CLASS:
                binaryToJsonAT(message, strJasonMsg);
                break;
            case NODE_SENSEAIR_CLASS:
                binaryToJsonSENSEAIR(message, strJasonMsg);
                break;
            case NODE_CMT_CLASS:
                binaryToJsonCMT(message, strJasonMsg);
                break;
            case NODE_BME688_CLASS:
                binaryToJsonBME688(message, strJasonMsg);
                break;
            default:
                printf("Unknown Class, Blob Packet Sent!\n");
                res = FALSE;
            }
       }
        else {
            printf("Wrong Packet Length %3d != %3d, Blob Packet Sent!\n", d_size, NODE_HEADER_SIZE);
            res = FALSE;
        }
    }
    else {
        printf("Blob Sent to Fox D27\n");
        res = FALSE;
    }

    if (res != FALSE) {
        xtimer_usleep(5000);
    }else{
        memset(strJasonMsg, 0, sizeof(strJasonMsg));
        sprintf(strJasonMsg, "{");
        binaryToJsonBlob(rssi, snr, (char *)message, len, strJasonMsg);
    }
    strJasonMsg[strlen(strJasonMsg) - 1] = '}';
    strJasonMsg[strlen(strJasonMsg)] = '\n';
    printf("%s\n", strJasonMsg);
    uart_write(UART_PORT, (uint8_t *)strJasonMsg, strlen(strJasonMsg));
    xtimer_usleep(5000);
    
    gpio_write(GPIO_PIN(PA, 27), 1);


}
void report_lora_setup(void)
{

    printf("LORA.BW %d\n", DEFAULT_LORA_BANDWIDTH);
    printf("LORA.SF %d\n", DEFAULT_LORA_SPREADING_FACTOR);
    printf("LORA.CR %d\n", DEFAULT_LORA_CODERATE);
    printf("LORA.CH %ld\n", DEFAULT_LORA_CHANNEL);
    printf("LORA.PW %d\n", DEFAULT_LORA_POWER);

}
int main(void)
{
    lora_state_t lora;
    int res;

    printf("\n");
    printf("-------------------------------------\n");
    printf("-   Packet Forwarder For Fox D27    -\n");
    printf("-          by Acmesystems           -\n");
    printf("-  Version  : %s              -\n", FW_VERSION);
    printf("-  Compiled : %s %s  -\n", __DATE__, __TIME__);
    printf("-------------------------------------\n\n");

    //	Init uart rx ringbuffer
    ringbuffer_init(&u_ctx.rx_buf, u_ctx.rx_mem, UART_BUF_SIZE);
    //	Init uart, with error handling
    res = uart_init(UART_PORT, UART_SPEED, rx_cb, NULL);
    if (res == UART_NOBAUD) {
        printf("Error: Given baudrate (%lu) not possible\n", UART_SPEED);
        return 1;
    }
    else if (res != UART_OK) {
        puts("Error: Unable to initialize UART device");
        return 1;
    }
    b_init();

    printf("Success: Initialized UART at BAUD %lu\n", UART_SPEED);
    printf("Initialization OK.\n");

#if 0
    //
    //	Test Serial Tx
    //
    char message[6] = "PING.\0";
    uint8_t counter = 0;
    while (1) {
        uart_write(UART_PORT, (uint8_t *)message, 5);
        counter++;
        message[4] = (char)((counter % 10) + 0x30);
        printf("Tx >  %s \n", message);
        xtimer_usleep(500000);
    }
#endif

#if 0
    //
    //  Test Serial Rx
    //
    printf("Waiting for Data From Serial Port\n");
    int blnLoopRx = TRUE;
    int c;
    char sermsg[100];
    int pos = 0;
    int cnt = 50;
    while (blnLoopRx) {
        //	Check incoming data from Uart
        c = ringbuffer_peek_one(&u_ctx.rx_buf);
        if ((c == -1)) {
            //	No incomingdata from Uart
            xtimer_usleep(250000);
            //	Check timeout
            if ((cnt != 0)) {
                cnt--;
                printf(".\n");
            }
            else {
                // Time Out
                blnLoopRx = FALSE;
                printf("Serial Port T.O.\n");
            }
        }
        else {
            //	Data available on Uart, send to Consolle
            c = ringbuffer_get_one(&u_ctx.rx_buf);
            cnt = 50;
            pos = 0;
            memset(sermsg, 0, sizeof(sermsg));
            while (c > 0) {
                sermsg[pos] = c;
                c = ringbuffer_get_one(&u_ctx.rx_buf);
                pos++;
            }
            printf("Uart To Radio %s\n", sermsg);
        }
    }
#endif

#if 0
    //
    //  Test Led
    //
    while (1) {
        gpio_write(GPIO_PIN(PA, 27), 1);
        xtimer_usleep(250000);
        gpio_write(GPIO_PIN(PA, 27), 0);
        xtimer_usleep(250000);
    }
#endif

#if 0
    //
    //  Test RR wakeup
    //
    gpio_init(GPIO_PIN(PB, 22), GPIO_OUT);
    while (1) {
        gpio_write(GPIO_PIN(PA, 27), 1);
        gpio_write(GPIO_PIN(PB, 22), 1);
        xtimer_usleep(500000);
        gpio_write(GPIO_PIN(PB, 22), 0);
        gpio_write(GPIO_PIN(PA, 27), 0);
        xtimer_usleep(500000);
    }
#endif

    memset(&lora, 0, sizeof(lora));
    lora.bandwidth = DEFAULT_LORA_BANDWIDTH;
    lora.spreading_factor = DEFAULT_LORA_SPREADING_FACTOR;
    lora.coderate = DEFAULT_LORA_CODERATE;
    lora.channel = DEFAULT_LORA_CHANNEL;
    lora.power = DEFAULT_LORA_POWER;
    lora.data_cb = *dump_message;
    if (lora_init(&(lora)) != 0) {
        return 1;
    }
    report_lora_setup();
    lora_listen();
    puts("Listen mode set; waiting for messages");
    thread_sleep();
    return 0;
}
