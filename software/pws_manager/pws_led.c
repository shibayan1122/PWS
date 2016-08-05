///////////////////////////////////////////////////////////
// pws_led.c
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
#include "pws_led.h"
#include "pws_debug.h"

// LEDイベント
typedef enum {
    EVT_LED_NONE    = -1       ,    // イベントなし
    EVT_LED_RED_OFF =  0       ,    // 赤色 LED 消灯
    EVT_LED_RED_ON             ,    // 赤色 LED 点灯
    EVT_LED_RED_BLINK          ,    // 赤色 LED 点滅
    EVT_LED_RED_BLINK_FAST     ,    // 赤色 LED 早点滅
    EVT_LED_GREEN_OFF          ,    // 緑色 LED 消灯
    EVT_LED_GREEN_ON           ,    // 緑色 LED 点灯
    EVT_LED_GREEN_BLINK        ,    // 緑色 LED 点滅
    EVT_LED_GREEN_BLINK_FAST   ,    // 緑色 LED 早点滅
    EVT_LED_YELLOW_OFF         ,    // 黄色 LED 消灯
    EVT_LED_YELLOW_ON          ,    // 黄色 LED 点灯
    EVT_LED_YELLOW_BLINK       ,    // 黄色 LED 点滅
    EVT_LED_YELLOW_BLINK_FAST  ,    // 黄色 LED 早点滅
    EVT_LED_BLINK_RED_GREEN    ,    // 赤緑 LED 交互点滅
    EVT_LED_BLINK_GREEN_YELLOW ,    // 緑黄 LED 交互点滅
    EVT_LED_BLINK_YELLOW_RED   ,    // 黄赤 LED 交互点滅
    EVT_LED_FINISH             ,    // スレッド終了イベント
    EVT_LED_MAX                     // LEDイベント最大個数
} LED_EVENT;

// LEDステートのインデックス
#define LED_RED             (0)
#define LED_GREEN           (1)
#define LED_YELLOW          (2)
#define MAX_LED             (3)

// SEQ定義
#define SEQ_LED_BLINK_NONE  (-1)
#define SEQ_LED_BLINK_OFF   (0)
#define SEQ_LED_BLINK_ON    (1)
#define SEQ_MAX             (20)

// LED オフ
static const int SEQ_ALWAYS_OFF[SEQ_MAX] = {
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
};


// LED オン
static const int SEQ_ALWAYS_ON[SEQ_MAX] = {
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
};

// LED 遅い点滅
static const int SEQ_NORMAL_BLINK[SEQ_MAX] = {
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
};

// LED 遅い逆点滅
static const int SEQ_REVERSE_BLINK[SEQ_MAX] = {
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_ON,
};

// LED 早い点滅
static const int SEQ_FAST_BLINK[SEQ_MAX] = {
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
    SEQ_LED_BLINK_ON,
    SEQ_LED_BLINK_OFF,
};

// 管理情報
static struct {
    const int   pin;
    int         curLightUp;
    int *       seqLightUp;
} LedCtx[MAX_LED] = {
    { PIN_LED_RED   , SEQ_LED_BLINK_NONE, (int *)SEQ_ALWAYS_OFF },
    { PIN_LED_GREEN , SEQ_LED_BLINK_NONE, (int *)SEQ_ALWAYS_OFF },
    { PIN_LED_YELLOW, SEQ_LED_BLINK_NONE, (int *)SEQ_ALWAYS_OFF },
};


static int             sockRcv;
static pthread_t       threadLedID;
static pthread_t       threadRcvID;
static pthread_mutex_t threadLedMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t threadRcvMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t threadCtxMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  threadRcvCond  = PTHREAD_COND_INITIALIZER;
static int             threadLedFinish = 0;

static void *threadLedControl(void *arg);
static void *threadRcvManager(void *arg);
static int ledGetEvent(char *msg);
static void ledCloseSocket(void);
static int ledSendMessageToMyself(char *msg);

#if defined(DEBUG_LOGOUT_STDIO) || defined(DEBUG_LOGOUT_FILE)
//
// イベント文字列
//
static char *strEvt[] = {
    "赤色 LED 消灯",
    "赤色 LED 点灯",
    "赤色 LED 点滅",
    "赤色 LED 早点滅",
    "緑色 LED 消灯",
    "緑色 LED 点灯",
    "緑色 LED 点滅",
    "緑色 LED 早点滅",
    "黄色 LED 消灯",
    "黄色 LED 点灯",
    "黄色 LED 点滅",
    "黄色 LED 早点滅",
    "赤緑 LED 交互点滅",
    "緑黄 LED 交互点滅",
    "黄赤 LED 交互点滅",
    "スレッド終了イベント",
};
#endif

//
// LED制御初期化
//
int ledInitialize(void)
{
    int i;

    PWS_DEBUG("ledInitialize\n");

    // LED初期化
    for (i = 0; i < MAX_LED; i++) {
        gpioWrite(LedCtx[i].pin, PIN_VAL_OFF);
    }

    // スレッドの作成
    pthread_create(&threadRcvID, NULL, threadRcvManager, NULL);
    pthread_create(&threadLedID, NULL, threadLedControl, NULL);

    // 受信可能な状態まで待つ！
    pthread_mutex_lock(&threadRcvMutex);
    pthread_cond_wait(&threadRcvCond, &threadRcvMutex);
    pthread_mutex_unlock(&threadRcvMutex);

    return 0;
}

//
// LED制御終了処理
//
void ledFinish(void)
{
    // LED制御スレッド終了
    pthread_mutex_lock(&threadLedMutex);
    threadLedFinish = 1;
    pthread_mutex_unlock(&threadLedMutex);

    // スレッド終了待ち
    pthread_join(threadLedID, NULL);

    // メッセージ受信スレッド終了
    ledSendMessageToMyself(MSG_LED_FINISH);

    // スレッド終了待ち
    pthread_join(threadRcvID, NULL);

    PWS_DEBUG("ledFinish\n");
}

// LED制御スレッド
static void *threadLedControl(void *arg)
{
    
    int i, loop, seq;
    int flgLightUp;
    struct timespec sleep_ts;

    // スリープ時間設定
    sleep_ts.tv_sec  = 0;
    sleep_ts.tv_nsec = 1000 * 1000 * 100;

    seq = 0;
    loop = 1;
    while (loop) {
        pthread_mutex_lock(&threadCtxMutex);

        for (i = 0; i < MAX_LED; i++) {
            flgLightUp = LedCtx[i].seqLightUp[seq];
            if (LedCtx[i].curLightUp != flgLightUp) {
                if (flgLightUp == SEQ_LED_BLINK_ON) {
                    gpioWrite(LedCtx[i].pin, PIN_VAL_ON);
                }
                else if (flgLightUp == SEQ_LED_BLINK_OFF) {
                    gpioWrite(LedCtx[i].pin, PIN_VAL_OFF);
                }
                LedCtx[i].curLightUp = flgLightUp;
            }
        }

        pthread_mutex_unlock(&threadCtxMutex);

        // スリープ（500 msec）
        nanosleep(&sleep_ts, NULL);

        // SEQ更新
        seq++;
        if (seq >= SEQ_MAX) seq = 0;

        pthread_mutex_lock(&threadLedMutex);
        if (threadLedFinish == 1) {
            loop = 0;
        }
        pthread_mutex_unlock(&threadLedMutex);
     
    }

    return (void *)NULL;
}

// メッセージ受信スレッド
static void *threadRcvManager(void *arg)
{
    int n, rc, evt, loop;
    char buf[RECV_BUF_SIZE];
    struct sockaddr_in addr;

    // ソケット作成
    sockRcv = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockRcv == -1) {
        PWS_DEBUG("ERROR: socket\n");
        return (void *)NULL;
    }
    PWS_DEBUG("sock    %d\n", sockRcv);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PWS_PORT_LED_CONTROLLER);

    rc = bind(sockRcv, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1) {
        PWS_DEBUG("ERROR: bind\n");
        close(sockRcv);
        return (void *)NULL;
    }

    // 受信可能な状態であることを設定
    pthread_mutex_lock(&threadRcvMutex);
    pthread_cond_signal(&threadRcvCond);
    pthread_mutex_unlock(&threadRcvMutex);

    loop = 1;
    while (loop) {
        memset(buf, 0, sizeof(buf));
        n = recv(sockRcv, buf, sizeof(buf) - 1, 0);
        if (n < 0) {
            PWS_DEBUG("ERROR: recv\n");
            break;
        }
        evt = ledGetEvent(buf);
        pthread_mutex_lock(&threadCtxMutex);
        switch(evt) {

        // 赤色LED
        case EVT_LED_RED_OFF:
            LedCtx[LED_RED   ].seqLightUp = (int *)SEQ_ALWAYS_OFF;
            break;
        case EVT_LED_RED_ON:
            LedCtx[LED_RED   ].seqLightUp = (int *)SEQ_ALWAYS_ON;
            break;
        case EVT_LED_RED_BLINK:
            LedCtx[LED_RED   ].seqLightUp = (int *)SEQ_NORMAL_BLINK;
            break;
        case EVT_LED_RED_BLINK_FAST:
            LedCtx[LED_RED   ].seqLightUp = (int *)SEQ_FAST_BLINK;
            break;

        // 緑色LED
        case EVT_LED_GREEN_OFF:
            LedCtx[LED_GREEN ].seqLightUp = (int *)SEQ_ALWAYS_OFF;
            break;
        case EVT_LED_GREEN_ON:
            LedCtx[LED_GREEN ].seqLightUp = (int *)SEQ_ALWAYS_ON;
            break;
        case EVT_LED_GREEN_BLINK:
            LedCtx[LED_GREEN ].seqLightUp = (int *)SEQ_NORMAL_BLINK;
            break;
        case EVT_LED_GREEN_BLINK_FAST:
            LedCtx[LED_GREEN ].seqLightUp = (int *)SEQ_FAST_BLINK;
            break;

        // 黄色LED
        case EVT_LED_YELLOW_OFF:
            LedCtx[LED_YELLOW].seqLightUp = (int *)SEQ_ALWAYS_OFF;
            break;
        case EVT_LED_YELLOW_ON:
            LedCtx[LED_YELLOW].seqLightUp = (int *)SEQ_ALWAYS_ON;
            break;
        case EVT_LED_YELLOW_BLINK:
            LedCtx[LED_YELLOW].seqLightUp = (int *)SEQ_NORMAL_BLINK;
            break;
        case EVT_LED_YELLOW_BLINK_FAST:
            LedCtx[LED_YELLOW].seqLightUp = (int *)SEQ_FAST_BLINK;
            break;

        // ２色交互
        case EVT_LED_BLINK_RED_GREEN:
            LedCtx[LED_RED   ].seqLightUp = (int *)SEQ_NORMAL_BLINK;
            LedCtx[LED_GREEN ].seqLightUp = (int *)SEQ_REVERSE_BLINK;
            break;
        case EVT_LED_BLINK_GREEN_YELLOW:
            LedCtx[LED_GREEN ].seqLightUp = (int *)SEQ_NORMAL_BLINK;
            LedCtx[LED_YELLOW].seqLightUp = (int *)SEQ_REVERSE_BLINK;
            break;
        case EVT_LED_BLINK_YELLOW_RED:
            LedCtx[LED_YELLOW].seqLightUp = (int *)SEQ_NORMAL_BLINK;
            LedCtx[LED_RED   ].seqLightUp = (int *)SEQ_REVERSE_BLINK;
            break;
        case EVT_LED_FINISH:
            loop = 0;
            break;
        default:
            break;
        }
        pthread_mutex_unlock(&threadCtxMutex);
    }

    ledCloseSocket();

    return (void *)NULL;
}


// イベントの取得
static int ledGetEvent(char *msg)
{
    int i, evt = EVT_LED_NONE;
    static struct {
        int   evt;
        char* msg;
    } EVT_TABLE[] = {
        { EVT_LED_RED_OFF            , MSG_LED_RED_OFF            },
        { EVT_LED_RED_ON             , MSG_LED_RED_ON             },
        { EVT_LED_RED_BLINK          , MSG_LED_RED_BLINK          },
        { EVT_LED_RED_BLINK_FAST     , MSG_LED_RED_BLINK_FAST     },
        { EVT_LED_GREEN_OFF          , MSG_LED_GREEN_OFF          },
        { EVT_LED_GREEN_ON           , MSG_LED_GREEN_ON           },
        { EVT_LED_GREEN_BLINK        , MSG_LED_GREEN_BLINK        },
        { EVT_LED_GREEN_BLINK_FAST   , MSG_LED_GREEN_BLINK_FAST   },
        { EVT_LED_YELLOW_OFF         , MSG_LED_YELLOW_OFF         },
        { EVT_LED_YELLOW_ON          , MSG_LED_YELLOW_ON          },
        { EVT_LED_YELLOW_BLINK       , MSG_LED_YELLOW_BLINK       },
        { EVT_LED_YELLOW_BLINK_FAST  , MSG_LED_YELLOW_BLINK_FAST  },
        { EVT_LED_BLINK_RED_GREEN    , MSG_LED_BLINK_RED_GREEN    },
        { EVT_LED_BLINK_GREEN_YELLOW , MSG_LED_BLINK_GREEN_YELLOW },
        { EVT_LED_BLINK_YELLOW_RED   , MSG_LED_BLINK_YELLOW_RED   },
        { EVT_LED_FINISH             , MSG_LED_FINISH             },
        { EVT_LED_NONE               , NULL                       },
    };

    for (i = 0; EVT_TABLE[i].msg != NULL; i++) {
        if (strcmp(msg, EVT_TABLE[i].msg) == 0) {
            evt = EVT_TABLE[i].evt;
        }
    }

#if defined(DEBUG_LOGOUT_STDIO) || defined(DEBUG_LOGOUT_FILE)
    if (evt >= 0) {
        PWS_DEBUG("ledGetEvent %d [%s]\n", evt, strEvt[evt]);
    }
    else {
        PWS_DEBUG("ledGetEvent %d [（イベント無し）]\n", evt);
    }
#endif
    
    return evt;
}

// ソケットをクローズ
static void ledCloseSocket(void)
{
    if (sockRcv >= 0) {
        close(sockRcv);
        sockRcv = -1;
    }
}

// メッセージを自身へ送信
static int ledSendMessageToMyself(char *msg)
{
    int n, sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        PWS_DEBUG("ERROR: socket\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port        = htons(PWS_PORT_LED_CONTROLLER);
    n = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n == -1) {
        PWS_DEBUG("ERROR: sendto\n");
        close(sock);
        return -1;
    }

    close(sock);

    return 0;
}

