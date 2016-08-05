///////////////////////////////////////////////////////////
// pws_gpio.c
///////////////////////////////////////////////////////////

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "def.h"
#include "pws_gpio.h"
#include "pws_osc.h"
#include "pws_btn.h"
#include "pws_led.h"
#include "pws_manager.h"
#include "pws_debug.h"

static int gpioSendMessageToManager(char *msg);

//
// GPIO初期化
//
int gpioInitialize(void)
{
    // wiringPi 初期化
    if (wiringPiSetup() == -1){
        PWS_DEBUG("ERROR: wiringPiSetup\n");
        return -1;
    }

    // ピンの入出力モード設定
    pinMode(PIN_LED_YELLOW, OUTPUT);
    pinMode(PIN_LED_RED   , OUTPUT);
    pinMode(PIN_LED_GREEN , OUTPUT);
    pinMode(PIN_BTN_1     , INPUT );
    pinMode(PIN_BTN_2     , INPUT );
    pinMode(PIN_BTN_3     , INPUT );
    pinMode(PIN_BTN_4     , INPUT );
    pinMode(PIN_BTN_5     , INPUT );
    pinMode(PIN_BTN_6     , INPUT );
    pinMode(PIN_BTN_7     , INPUT );
    
    // ピンの初期状態設定
    pullUpDnControl(PIN_BTN_1, PUD_DOWN);
    pullUpDnControl(PIN_BTN_2, PUD_UP);
    pullUpDnControl(PIN_BTN_3, PUD_UP);
    pullUpDnControl(PIN_BTN_4, PUD_UP);
    pullUpDnControl(PIN_BTN_5, PUD_UP);
    pullUpDnControl(PIN_BTN_6, PUD_UP);
    pullUpDnControl(PIN_BTN_7, PUD_UP);

    // ボタン初期化（スレッド作成）
    btnInitialize();

    // LED 初期化（スレッド作成）
    ledInitialize();

    return 0;
}

// 起動モードの判定
void gpioCheckApMode(void)
{
    int bootBtn1 = digitalRead(PIN_BTN_1);
    int bootBtn3 = digitalRead(PIN_BTN_3);

    if (bootBtn1 == PIN_VAL_ON && bootBtn3 == PIN_VAL_OFF)
        gpioSendMessageToManager(MSG_PUSH_AP_SET_BTN);
    else
        gpioSendMessageToManager(MSG_INITIALIZE);
}

// GPIO読込み
int gpioRead(int pin)
{
    return digitalRead(pin);
}

// GPIO書込み
void gpioWrite(int pin, int val)
{
    digitalWrite(pin, val);
}

// GPIO終了処理
void gpioFinish(void)
{
    ledFinish();
    btnFinish();
}

// マネージャーにメッセージを送信
static int gpioSendMessageToManager(char *msg)
{
    int n, ret, sock, len;
    uint8_t sendBuf[SEND_BUF_SIZE];
    struct sockaddr_in addr;
    OSC_MESSAGE oscMsg;

    memset(&oscMsg, 0, sizeof(oscMsg));
    
    oscMsg.addr = msg;
    oscMsg.num  = 0;
    ret = oscEncode(&oscMsg, (uint8_t *)sendBuf, &len);
    if (ret < 0) {
        return -1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        PWS_DEBUG("ERROR: socket\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port        = htons(PWS_PORT_MANAGER);
    n = sendto(sock, sendBuf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n == -1) {
        PWS_DEBUG("ERROR: sendto\n");
        return -1;
    }

    close(sock);

    return 0;
}

