#ifndef BOARD_KEYS_H
#define BOARD_KEYS_H

#include "stm32f4xx.h"

/*
 * KEY0=PA0  KEY1=PE2：横向角度 - / +
 * KEY2=PE3  KEY3=PE4：纵向角度 - / +（若板子无 PE3/PE4，请在 board_keys.c 改为实际引脚或注释掉对应 GPIO）
 */
void BoardKeys_Init(void);
/*
 * 每 1ms 调用一次：
 *  0 无事件
 *  1~4 对应 KEY0~KEY3 按下沿（单键）
 *  5 约定为「云台回初始位」组合键（见 board_keys_gec_m4.h，默认 K1+K2 同时按下）
 */
uint8_t BoardKeys_Poll1ms(void);

#endif
