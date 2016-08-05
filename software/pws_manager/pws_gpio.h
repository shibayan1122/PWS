///////////////////////////////////////////////////////////
// pws_gpio.h
///////////////////////////////////////////////////////////
#ifndef __PWS_GPIO_H__
#define __PWS_GPIO_H__

//
// wiringPi用ピン番号表
//
//  +-----+-----+---------+------+---+-Pi Zero--+---+------+---------+-----+-----+
//  | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
//  +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
//  |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
//  |   2 |   8 |   SDA.1 |   IN | 1 |  3 || 4  |   |      | 5V      |     |     |
//  |   3 |   9 |   SCL.1 |   IN | 1 |  5 || 6  |   |      | 0v      |     |     |
//  |   4 |   7 | GPIO. 7 |   IN | 1 |  7 || 8  | 1 | ALT0 | TxD     | 15  | 14  |
//  |     |     |      0v |      |   |  9 || 10 | 1 | ALT0 | RxD     | 16  | 15  |
//  |  17 |   0 | GPIO. 0 |   IN | 0 | 11 || 12 | 0 | IN   | GPIO. 1 | 1   | 18  |
//  |  27 |   2 | GPIO. 2 |   IN | 0 | 13 || 14 |   |      | 0v      |     |     |
//  |  22 |   3 | GPIO. 3 |   IN | 0 | 15 || 16 | 0 | IN   | GPIO. 4 | 4   | 23  |
//  |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
//  |  10 |  12 |    MOSI |   IN | 0 | 19 || 20 |   |      | 0v      |     |     |
//  |   9 |  13 |    MISO |   IN | 0 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
//  |  11 |  14 |    SCLK |   IN | 0 | 23 || 24 | 1 | IN   | CE0     | 10  | 8   |
//  |     |     |      0v |      |   | 25 || 26 | 1 | IN   | CE1     | 11  | 7   |
//  |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
//  |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
//  |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 0 | IN   | GPIO.26 | 26  | 12  |
//  |  13 |  23 | GPIO.23 |   IN | 0 | 33 || 34 |   |      | 0v      |     |     |
//  |  19 |  24 | GPIO.24 |   IN | 0 | 35 || 36 | 0 | IN   | GPIO.27 | 27  | 16  |
//  |  26 |  25 | GPIO.25 |   IN | 0 | 37 || 38 | 0 | IN   | GPIO.28 | 28  | 20  |
//  |     |     |      0v |      |   | 39 || 40 | 0 | IN   | GPIO.29 | 29  | 21  |
//  +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
//  | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
//  +-----+-----+---------+------+---+-Pi Zero--+---+------+---------+-----+-----+

// LEDのwPiピン番号
// ------------------------------------
// 名称     GPIOピン番号    wPiピン番号  
// LED1     4               7           赤色
// LED2     17              0           緑色
// LED3     12              26          黄色／起動状態を示す（起動している間ずっと光る）

#define PIN_LED_RED         (7)         // 赤色 LED
#define PIN_LED_GREEN       (0)         // 緑色 LED
#define PIN_LED_YELLOW      (26)        // 黄色 LED

// ボタンのwPiピン番号
// ------------------------------------
// 名称     GPIOピン番号    wPiピン番号
// ボタン1  5               21
// ボタン2  25              6
// ボタン3  24              5
// ボタン4  22              3
// ボタン5  23              4
// ボタン6  18              1
// ボタン7  27              2
#define PIN_BTN_1           (21)        // ボタン1
#define PIN_BTN_2           (6)         // ボタン2
#define PIN_BTN_3           (5)         // ボタン3
#define PIN_BTN_4           (3)         // ボタン4
#define PIN_BTN_5           (4)         // ボタン5
#define PIN_BTN_6           (1)         // ボタン6
#define PIN_BTN_7           (2)         // ボタン7

// ピンの値
#define PIN_VAL_ON          (1)
#define PIN_VAL_OFF         (0)


// スイッチ長押し時間（ミリ秒）
#define LONG_PUSH_TIME      (2000.0)

extern int gpioInitialize(void);
extern void gpioCheckApMode(void);
extern int gpioRead(int pin);
extern void gpioWrite(int pin, int val);
extern void gpioFinish(void);

#endif // __PWS_GPIO_H__
