///////////////////////////////////////////////////////////
//  def.h
///////////////////////////////////////////////////////////
#ifndef __DEF_H__
#define __DEF_H__

// 受信バッファーサイズ
#define RECV_BUF_SIZE               (1024)
#define SEND_BUF_SIZE               (1024)

// コマンド
#define PWS_CMD_AP_CONFIG           "/usr/bin/python /pws/py/ap_conf.py &"
#define PWS_CMD_WEB_RESTART         "/bin/systemctl restart pws-webserver"
#define PWS_CMD_SHUTDOWN            "/sbin/shutdown now -h"

//
// ポート番号定義
//                                            (モジュール名)       (ポート番号)
#define PWS_PORT_MANAGER            (8001)  // PWS Controller       8001
#define PWS_PORT_RECORDER           (8002)  // Recorder             8002
#define PWS_PORT_PLAYER             (8003)  // Player               8003
#define PWS_PORT_TUNER              (8004)  // Tuner                8004
#define PWS_PORT_EFFECT_CONTROLLER  (8005)  // Effect Controller    8005
#define PWS_PORT_AUDIO_OUT          (8006)  // Audio Out            8006
#define PWS_PORT_FILE_UPLOADER      (8100)  // File Uploader        8100
#define PWS_PORT_FILE_DOWNLOADER    (8101)  // File Downloader      8101
#define PWS_PORT_AP_CONFIGURATOR    (8200)  // AP Configurator      8200

//
// 通信メッセージ
//
#define MSG_PD_INIT_FINISHED    "/pd_initializer/initialize/finished"   // PD初期化終了土        (Pd Initializer    →  PWS Controller   ）
#define MSG_REC_START           "/recorder/record/start"                // 録音開始要求         （PWS Controller    →  Recorder         ）
#define MSG_REC_STOP            "/recorder/record/stop"                 // 録音終了要求         （PWS Controller    →  Recorder         ）
#define MSG_REC_STARTED         "/recorder/record/started"              // 録音開始通知         （Recorder          →  PWS Controller   ）
#define MSG_REC_STOPPED         "/recorder/record/stopped"              // 録音終了通知         （Recorder          →  PWS Controller   ）
#define MSG_PLAY_START          "/player/playback/start"                // 再生開始要求         （PWS Controller    →  Player           ）
#define MSG_PLAY_STOP           "/player/playback/stop"                 // 再生終了要求         （PWS Controller    →  Player           ）
#define MSG_PLAY_STARTED        "/player/playback/started"              // 再生開始通知         （Player            →  PWS Controller   ）
#define MSG_PLAY_STOPPED        "/player/playback/stopped"              // 再生終了通知         （Player            →  PWS Controller   ）
#define MSG_TUNING_START        "/tuner/tune/start"                     // チューニング開始要求 （PWS Controller    →  Tuner            ）
#define MSG_TUNING_STOP         "/tuner/tune/stop"                      // チューニング終了要求 （PWS Controller    →  Tuner            ）
#define MSG_TUNING_STARTED      "/tuner/tune/started"                   // チューニング開始通知 （Tuner             →  PWS Controller   ）
#define MSG_TUNING_STOPPED      "/tuner/tune/stopped"                   // チューニング終了通知 （Tuner             →  PWS Controller   ）
#define MSG_TUNING_COND         "/tuner/tune/cond"                      // チューニング状態通知 （Tuner             →  PWS Controller   ）
#define MSG_EFFECT_CHANGE       "/effector/effect/toggle"               // エフェクト変更要求   （PWS Controller    →  Effect Controller）
#define MSG_VOL_UP              "/audio_out/volume/up"                  // ボリュームアップ要求 （PWS Controller    →  Audio Out        ）
#define MSG_VOL_DOWN            "/audio_out/volume/down"                // ボリュームダウン要求 （PWS Controller    →  Audio Out        ）
#define MSG_UPLOAD_START        "/uploader/upload/start"                // アップロード開始要求 （PWS Controller    →  File Uploader    ）
#define MSG_UPLOAD_STARTED      "/uploader/upload/started"              // アップロード開始通知 （File Uploader     →  PWS Controller   ）
#define MSG_UPLOAD_STOP         "/uploader/upload/stop"                 // アップロード終了要求 （PWS Controller    →  File Uploader    ）
#define MSG_UPLOAD_STOPPED      "/uploader/upload/stopped"              // アップロード終了通知 （File Uploader     →  PWS Controller   ）
#define MSG_DOWNLOAD_START      "/downloader/download/start"            // ダウンロード開始要求 （PWS Controller    →  File Downloader  ）
#define MSG_DOWNLOAD_STOP       "/downloader/download/started"          // ダウンロード開始通知 （File Downloader   →  PWS Controller   ）
#define MSG_DOWNLOAD_STOPPED    "/downloader/download/stopped"          // ダウンロード終了通知 （File Downloader   →  PWS Controller   ）
#define MSG_AP_CONFIGURED       "/ap_configurator/configure/configured" // AP設定終了通知       （AP Configurator   →  PWS Controller   ）
#define MSG_SYSTEM_LED_SET      "/system/led/set"                       // システムLED操作      （anyone            →  PWS Controller   ）

#endif  // __DEF_H__
