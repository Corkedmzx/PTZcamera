#include "tim1.h"

/*
 * TIM1 已用于 LED2(PE13)/LED3(PE14) 的 CH3/CH4 PWM（见 led_group_pwm.c）。
 * 旧版 1ms 更新中断里翻转 LED0/3 的逻辑已移除，避免与 TIM14(TIM0)/TIM1 PWM 冲突。
 */
void TIM1_Init(void)
{
}
