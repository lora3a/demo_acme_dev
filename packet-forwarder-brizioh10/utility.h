#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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
    uint32_t result = (ptr[0] & 0x000000ff) +
                      ((ptr[1] <<
                        8) & 0x0000ff00) +
                      ((ptr[2] << 16) & 0x00ff0000) + ((ptr[3] << 24) & 0xff000000);

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
