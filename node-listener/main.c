#include <stdio.h>
#include <string.h>

#include "thread.h"
#include "acme_lora.h"

char payload[MAX_PACKET_LEN];

void dump_message(const char *message, size_t len, int16_t *rssi, int8_t *snr)
{
    size_t n = (len < MAX_PACKET_LEN-1) ? len : MAX_PACKET_LEN-2;
    memcpy(payload, message, n);
    payload[n] = 0;
    printf("{\"rssi\":%d,\"snr\":%d,\"payload\":\"%s\"}\n", *rssi, *snr, payload);
}

void dump_message_hex(const char *message, size_t len, int16_t *rssi, int8_t *snr)
{
    size_t n = (len < MAX_PACKET_LEN-1) ? len : MAX_PACKET_LEN-2;
    memcpy(payload, message, n);
    payload[n] = 0;
    printf("{\"rssi\":%d,\"snr\":%d,\"payloadHEX\":", *rssi, *snr);
    for (size_t i = 0; i<n; i++) {
		printf("%02X",payload[i]);
	}
	puts("}\n");	
}

int main(void)
{
    lora_state_t lora;

    memset(&lora, 0, sizeof(lora));
    lora.bandwidth = DEFAULT_LORA_BANDWIDTH;
    lora.spreading_factor = DEFAULT_LORA_SPREADING_FACTOR;
    lora.coderate = DEFAULT_LORA_CODERATE;
    lora.channel = DEFAULT_LORA_CHANNEL;
    lora.power = DEFAULT_LORA_POWER;
    lora.data_cb = *dump_message_hex;
    printf("LoRa Bandwith: %d\n", DEFAULT_LORA_BANDWIDTH);
    printf("LoRa Spreading Factor: %d\n", DEFAULT_LORA_SPREADING_FACTOR);
    printf("LoRa CodeRate: %d\n", DEFAULT_LORA_CODERATE);
    printf("LoRa Channel: %ld\n", DEFAULT_LORA_CHANNEL);
    printf("LoRa Power: %d\n", DEFAULT_LORA_POWER);

    if (lora_init(&(lora)) != 0) {
        return 1;
    }
    lora_listen();
    thread_sleep();
    return 0;
}
