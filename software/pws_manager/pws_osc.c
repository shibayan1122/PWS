///////////////////////////////////////////////////////////
// pws_osc.c
///////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "def.h"
#include "pws_osc.h"
#include "pws_gpio.h"
#include "pws_debug.h"

static int oscPadSize(int len);
static int32_t oscEndian_i(const int32_t x);
static float oscEndian_f(const float x);

//-----------------------------------------------------------------------------
//【関数名】 oscDecode
// 
//【内  容】 OSCメッセージにデコード
// 
//【引  数】 uint8_t      *data     OSCメッセージ
//           int           size     OSCメッセージの長さ
//           OSC_MESSAGE  *msg      デコード結果（構造体）
// 
//【戻り値】 0 以上: 引数の個数
//           -1    : 失敗
// 
//【履  歴】 [新規] 2016/07/04
//-----------------------------------------------------------------------------
int oscDecode(uint8_t *data, int size, OSC_MESSAGE *msg)
{
    uint8_t *       ptr     = data;
    uint8_t *       end     = data + size;
    char            c;
    char *          adr     = NULL;
    char *          typ     = NULL;
    int             i, pad, dlen;
    int             num     = 0;
    int             len     = 0;
    DECODE_STATE    state   = STANDBY;

    if (data == NULL) {
        PWS_DEBUG("ERROR: data is NULL\n");
        return -1;
    }

    if (msg == NULL) {
        PWS_DEBUG("ERROR: msg is NULL\n");
        return -1;
    }

    while (ptr < end && num < OSC_DATA_NUM) {
        c = *ptr;
        switch (state) {
        case STANDBY:
            if (c == '/') {
                adr = (char *)ptr;
                ptr++;
                state = ADDRESS;
            }
            else {
                PWS_DEBUG("ERROR: STANDBY\n");
                return -1;
            }
            break;
        case ADDRESS:
            if (c != '\0') {
                PWS_DEBUG("INFO: addr[%s]\n", adr);
                msg->addr = adr;
                ptr += strlen((char *)ptr);
            }
            state = ADDRESS_PADDING;
            break;
        case ADDRESS_PADDING:
            while (*ptr == '\0' && ptr < end) {
                ptr++;
            }
            if (ptr < end) {
                c = *ptr;
                if (c == ',') {
                    ptr++;
                    state = TYPES;
                }
                else {
                    PWS_DEBUG("ERROR: ADDRESS_PADDING\n");
                    return -1;
                }
            }
            break;
        case TYPES:
            if (c != '\0') {
                len = strlen((const char *)ptr);
                typ = (char *)ptr;
                ptr += len;
            }
            state = TYPES_PADDING;
            break;
        case TYPES_PADDING:
            pad = oscPadSize(ptr - data);
            if (pad == 0) {
                pad = 4;
            }
            while (pad-- && ptr < end) {
                ptr++;
            }
            state = DATA;
            break;
        case DATA:
            len = strlen(typ);
            for (i = 0; i < len; i++) {
                switch (typ[i]) {
                case 'i':
                    if (ptr + 4 <= end) {
	                    PWS_DEBUG("INFO[%d]: int[%d]\n", i, *((const int *)ptr));
                        msg->data[num].type = typ[i];
                        msg->data[num].dlen = 4;
                        msg->data[num].u.i = oscEndian_i(*((const int32_t *)ptr));
                        ptr += 4;
                    }
                    num++;
                    msg->num = num;
                    break;
                case 'f':
                    if (ptr + 4 <= end) {
	                    PWS_DEBUG("INFO[%d]: float[%f]\n", i, *((const float *)ptr));
                        msg->data[num].type = typ[i];
                        msg->data[num].dlen = 4;
                        msg->data[num].u.f = oscEndian_f(*((const float *)ptr));
                        ptr += 4;
                        num++;
                        msg->num = num;
                    }
                    break;
                case 's':
                    dlen = strlen((const char *)ptr);
                    if (ptr + dlen <= end) {
	                    PWS_DEBUG("INFO[%d]: str[%s]\n", i, ptr);
                        msg->data[num].type = typ[i];
                        msg->data[num].dlen = dlen;
                        msg->data[num].u.s  = (char *)ptr;
                        ptr += dlen;
                        num++;
                        msg->num = num;
                    }
                    break;
                default:
                    break;
                }
            }
            state = DATA_PADDING;
            break;
        case DATA_PADDING:
            ptr = end;
            break;
        default:
            break;
        }
    }

    return num;
}

//-----------------------------------------------------------------------------
//【関数名】 oscEncode
// 
//【内  容】 OSCメッセージにエンコード
// 
//【引  数】 OSC_MESSAGE  *msg      エンコードするメッセージ（構造体）
//           uint8_t     **data     OSCメッセージ
//           int          *size     OSCメッセージの長さ
//
//【戻り値】  0 : 成功
//           -1 : 失敗
// 
//【履  歴】 [新規] 2016/07/04
//-----------------------------------------------------------------------------
int oscEncode(OSC_MESSAGE *msg, uint8_t *data, int *size)
{
    uint8_t nullChar = '\0';
    int i, len, pad, count;
    uint8_t  *p = data;
    uint8_t  *ptr = p;
    uint8_t  *pu;
    uint8_t  *pf;
    uint32_t ui;
    float    uf;

    if (msg == NULL) {
        PWS_DEBUG("ERROR: msg is NULL\n");
        return -1;
    }

    ////////////
    // ADDRESS
    ////////////
    len = strlen(msg->addr) + 1;
    pad = oscPadSize(len);

    memcpy(ptr, msg->addr, len);
    ptr += len;

    while (pad--) {
        *ptr = nullChar;
        ptr++;
    }

    ////////////
    // TYPE
    ////////////
    if (msg->num > 0) {
        *ptr = ',';
        ptr++;
        for (i = 0; i < msg->num; i++) {
            *ptr = msg->data[i].type;
            ptr++;
        }
        pad = oscPadSize(msg->num + 1); // 1 is for the comma
        if (pad == 0){
            pad = 4;
        }
        while (pad--) {
            *ptr = nullChar;
            ptr++;
        }
    }

    ////////////
    // DATA
    ////////////
    count = msg->num;
    if (count > OSC_DATA_NUM) {
        count = OSC_DATA_NUM;
    }
    for (i = 0; i < count; i++) {
        OSC_DATA *d = &(msg->data[i]);
        switch (d->type) {
        case 's':
            memcpy(ptr, d->u.s, d->dlen);
            ptr += d->dlen;
            pad = oscPadSize(d->dlen);
            if (pad == 0){
                pad = 4;
            }
            while (pad--) {
                *ptr = nullChar;
                ptr++;
            }
            break;
        case 'i':
            ui = oscEndian_i(d->u.i);
            pu = (uint8_t *) &ui;
            memcpy(ptr, pu, d->dlen);
            ptr += d->dlen;
            break;
        case 'f':
            uf = oscEndian_f(d->u.f);
            pf = (uint8_t *) &uf;
            memcpy(ptr, pf, d->dlen);
            ptr += d->dlen;
            break;
        }
    }

    *size = ptr - p;

    return 0;
}

//-----------------------------------------------------------------------------
//【関数名】 oscPadSize
// 
//【内  容】 データ長を４バイト境界とするためのパディングバイト数を求める
//           注）文字列の場合は、末尾の'\0'を含むサイズを与えること
// 
//【引  数】 int len    データ長
//
//【戻り値】 パディングバイト数
// 
//【履  歴】 [新規] 2016/07/04
//-----------------------------------------------------------------------------
static int oscPadSize(int len)
{
    int area = (len + 3) / 4;
    area *= 4;
    return area - len;
}

//-----------------------------------------------------------------------------
//【関数名】 oscEndian_i
// 
//【内  容】 32ビット整数のエンディアンを逆転する
// 
//【引  数】 const int32_t  x
//
//【戻り値】 エンディアンを逆転した32ビット整数
// 
//【履  歴】 [新規] 2016/07/04
//-----------------------------------------------------------------------------
static int32_t oscEndian_i(const int32_t x)
{
    int32_t ret;
    int size = sizeof(int32_t);
    char* src = (char*)&x + sizeof(int32_t) - 1;
    char* dst = (char*)&ret;
    while (size-- > 0) {
        *dst++ = *src--;
    }
    return ret;
}

//-----------------------------------------------------------------------------
//【関数名】 oscEndian_f
// 
//【内  容】 単精度実数のエンディアンを逆転する
// 
//【引  数】 const float    x
//
//【戻り値】 エンディアンを逆転した単精度実数
// 
//【履  歴】 [新規] 2016/07/04
//-----------------------------------------------------------------------------
static float oscEndian_f(const float x)
{
    float ret;
    int size = sizeof(float);
    char* src = (char*)&x + sizeof(float) - 1;
    char* dst = (char*)&ret;
    while (size-- > 0) {
        *dst++ = *src--;
    }
    return ret;
}
