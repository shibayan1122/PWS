///////////////////////////////////////////////////////////
// pws_led.h
///////////////////////////////////////////////////////////
#ifndef __PWS_LED_H__
#define __PWS_LED_H__

//
// ポート番号定義
//                                            (モジュール名)       (ポート番号)
#define PWS_PORT_LED_CONTROLLER     (9001)  // LED Controller       9001

#define MSG_LED_RED_OFF             "/led/red/off"
#define MSG_LED_RED_ON              "/led/red/on"
#define MSG_LED_RED_BLINK           "/led/red/blink"
#define MSG_LED_RED_BLINK_FAST      "/led/red/blink/fast"
#define MSG_LED_GREEN_OFF           "/led/green/off"
#define MSG_LED_GREEN_ON            "/led/green/on"
#define MSG_LED_GREEN_BLINK         "/led/green/blink"
#define MSG_LED_GREEN_BLINK_FAST    "/led/green/blink/fast"
#define MSG_LED_YELLOW_OFF          "/led/orange/off"
#define MSG_LED_YELLOW_ON           "/led/orange/on"
#define MSG_LED_YELLOW_BLINK        "/led/orange/blink"
#define MSG_LED_YELLOW_BLINK_FAST   "/led/orange/blink/fast"
#define MSG_LED_BLINK_RED_GREEN     "/led/blink/red/green"
#define MSG_LED_BLINK_GREEN_YELLOW  "/led/blink/green/orange"
#define MSG_LED_BLINK_YELLOW_RED    "/led/blink/orange/red"
#define MSG_LED_FINISH              "/led/finish"

//
// LED制御初期化
//
extern int ledInitialize(void);

//
// LED制御終了処理
//
extern void ledFinish(void);

#endif // __PWS_LED_H__
