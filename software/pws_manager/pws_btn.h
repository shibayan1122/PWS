///////////////////////////////////////////////////////////
// pws_btn.h
///////////////////////////////////////////////////////////
#ifndef __PWS_BTN_H__
#define __PWS_BTN_H__

#define MSG_PUSH_AP_SET_BTN     "/btnmonitor/push/apset"        // AP設定ボタン押下
#define MSG_PUSH_REC_BTN        "/btnmonitor/push/recbtn"       // 録音ボタン押下
#define MSG_PUSH_PLAY_BTN       "/btnmonitor/push/playbtn"      // 再生ボタン押下
#define MSG_PUSH_VOL_UP_BTN     "/btnmonitor/push/volupbtn"     // ボリュームアップボタン押下
#define MSG_PUSH_VOL_DOWN_BTN   "/btnmonitor/push/voldownbtn"   // ボリュームダウンボタン押下
#define MSG_PUSH_EFFECT_BTN     "/btnmonitor/push/effectbtn"    // エフェクトボタン押下
#define MSG_PUSH_TUNING_BTN     "/btnmonitor/push/tuningbtn"    // チューニングボタン押下
#define MSG_PUSH_SHUTDOWN_BTN   "/btnmonitor/push/shutdownbtn"  // シャットダウンボタン押下

//
// スイッチ監視初期化
//
extern int btnInitialize(void);

//
// スイッチ監視終了処理
//
extern void btnFinish(void);

#endif // __PWS_BTN_H__
