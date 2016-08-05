///////////////////////////////////////////////////////////
// pws_btn.c
///////////////////////////////////////////////////////////

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "def.h"
#include "pws_gpio.h"
#include "pws_osc.h"
#include "pws_btn.h"
#include "pws_debug.h"

#define BTN_1               (0)
#define BTN_2               (1)
#define BTN_3               (2)
#define BTN_4               (3)
#define BTN_5               (4)
#define BTN_6               (5)
#define BTN_7               (6)
#define MAX_BTN             (7)

#define STATE_RELEASE       (0)
#define STATE_PUSH          (1)
#define STATE_PUSH_OVER     (2)
#define STATE_MAX           (3)

#define EVT_NONE            (-1)
#define EVT_PUSH            (0)
#define EVT_RELEASE         (1)
#define EVT_LONG_PUSH       (2)
#define EVT_MAX             (3)

// 管理情報
static struct {
    int     pin;
    int     state;
    int     push;
    double  time;
} BtnCtx[MAX_BTN] = {
    { PIN_BTN_1, STATE_RELEASE, PIN_VAL_OFF, -1.0 },
    { PIN_BTN_2, STATE_RELEASE, PIN_VAL_ON , -1.0 },
    { PIN_BTN_3, STATE_RELEASE, PIN_VAL_ON , -1.0 },
    { PIN_BTN_4, STATE_RELEASE, PIN_VAL_ON , -1.0 },
    { PIN_BTN_5, STATE_RELEASE, PIN_VAL_ON , -1.0 },
    { PIN_BTN_6, STATE_RELEASE, PIN_VAL_ON , -1.0 },
    { PIN_BTN_7, STATE_RELEASE, PIN_VAL_ON , -1.0 },
};

static pthread_t       threadBtnID;
static pthread_mutex_t threadBtnMutex  = PTHREAD_MUTEX_INITIALIZER;
static int             threadBtnFinish = 0;

static void *threadBtnMonitor(void *arg);
static int btnActionPush(int btn);
static int btnActionRelease(int btn);
static int btnActionLongPush(int btn);
static int btnSendMessageToManager(char *msg);
static double btnGetCurrentMsec();
static int btnGetEvent(int idx);

// 状態遷移テーブル
static struct {
    int next;
    int (*func)(int btn);
} STATE_TABLE[STATE_MAX][EVT_MAX] = {
    //
    // STATE_RELEASE：ボタンが押されてない状態
    //
    {
        { STATE_PUSH      , btnActionPush     }, // ボタンが押された
        { STATE_RELEASE   , NULL              }, // ボタンが離された
        { STATE_RELEASE   , NULL              }, // 長押し時間を過ぎた
    },
    //
    // STATE_PUSH：ボタン押下状態
    //
    {
        { STATE_PUSH      , NULL              }, // ボタンが押された
        { STATE_RELEASE   , btnActionRelease  }, // ボタンが離された
        { STATE_PUSH_OVER , btnActionLongPush }, // 長押し時間を過ぎた
    },
    //
    // STATE_PUSH_OVER：ボタン押下で長押し時間超過状態
    //
    {
        { STATE_PUSH_OVER , NULL              }, // ボタンが押された
        { STATE_RELEASE   , NULL              }, // ボタンが離された
        { STATE_PUSH_OVER , NULL              }, // 長押し時間を過ぎた
    },
};

//
// ボタン監視初期化
//
int btnInitialize(void)
{
    PWS_DEBUG("btnInitialize\n");

    // スレッドの作成
    pthread_create(&threadBtnID , NULL, threadBtnMonitor , NULL);

    return 0;
}

//
// ボタン監視終了処理
//
void btnFinish(void)
{
    // ボタン監視スレッド終了
    pthread_mutex_lock(&threadBtnMutex);
    threadBtnFinish = 1;
    pthread_mutex_unlock(&threadBtnMutex);

    // スレッド終了待ち
    pthread_join(threadBtnID , NULL);

    PWS_DEBUG("btnFinish\n");
}

// ボタン監視スレッド
static void *threadBtnMonitor(void *arg)
{
    int i, ret, loop, next;
    int evt[MAX_BTN];
    int (*func)(int btn);
    struct timespec ts;

    // スリープ時間設定
    ts.tv_sec  = 0;
    ts.tv_nsec = 1000 * 1000 * 100; // 100 msec

    loop = 1;
    while (loop) {
        // ボタンイベントの取得
        for (i = 0; i < MAX_BTN; i++) {
            evt[i] = btnGetEvent(i);
        }

        // 状態遷移
        for (i = 0; i < MAX_BTN; i++) {
            if (evt[i] >= 0) {
                next = STATE_TABLE[BtnCtx[i].state][evt[i]].next;
                func = STATE_TABLE[BtnCtx[i].state][evt[i]].func;
                if (func != NULL) {
                    ret = func(i);
                    if (ret < 0) {
                       // func error !!
                        PWS_DEBUG("ERROR: func\n");
                    }
                }
                BtnCtx[i].state = next;
            }
        }
        // スリープ（100 msec）
        nanosleep(&ts, NULL);

        pthread_mutex_lock(&threadBtnMutex);
        if (threadBtnFinish == 1) {
            loop = 0;
        }
        pthread_mutex_unlock(&threadBtnMutex);
    }

    return (void *)NULL;
}

static int btnGetEvent(int idx)
{
    int evt, push;
    double nowTime;

    evt  = EVT_NONE;
    push = gpioRead(BtnCtx[idx].pin);

    if (push != BtnCtx[idx].push) {
        if (idx == 0)   // ボタン1（shutdown）
            evt = (push == PIN_VAL_ON) ? EVT_PUSH : EVT_RELEASE;
        else
            evt = (push == PIN_VAL_OFF) ? EVT_PUSH : EVT_RELEASE;
        BtnCtx[idx].push = push;
    }
    if (BtnCtx[idx].state == STATE_PUSH && evt == EVT_NONE) {
        if (BtnCtx[idx].time >= 0.0) {
            nowTime = btnGetCurrentMsec();
            if (nowTime - BtnCtx[idx].time > LONG_PUSH_TIME) {
                evt = EVT_LONG_PUSH;
            }
        }
    }
    
    return evt;
}

static int btnActionPush(int btn)
{
    BtnCtx[btn].time = btnGetCurrentMsec();

    return 0;
}

static int btnActionRelease(int btn)
{
    // ボタン押下イベント発行
    switch (btn) {
    case BTN_1:
        if (BtnCtx[BTN_3].state == STATE_PUSH || BtnCtx[BTN_3].state == STATE_PUSH_OVER) {
            PWS_DEBUG("ボタン押下 [1] [3]\n");
            btnSendMessageToManager(MSG_PUSH_AP_SET_BTN);
        }
        break;
    case BTN_2:
        PWS_DEBUG("ボタン押下 [2]\n");
        btnSendMessageToManager(MSG_PUSH_TUNING_BTN);
        break;
    case BTN_3:
        if (BtnCtx[BTN_1].state == STATE_PUSH || BtnCtx[BTN_1].state == STATE_PUSH_OVER) {
            PWS_DEBUG("ボタン押下 [1] [3]\n");
            btnSendMessageToManager(MSG_PUSH_AP_SET_BTN);
        }
        else {
            PWS_DEBUG("ボタン押下 [3]\n");
            btnSendMessageToManager(MSG_PUSH_EFFECT_BTN);
        }
        break;
    case BTN_4:
        PWS_DEBUG("ボタン押下 [4]\n");
        btnSendMessageToManager(MSG_PUSH_REC_BTN);
        break;
    case BTN_5:
        PWS_DEBUG("ボタン押下 [5]\n");
        btnSendMessageToManager(MSG_PUSH_PLAY_BTN);
        break;
    case BTN_6:
        PWS_DEBUG("ボタン押下 [6]\n");
        btnSendMessageToManager(MSG_PUSH_VOL_UP_BTN);
        break;
    case BTN_7:
        PWS_DEBUG("ボタン押下 [7]\n");
        btnSendMessageToManager(MSG_PUSH_VOL_DOWN_BTN);
        break;
    default:
        break;
    }

    return 0;
}

static int btnActionLongPush(int btn)
{
    // ボタン長押しイベント発行
    switch (btn) {
    case BTN_1:
        if (BtnCtx[BTN_2].state != STATE_PUSH && BtnCtx[BTN_2].state != STATE_PUSH_OVER &&
            BtnCtx[BTN_3].state != STATE_PUSH && BtnCtx[BTN_3].state != STATE_PUSH_OVER &&
            BtnCtx[BTN_4].state != STATE_PUSH && BtnCtx[BTN_4].state != STATE_PUSH_OVER &&
            BtnCtx[BTN_5].state != STATE_PUSH && BtnCtx[BTN_5].state != STATE_PUSH_OVER &&
            BtnCtx[BTN_6].state != STATE_PUSH && BtnCtx[BTN_6].state != STATE_PUSH_OVER &&
            BtnCtx[BTN_7].state != STATE_PUSH && BtnCtx[BTN_7].state != STATE_PUSH_OVER)
        {
            PWS_DEBUG("ボタン長押し [1]\n");
            btnSendMessageToManager(MSG_PUSH_SHUTDOWN_BTN);
        }
        break;
    case BTN_2:
        break;
    case BTN_3:
        break;
    case BTN_4:
        break;
    case BTN_5:
        break;
    case BTN_6:
        break;
    case BTN_7:
        break;
    default:
        break;
    }

    return 0;
}

// マネージャーにメッセージを送信
static int btnSendMessageToManager(char *msg)
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
        PWS_DEBUG("ERROR: Socket\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port        = htons(PWS_PORT_MANAGER);
    n = sendto(sock, sendBuf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n == -1) {
        PWS_DEBUG("ERROR: Sendto\n");
        close(sock);
        return -1;
    }

    close(sock);

    return 0;
}


// 起動後の時間を取得（ミリ秒）
static double btnGetCurrentMsec()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec * 0.001;
}

