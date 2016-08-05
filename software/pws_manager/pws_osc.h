///////////////////////////////////////////////////////////
// pws_osc.h
///////////////////////////////////////////////////////////
#ifndef __PWS_OSC_H__
#define __PWS_OSC_H__

#define DECODE_BUFF_SIZE    (512)
#define OSC_DATA_NUM        (3)
#define OSC_STRING_LEN      (512)

typedef enum {
    STANDBY,
    ADDRESS,
    ADDRESS_PADDING,
    TYPES,
    TYPES_PADDING,
    DATA,
    DATA_PADDING,
    DONE,
} DECODE_STATE;

typedef struct {
    int             dlen;
    char            type;
    union {
        char *      s;    // string
        int32_t     i;    // int
        float       f;    // float
        double      d;    // double
        uint64_t    l;    // long
        uint8_t *   b;    // blob
    } u;
} OSC_DATA;

typedef struct {
    char *         addr;
    int            num;
    OSC_DATA       data[OSC_DATA_NUM];
} OSC_MESSAGE;

extern int oscDecode(uint8_t *data, int size, OSC_MESSAGE *msg);
extern int oscEncode(OSC_MESSAGE *msg, uint8_t *data, int *size);

#endif // __PWS_OSC_H__
