///////////////////////////////////////////////////////////
// pws_manager.c
///////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "pws_manager.h"
#include "pws_osc.h"
#include "pws_gpio.h"
#include "pws_led.h"
#include "pws_debug.h"

//
// ステートマシンの状態
//
typedef enum {
    STATE_INIT = 0              ,   // 初期化中
    STATE_APSET                 ,   // AP設定モード中
    STATE_APSET_WAIT            ,   // AP設定終了待ち
    STATE_PD_WAIT               ,   // PD初期化終了待ち
    STATE_IDLE                  ,   // アイドル
    STATE_REC                   ,   // 録音中
    STATE_PLAY                  ,   // 再生中
    STATE_TUNE                  ,   // チューニング中
    STATE_MAX                       // 最大個数
} STATE;

//
// イベント
//
typedef enum {
    EVT_NONE       = -1         ,   // イベントなし
    EVT_INITIALIZE              ,   // 起動
    EVT_PUSH_AP_SET_BTN         ,   // AP設定ボタン押下
    EVT_RECV_AP_CONFIGURED      ,   // AP設定終了通知
    EVT_RECV_AP_CONFIG_ERROR    ,   // AP設定異常通知
    EVT_RECV_PD_INIT_FINISHED   ,   // PD初期化終了通知
    EVT_RECV_PD_INIT_ERROR      ,   // PD初期化異常通知
    EVT_PUSH_REC_BTN            ,   // 録音ボタン押下
    EVT_RECV_REC_STOPPED        ,   // 録音終了通知
    EVT_PUSH_PLAY_BTN           ,   // 再生ボタン押下
    EVT_RECV_PLAY_STOPPED       ,   // 再生終了通知
    EVT_PUSH_VOL_UP_BTN         ,   // ボリュームアップボタン押下
    EVT_PUSH_VOL_DOWN_BTN       ,   // ボリュームダウンボタン押下
    EVT_PUSH_EFFECT_BTN         ,   // エフェクトボタン押下
    EVT_PUSH_TUNING_BTN         ,   // チューニングボタン押下
    EVT_RECV_TUNING_STOPPED     ,   // チューニング終了通知
    EVT_RECV_TUNING_COND        ,   // チューニング状態通知
    EVT_RECV_UPLOAD_STARTED     ,   // アップロード開始通知
    EVT_RECV_UPLOAD_STOPPED     ,   // アップロード終了通知
    EVT_RECV_DOWNLOAD_STOPPED   ,   // ダウンロード終了通知
    EVT_PUSH_SHUTDOWN_BTN       ,   // シャットダウンボタン押下 
    EVT_MAX                         // イベント最大個数
} EVENT;

//
// マネージャーのコンテキスト（状態管理）
//
static struct {
    int state;
    int event;
} MgrCtx;

//
// 関数のプロトタイプ宣言
//
static int mgrInitialize(int code, void *arg1, void *arg2);
static int mgrApConfigure(int code, void *arg1, void *arg2);
static int mgrApConfigured(int code, void *arg1, void *arg2);
static int mgrApConfigError(int code, void *arg1, void *arg2);
static int mgrPdInitFinished(int code, void *arg1, void *arg2);
static int mgrPdInitError(int code, void *arg1, void *arg2);
static int mgrRecStart(int code, void *arg1, void *arg2);
static int mgrRecStop(int code, void *arg1, void *arg2);
static int mgrRecStopped(int code, void *arg1, void *arg2);
static int mgrPlayStart(int code, void *arg1, void *arg2);
static int mgrPlayStop(int code, void *arg1, void *arg2);
static int mgrPlayStopped(int code, void *arg1, void *arg2);
static int mgrVolumeUp(int code, void *arg1, void *arg2);
static int mgrVolumeDown(int code, void *arg1, void *arg2);
static int mgrEffectChange(int code, void *arg1, void *arg2);
static int mgrTuningStart(int code, void *arg1, void *arg2);
static int mgrTuningStop(int code, void *arg1, void *arg2);
static int mgrTuningStopped(int code, void *arg1, void *arg2);
static int mgrTuningCond(int code, void *arg1, void *arg2);
static int mgrUploadStarted(int code, void *arg1, void *arg2);
static int mgrUploadStopped(int code, void *arg1, void *arg2);
static int mgrDownloadStopped(int code, void *arg1, void *arg2);
static int mgrShutdown(int code, void *arg1, void *arg2);

static int mgrGetEvent(char *buf, int len, OSC_MESSAGE *msg);
static int mgrSendMessageToSndModule(int port, char *msg, char *param);
static int mgrSendMessageToLedController(char *msg);
static void mgrCloseSocket(void);
static void mgrSigHandler(int sig);

#if defined(DEBUG_LOGOUT_STDIO) || defined(DEBUG_LOGOUT_FILE)
static void mgrDebugOut(char *buf, int len);
#endif

//
// 状態遷移テーブル
//
static struct {
    int next;
    int (*func)(int code, void *arg1, void *arg2);
} STATE_TABLE[STATE_MAX][EVT_MAX] = {
    //
    // STATE_INIT：初期化中
    //
    {
        { STATE_PD_WAIT   , mgrInitialize       }, // 起動
        { STATE_APSET     , mgrApConfigure      }, // AP設定ボタン押下
        { STATE_INIT      , NULL                }, // AP設定終了通知
        { STATE_INIT      , NULL                }, // AP設定異常通知
        { STATE_IDLE      , mgrPdInitFinished   }, // PD初期化終了通知
        { STATE_INIT      , mgrPdInitError      }, // PD初期化異常通知
        { STATE_INIT      , NULL                }, // 録音ボタン押下
        { STATE_INIT      , NULL                }, // 録音終了通知
        { STATE_INIT      , NULL                }, // 再生ボタン押下
        { STATE_INIT      , NULL                }, // 再生終了通知
        { STATE_INIT      , NULL                }, // ボリュームアップボタン押下
        { STATE_INIT      , NULL                }, // ボリュームダウンボタン押下
        { STATE_INIT      , NULL                }, // エフェクトボタン押下
        { STATE_INIT      , NULL                }, // チューニングボタン押下
        { STATE_INIT      , NULL                }, // チューニング終了通知
        { STATE_INIT      , NULL                }, // チューニング状態通知
        { STATE_INIT      , NULL                }, // アップロード開始通知
        { STATE_INIT      , NULL                }, // アップロード終了通知
        { STATE_INIT      , NULL                }, // ダウンロード終了通知
        { STATE_INIT      , mgrShutdown         }, // シャットダウンボタン押下
    },

    //
    // STATE_APSET：AP設定モード中
    //
    {
        { STATE_APSET     , NULL                }, // 起動
        { STATE_APSET     , NULL                }, // AP設定ボタン押下
        { STATE_PD_WAIT   , mgrApConfigured     }, // AP設定終了通知
        { STATE_APSET     , mgrApConfigError    }, // AP設定異常通知
        { STATE_APSET_WAIT, mgrPdInitFinished   }, // PD初期化終了通知
        { STATE_APSET     , mgrPdInitError      }, // PD初期化異常通知
        { STATE_APSET     , NULL                }, // 録音ボタン押下
        { STATE_APSET     , NULL                }, // 録音終了通知
        { STATE_APSET     , NULL                }, // 再生ボタン押下
        { STATE_APSET     , NULL                }, // 再生終了通知
        { STATE_APSET     , NULL                }, // ボリュームアップボタン押下
        { STATE_APSET     , NULL                }, // ボリュームダウンボタン押下
        { STATE_APSET     , NULL                }, // エフェクトボタン押下
        { STATE_APSET     , NULL                }, // チューニングボタン押下
        { STATE_APSET     , NULL                }, // チューニング終了通知
        { STATE_APSET     , NULL                }, // チューニング状態通知
        { STATE_APSET     , NULL                }, // アップロード開始通知
        { STATE_APSET     , NULL                }, // アップロード終了通知
        { STATE_APSET     , NULL                }, // ダウンロード終了通知
        { STATE_INIT      , mgrShutdown         }, // シャットダウンボタン押下
    },

    //
    // STATE_APSET_WAIT：AP設定終了待ち
    //
    {
        { STATE_APSET_WAIT, NULL                }, // 起動
        { STATE_APSET_WAIT, NULL                }, // AP設定ボタン押下
        { STATE_IDLE      , mgrApConfigured     }, // AP設定終了通知
        { STATE_APSET_WAIT, mgrApConfigError    }, // AP設定異常通知
        { STATE_APSET_WAIT, NULL                }, // PD初期化終了通知
        { STATE_APSET_WAIT, NULL                }, // PD初期化異常通知
        { STATE_APSET_WAIT, NULL                }, // 録音ボタン押下
        { STATE_APSET_WAIT, NULL                }, // 録音終了通知
        { STATE_APSET_WAIT, NULL                }, // 再生ボタン押下
        { STATE_APSET_WAIT, NULL                }, // 再生終了通知
        { STATE_APSET_WAIT, NULL                }, // ボリュームアップボタン押下
        { STATE_APSET_WAIT, NULL                }, // ボリュームダウンボタン押下
        { STATE_APSET_WAIT, NULL                }, // エフェクトボタン押下
        { STATE_APSET_WAIT, NULL                }, // チューニングボタン押下
        { STATE_APSET_WAIT, NULL                }, // チューニング終了通知
        { STATE_APSET_WAIT, NULL                }, // チューニング状態通知
        { STATE_APSET_WAIT, NULL                }, // アップロード開始通知
        { STATE_APSET_WAIT, NULL                }, // アップロード終了通知
        { STATE_APSET_WAIT, NULL                }, // ダウンロード終了通知
        { STATE_INIT      , mgrShutdown         }, // シャットダウンボタン押下
    },

    //
    // STATE_PD_WAIT：PD初期化終了待ち
    //
    {
        { STATE_PD_WAIT   , NULL                }, // 起動
        { STATE_PD_WAIT   , NULL                }, // AP設定ボタン押下
        { STATE_PD_WAIT   , NULL                }, // AP設定終了通知
        { STATE_PD_WAIT   , NULL                }, // AP設定異常通知
        { STATE_IDLE      , mgrPdInitFinished   }, // PD初期化終了通知
        { STATE_PD_WAIT   , mgrPdInitError      }, // PD初期化異常通知
        { STATE_PD_WAIT   , NULL                }, // 録音ボタン押下
        { STATE_PD_WAIT   , NULL                }, // 録音終了通知
        { STATE_PD_WAIT   , NULL                }, // 再生ボタン押下
        { STATE_PD_WAIT   , NULL                }, // 再生終了通知
        { STATE_PD_WAIT   , NULL                }, // ボリュームアップボタン押下
        { STATE_PD_WAIT   , NULL                }, // ボリュームダウンボタン押下
        { STATE_PD_WAIT   , NULL                }, // エフェクトボタン押下
        { STATE_PD_WAIT   , NULL                }, // チューニングボタン押下
        { STATE_PD_WAIT   , NULL                }, // チューニング終了通知
        { STATE_PD_WAIT   , NULL                }, // チューニング状態通知
        { STATE_PD_WAIT   , NULL                }, // アップロード開始通知
        { STATE_PD_WAIT   , NULL                }, // アップロード終了通知
        { STATE_PD_WAIT   , NULL                }, // ダウンロード終了通知
        { STATE_INIT      , mgrShutdown         }, // シャットダウンボタン押下
    },

    //
    // STATE_IDLE：アイドル中
    //
    {
        { STATE_IDLE      , NULL                }, // 起動
        { STATE_IDLE      , NULL                }, // AP設定ボタン押下
        { STATE_IDLE      , NULL                }, // AP設定終了通知
        { STATE_IDLE      , NULL                }, // AP設定異常通知
        { STATE_IDLE      , NULL                }, // PD初期化終了通知
        { STATE_IDLE      , NULL                }, // PD初期化異常通知
        { STATE_REC       , mgrRecStart         }, // 録音ボタン押下
        { STATE_IDLE      , NULL                }, // 録音終了通知
        { STATE_PLAY      , mgrPlayStart        }, // 再生ボタン押下
        { STATE_IDLE      , NULL                }, // 再生終了通知
        { STATE_IDLE      , mgrVolumeUp         }, // ボリュームアップボタン押下
        { STATE_IDLE      , mgrVolumeDown       }, // ボリュームダウンボタン押下
        { STATE_IDLE      , mgrEffectChange     }, // エフェクトボタン押下
        { STATE_TUNE      , mgrTuningStart      }, // チューニングボタン押下
        { STATE_IDLE      , NULL                }, // チューニング終了通知
        { STATE_IDLE      , NULL                }, // チューニング状態通知
        { STATE_IDLE      , mgrUploadStarted    }, // アップロード開始通知
        { STATE_IDLE      , mgrUploadStopped    }, // アップロード終了通知
        { STATE_IDLE      , mgrDownloadStopped  }, // ダウンロード終了通知
        { STATE_INIT      , mgrShutdown         }, // シャットダウンボタン押下
    },

    //
    // STATE_REC：録音中
    //
    {
        { STATE_REC       , NULL                }, // 起動
        { STATE_REC       , NULL                }, // AP設定ボタン押下
        { STATE_REC       , NULL                }, // AP設定終了通知
        { STATE_REC       , NULL                }, // AP設定異常通知
        { STATE_REC       , NULL                }, // PD初期化終了通知
        { STATE_REC       , NULL                }, // PD初期化異常通知
        { STATE_REC       , mgrRecStop          }, // 録音ボタン押下
        { STATE_IDLE      , mgrRecStopped       }, // 録音終了通知
        { STATE_REC       , NULL                }, // 再生ボタン押下
        { STATE_REC       , NULL                }, // 再生終了通知
        { STATE_REC       , mgrVolumeUp         }, // ボリュームアップボタン押下
        { STATE_REC       , mgrVolumeDown       }, // ボリュームダウンボタン押下
        { STATE_REC       , mgrEffectChange     }, // エフェクトボタン押下
        { STATE_REC       , NULL                }, // チューニングボタン押下
        { STATE_REC       , NULL                }, // チューニング終了通知
        { STATE_REC       , NULL                }, // チューニング状態通知
        { STATE_REC       , mgrUploadStarted    }, // アップロード開始通知
        { STATE_REC       , mgrUploadStopped    }, // アップロード終了通知
        { STATE_REC       , mgrDownloadStopped  }, // ダウンロード終了通知
        { STATE_INIT      , mgrShutdown         }, // シャットダウンボタン押下
    },

    //
    // STATE_PLAY：再生中
    //
    {
        { STATE_PLAY      , NULL                }, // 起動
        { STATE_PLAY      , NULL                }, // AP設定ボタン押下
        { STATE_PLAY      , NULL                }, // AP設定終了通知
        { STATE_PLAY      , NULL                }, // AP設定異常通知
        { STATE_PLAY      , NULL                }, // PD初期化終了通知
        { STATE_PLAY      , NULL                }, // PD初期化異常通知
        { STATE_PLAY      , NULL                }, // 録音ボタン押下
        { STATE_PLAY      , NULL                }, // 録音終了通知
        { STATE_PLAY      , mgrPlayStop         }, // 再生ボタン押下
        { STATE_IDLE      , mgrPlayStopped      }, // 再生終了通知
        { STATE_PLAY      , mgrVolumeUp         }, // ボリュームアップボタン押下
        { STATE_PLAY      , mgrVolumeDown       }, // ボリュームダウンボタン押下
        { STATE_PLAY      , mgrEffectChange     }, // エフェクトボタン押下
        { STATE_PLAY      , NULL                }, // チューニングボタン押下
        { STATE_PLAY      , NULL                }, // チューニング終了通知
        { STATE_PLAY      , NULL                }, // チューニング状態通知
        { STATE_PLAY      , mgrUploadStarted    }, // アップロード開始通知
        { STATE_PLAY      , mgrUploadStopped    }, // アップロード終了通知
        { STATE_PLAY      , mgrDownloadStopped  }, // ダウンロード終了通知
        { STATE_INIT      , mgrShutdown         }, // シャットダウンボタン押下
    },

    //
    // STATE_TUNE：チューニング中
    //
    {
        { STATE_TUNE      , NULL                }, // 起動
        { STATE_TUNE      , NULL                }, // AP設定ボタン押下
        { STATE_TUNE      , NULL                }, // AP設定終了通知
        { STATE_TUNE      , NULL                }, // AP設定異常通知
        { STATE_TUNE      , NULL                }, // PD初期化終了通知
        { STATE_TUNE      , NULL                }, // PD初期化異常通知
        { STATE_TUNE      , NULL                }, // 録音ボタン押下
        { STATE_TUNE      , NULL                }, // 録音終了通知
        { STATE_TUNE      , NULL                }, // 再生ボタン押下
        { STATE_TUNE      , NULL                }, // 再生終了通知
        { STATE_TUNE      , mgrVolumeUp         }, // ボリュームアップボタン押下
        { STATE_TUNE      , mgrVolumeDown       }, // ボリュームダウンボタン押下
        { STATE_TUNE      , mgrEffectChange     }, // エフェクトボタン押下
        { STATE_TUNE      , mgrTuningStop       }, // チューニングボタン押下
        { STATE_IDLE      , mgrTuningStopped    }, // チューニング終了通知
        { STATE_TUNE      , mgrTuningCond       }, // チューニング状態通知
        { STATE_TUNE      , mgrUploadStarted    }, // アップロード開始通知
        { STATE_TUNE      , mgrUploadStopped    }, // アップロード終了通知
        { STATE_TUNE      , mgrDownloadStopped  }, // ダウンロード終了通知
        { STATE_INIT      , mgrShutdown         }, // シャットダウンボタン押下
    },
};

#if defined(DEBUG_LOGOUT_STDIO) || defined(DEBUG_LOGOUT_FILE)
//
// ステート文字列
//
static char *strState[] = {
    "初期化中",
    "AP設定モード中",
    "AP設定終了待ち",
    "PD初期化終了待ち",
    "アイドル",
    "録音中",
    "再生中",
    "チューニング中",
};

//
// イベント文字列
//
static char *strEvt[] = {
    "起動",
    "AP設定ボタン押下",
    "AP設定終了通知",
    "AP設定異常通知",
    "PD初期化終了通知",
    "PD初期化異常通知",
    "録音ボタン押下",
    "録音終了通知",
    "再生ボタン押下",
    "再生終了通知",
    "ボリュームアップボタン押下",
    "ボリュームダウンボタン押下",
    "エフェクトボタン押下",
    "チューニングボタン押下",
    "チューニング終了通知",
    "チューニング状態通知",
    "アップロード開始通知",
    "アップロード終了通知",
    "ダウンロード終了通知",
    "シャットダウンボタン押下 ",
};
#endif

static int sock        = -1;
static int mainFinish  =  0;
static pthread_mutex_t mainMutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
    int rc, n, evt, next, ret, loop;
    char buf[RECV_BUF_SIZE];
    struct sockaddr_in addr;
    int (*func)(int code, void *arg1, void *arg2);
    OSC_MESSAGE oscMsg;

    PWS_DEBUG("START\n");

    // マネージャーのコンテキスト初期化
    memset(&MgrCtx, 0, sizeof(MgrCtx));

    // GPIO初期化
    gpioInitialize();

    // シグナルの設定
    signal(SIGTERM, mgrSigHandler);
    signal(SIGINT , mgrSigHandler);
    signal(SIGKILL, mgrSigHandler);
 
    // ソケット作成
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        PWS_DEBUG("Socket Error\n");
        gpioFinish();
        return 1;
    }
    PWS_DEBUG("sock    %d\n", sock);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PWS_PORT_MANAGER);

    rc = bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == -1) {
        gpioFinish();
        close(sock);
        sock = -1;
        PWS_DEBUG("Bind Error\n");
        return 2;
    }

    // 起動モードの判定
    gpioCheckApMode();

    loop = 1;
    while (loop) {
        memset(&oscMsg, 0, sizeof(oscMsg));
        memset(buf, 0, sizeof(buf));
        n = recv(sock, buf, sizeof(buf) - 1, 0);
        if (n < 0) {
            PWS_DEBUG("ERROR: recv\n");
            break;
        }
#if defined(DEBUG_LOGOUT_STDIO) || defined(DEBUG_LOGOUT_FILE)
        mgrDebugOut(buf, n);
#endif
        evt = mgrGetEvent(buf, n, &oscMsg);
#if defined(DEBUG_LOGOUT_STDIO) || defined(DEBUG_LOGOUT_FILE)
        if (evt >= 0) {
            PWS_DEBUG("evt=%2d [%s]\n", evt, strEvt[evt]);
        }
        else {
            PWS_DEBUG("evt=%2d [（イベント無し）]\n", evt);
        }
#endif
        if (evt >= 0) {
            next = STATE_TABLE[MgrCtx.state][evt].next;
            func = STATE_TABLE[MgrCtx.state][evt].func;
            if (func != NULL) {
                ret = func(oscMsg.data[0].u.i, oscMsg.data[1].u.s, oscMsg.data[2].u.s);
                if (ret < 0) {
                    // func error !!
                    PWS_DEBUG("ERROR: func()[%s][%s]\n", strState[MgrCtx.state], strEvt[evt]);
                }
            }
            PWS_DEBUG("state  [%s] --> [%s]\n", strState[MgrCtx.state], strState[next]);
            MgrCtx.state = next;
        }

        pthread_mutex_lock(&mainMutex);
        if (mainFinish == 1) {
            loop = 0;
        }
        pthread_mutex_unlock(&mainMutex);
    }

    gpioFinish();

    mgrCloseSocket();

    return 0;
}


//
// アクション関数
//


// 初期化
static int  mgrInitialize(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    // LED 設定（黄色点灯）
    mgrSendMessageToLedController(MSG_LED_RED_OFF);
    mgrSendMessageToLedController(MSG_LED_GREEN_OFF);
    mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK);

    return 0;
}

// AP設定開始
static int mgrApConfigure(int code, void *arg1, void *arg2)
{
    static const char *cmd = PWS_CMD_AP_CONFIG;
        
    PWS_DEBUG("action: %s\n", __func__);
    
    // LED 設定（緑色点滅）
    mgrSendMessageToLedController(MSG_LED_RED_OFF);
    mgrSendMessageToLedController(MSG_LED_GREEN_BLINK);
    mgrSendMessageToLedController(MSG_LED_YELLOW_OFF);

    system(cmd);

    return 0;
}

// AP設定終了通知受信
static int mgrApConfigured(int code, void *arg1, void *arg2)
{
    static const char *cmd = PWS_CMD_WEB_RESTART;

	PWS_DEBUG("action: %s\n", __func__);

    mgrSendMessageToLedController(MSG_LED_GREEN_OFF);
    switch(MgrCtx.state) {
    case STATE_APSET:
        // LED 設定（黄色点滅）
        mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK);
        break;
    case STATE_APSET_WAIT:
        // LED 設定（黄色点灯）
        mgrSendMessageToLedController(MSG_LED_YELLOW_ON);
        // WEBサーバー再起動
	    system(cmd);
        break;
    default:
        break;
    }

    return 0;
}

// AP設定異常通知
static int mgrApConfigError(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    if (code < -1) {
        switch(MgrCtx.state) {
        case STATE_APSET:
	        // LED 設定（黄色早点滅）
            mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK_FAST);
            break;
        case STATE_APSET_WAIT:
	        // LED 設定（黄色早点滅）
            mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK_FAST);
            break;
        default:
            break;
        }
    }

    return 0;
}

// PD初期化終了通知
static int mgrPdInitFinished(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    switch(MgrCtx.state) {
    case STATE_APSET:
        break;
    case STATE_PD_WAIT:
        // LED 設定（黄色点灯）
        mgrSendMessageToLedController(MSG_LED_YELLOW_ON);
        break;
    default:
        break;
    }

    return 0;
}

// PD初期化異常通知
static int mgrPdInitError(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    switch(MgrCtx.state) {
    case STATE_APSET:
		// LED 設定（黄色早点滅）
        mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK_FAST);
        break;
    case STATE_PD_WAIT:
		// LED 設定（黄色早点滅）
        mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK_FAST);
        break;
    default:
        break;
    }

    return 0;
}

// 録音開始
static int  mgrRecStart(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    // LED 設定（赤色点灯）
    mgrSendMessageToLedController(MSG_LED_RED_ON);
    // LED 設定（緑色消灯）
    mgrSendMessageToLedController(MSG_LED_GREEN_OFF);
    // LED 設定（黄色点灯）
    mgrSendMessageToLedController(MSG_LED_YELLOW_ON);

    mgrSendMessageToSndModule(PWS_PORT_RECORDER, MSG_REC_START, NULL);
    
    return 0;
}

// 録音終了
static int  mgrRecStop(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    mgrSendMessageToSndModule(PWS_PORT_RECORDER, MSG_REC_STOP, NULL);

    return 0;
}

// 録音終了通知受信
static int  mgrRecStopped(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    // LED 設定（赤色消灯）
    mgrSendMessageToLedController(MSG_LED_RED_OFF);
    if (code == 0) {
        mgrSendMessageToSndModule(PWS_PORT_FILE_UPLOADER, MSG_UPLOAD_START, arg1);
    }
    else {
		// LED 設定（黄色早点滅）
        mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK_FAST);
    }

    return 0;
}

// 再生開始
static int  mgrPlayStart(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    // LED 設定（赤色消灯）
    mgrSendMessageToLedController(MSG_LED_RED_OFF);
    // LED 設定（緑色点灯）
    mgrSendMessageToLedController(MSG_LED_GREEN_ON);
    // LED 設定（黄色点灯）
    mgrSendMessageToLedController(MSG_LED_YELLOW_ON);

    mgrSendMessageToSndModule(PWS_PORT_PLAYER, MSG_PLAY_START, NULL);

    return 0;
}

// 再生終了
static int  mgrPlayStop(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    mgrSendMessageToSndModule(PWS_PORT_PLAYER, MSG_PLAY_STOP, NULL);

    return 0;
}

// 再生終了通知受信
static int  mgrPlayStopped(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    // LED 設定（緑色消灯）
    mgrSendMessageToLedController(MSG_LED_GREEN_OFF);
    if (code != 0) {
		// LED 設定（黄色早点滅）
        mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK_FAST);
    }

    return 0;
}

// ボリュームアップ
static int  mgrVolumeUp(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);
    
    mgrSendMessageToSndModule(PWS_PORT_AUDIO_OUT, MSG_VOL_UP, NULL);

    return 0;
}

// ボリュームダウン
static int  mgrVolumeDown(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    mgrSendMessageToSndModule(PWS_PORT_AUDIO_OUT, MSG_VOL_DOWN, NULL);

    return 0;
}

// エフェクト変更
static int  mgrEffectChange(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    mgrSendMessageToSndModule(PWS_PORT_EFFECT_CONTROLLER, MSG_EFFECT_CHANGE, NULL);

    return 0;
}

// チューニング開始
static int  mgrTuningStart(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);
    
    // LED 設定（赤緑交互点滅）
    mgrSendMessageToLedController(MSG_LED_BLINK_RED_GREEN);
    // LED 設定（黄色点灯）
    mgrSendMessageToLedController(MSG_LED_YELLOW_ON);

    mgrSendMessageToSndModule(PWS_PORT_TUNER, MSG_TUNING_START, NULL);

    return 0;
}

// チューニング終了
static int  mgrTuningStop(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    mgrSendMessageToSndModule(PWS_PORT_TUNER, MSG_TUNING_STOP, NULL);

    return 0;
}

// チューニング終了通知受信
static int  mgrTuningStopped(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);
    
    // LED 設定（赤色消灯）
    mgrSendMessageToLedController(MSG_LED_RED_OFF);
    // LED 設定（緑色消灯）
    mgrSendMessageToLedController(MSG_LED_GREEN_OFF);
    if (code != 0) {
		// LED 設定（黄色早点滅）
        mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK_FAST);
    }

    return 0;
}

// チューニング状態通知受信
static int  mgrTuningCond(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    return 0;
}

static int mgrUploadStarted(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    // LED 設定（黄色点滅）
    mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK);

	return 0;
}

// アップロード終了通知受信
static int mgrUploadStopped(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    if (code == 0) {
        // LED 設定（黄色点灯）
        mgrSendMessageToLedController(MSG_LED_YELLOW_ON);
    }
    else {
        // LED 設定（黄色早点滅）
        mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK_FAST);
    }

    return 0;
}

// ダウンロード終了通知受信
static int mgrDownloadStopped(int code, void *arg1, void *arg2)
{
    PWS_DEBUG("action: %s\n", __func__);

    if (code == 0) {
        // LED 設定（黄色点灯）
        mgrSendMessageToLedController(MSG_LED_YELLOW_ON);
    }
    else {
        // LED 設定（黄色早点滅）
        mgrSendMessageToLedController(MSG_LED_YELLOW_BLINK_FAST);
    }

    return 0;
}

// シャットダウン
static int mgrShutdown(int code, void *arg1, void *arg2)
{
    static const char *cmd = PWS_CMD_SHUTDOWN;

    PWS_DEBUG("action: %s\n", __func__);

    system(cmd);
    
    return 0;
}

// イベント取得
static int mgrGetEvent(char *buf, int len, OSC_MESSAGE *msg)
{
    int i, ret, evt = EVT_NONE;
    static struct {
        char* msg;
        int   evt;
    } EVT_TABLE[] = {
        { MSG_INITIALIZE        , EVT_INITIALIZE            },  // 起動
        { MSG_PUSH_AP_SET_BTN   , EVT_PUSH_AP_SET_BTN       },  // AP設定ボタン押下
        { MSG_AP_CONFIGURED     , EVT_RECV_AP_CONFIGURED    },  // AP設定終了通知
        { MSG_PD_INIT_FINISHED  , EVT_RECV_PD_INIT_FINISHED },  // PD初期化終了通知
        { MSG_PUSH_REC_BTN      , EVT_PUSH_REC_BTN          },  // 録音ボタン押下
        { MSG_REC_STOPPED       , EVT_RECV_REC_STOPPED      },  // 録音終了通知
        { MSG_PUSH_PLAY_BTN     , EVT_PUSH_PLAY_BTN         },  // 再生ボタン押下
        { MSG_PLAY_STOPPED      , EVT_RECV_PLAY_STOPPED     },  // 再生終了通知
        { MSG_PUSH_VOL_UP_BTN   , EVT_PUSH_VOL_UP_BTN       },  // ボリュームアップボタン押下
        { MSG_PUSH_VOL_DOWN_BTN , EVT_PUSH_VOL_DOWN_BTN     },  // ボリュームダウンボタン押下
        { MSG_PUSH_EFFECT_BTN   , EVT_PUSH_EFFECT_BTN       },  // エフェクトボタン押下
        { MSG_PUSH_TUNING_BTN   , EVT_PUSH_TUNING_BTN       },  // チューニングボタン押下
        { MSG_TUNING_STOPPED    , EVT_RECV_TUNING_STOPPED   },  // チューニング終了通知
        { MSG_TUNING_COND       , EVT_RECV_TUNING_COND      },  // チューニング状態通知
        { MSG_UPLOAD_STARTED    , EVT_RECV_UPLOAD_STARTED   },  // アップロード開始通知
        { MSG_UPLOAD_STOPPED    , EVT_RECV_UPLOAD_STOPPED   },  // アップロード終了通知
        { MSG_DOWNLOAD_STOPPED  , EVT_RECV_DOWNLOAD_STOPPED },  // ダウンロード終了通知
        { MSG_PUSH_SHUTDOWN_BTN , EVT_PUSH_SHUTDOWN_BTN     },  // シャットダウンボタン押下 
        { NULL                  , -1                        },
    };

    ret = oscDecode((uint8_t *)buf, len, msg);
    if (ret < 0) {
        PWS_DEBUG("oscDecode Error\n");
        return -1;
    }

	PWS_DEBUG("addr=[%s]\n\n", msg->addr);
    
    for (i = 0; EVT_TABLE[i].msg != NULL; i++) {
        if (strcmp(msg->addr, EVT_TABLE[i].msg) == 0) {
            evt = EVT_TABLE[i].evt;
        }
    }

    switch(evt) {
    case EVT_RECV_AP_CONFIGURED:
        if (msg->num > 0 && msg->data[0].type == 'i' && msg->data[0].u.i < 0) {
            evt = EVT_RECV_AP_CONFIG_ERROR;
        }
        break;
    case EVT_RECV_PD_INIT_FINISHED:
        if (msg->num > 0 && msg->data[0].type == 'i' && msg->data[0].u.i < 0) {
            evt = EVT_RECV_PD_INIT_ERROR;
        }
        break;
    default:
        break;
    }

    return evt;
}

// メッセージを SND モジュールへ送信
static int mgrSendMessageToSndModule(int port, char *msg, char *param)
{
    int n, ret, sock, len;
    uint8_t sendBuf[SEND_BUF_SIZE];
    struct sockaddr_in addr;
    OSC_MESSAGE oscMsg;

	PWS_DEBUG("mgrSendMessageToSndModule port=%d\n", port);

    memset(&oscMsg, 0, sizeof(oscMsg));

    oscMsg.addr = msg;
    if (param != NULL && strlen(param) > 0) {
        oscMsg.num  = 1;
        oscMsg.data[0].dlen = strlen(param);
        oscMsg.data[0].type = 's';
        oscMsg.data[0].u.s  = param;
    }
    else {
        oscMsg.num  = 0;
    }
    ret = oscEncode(&oscMsg, (uint8_t *)sendBuf, &len);
    if (ret < 0) {
        return -1;
    }

#if defined(DEBUG_LOGOUT_STDIO) || defined(DEBUG_LOGOUT_FILE)
    mgrDebugOut((char *)sendBuf, len);
#endif

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        PWS_DEBUG("ERROR: Socket\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port        = htons(port);
    n = sendto(sock, sendBuf, len, 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n == -1) {
        PWS_DEBUG("ERROR: Sendto\n");
        close(sock);
        return -1;
    }

    close(sock);

    return 0;
}

// メッセージを LED Controller へ送信
static int mgrSendMessageToLedController(char *msg)
{
    int n, sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        PWS_DEBUG("ERROR: Socket\n");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port        = htons(PWS_PORT_LED_CONTROLLER);
    n = sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n == -1) {
        PWS_DEBUG("ERROR: Sendto\n");
        close(sock);
        return -1;
    }

    close(sock);

    return 0;
}

// ソケットのクローズ
static void mgrCloseSocket(void)
{
    PWS_DEBUG("mgrCloseSocket sock=%d\n", sock);
    if (sock >= 0) {
        close(sock);
        sock = -1;
    }
}

// シグナル受信/処理
static void mgrSigHandler(int sig)
{
    PWS_DEBUG("mgrSigHandler\n");

    gpioFinish();

    // メイン終了
    pthread_mutex_lock(&mainMutex);
    mainFinish = 1;
    pthread_mutex_unlock(&mainMutex);

    mgrCloseSocket();
}

#if defined(DEBUG_LOGOUT_STDIO) || defined(DEBUG_LOGOUT_FILE)
static void mgrDebugOut(char *buf, int len)
{
    int i;
    char  disp[1024];
    char  _str[17];
    char *p = disp;
    char *end = disp + sizeof(disp);
    memset(disp, 0, sizeof(disp));
    memset(_str, 0, sizeof(_str));
    for (i = 0; i < len; i++) {
        if ((i + 1) % 16 > 0) {
            if (buf[i] >= 0x20 && buf[i] < 0x7F) {
                _str[i % 16] = buf[i];
            }
            else {
                _str[i % 16] = '.';
            }
            snprintf(p, end - p, "%02x ", buf[i]);
            p += 3;
        }
        else {
            if (buf[i] >= 0x20 && buf[i] < 0x7F) {
                _str[i % 16] = buf[i];
            }
            else {
                _str[i % 16] = '.';
            }
            snprintf(p, end - p, "%02x %s\n", buf[i], _str);
            p += (3 + 16 + 1);
            memset(_str, 0, sizeof(_str));
        }
    }
    if ((len % 16) > 0) {
        for (i = 0; i < 16 - (len % 16); i++) {
            snprintf(p, end - p, "%s", "   ");
            p += 3;
        }
        snprintf(p, end - p, "%s\n", _str);
    }
    PWS_DEBUG("recv len=%d\n"
        "00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n"
        "-----------------------------------------------\n"
        "%s\n", len, disp);
}
#endif

