#ifndef _TIM1_H
#define _TIM1_H

#include "stm32f4xx.h"

void TIM1_Init(void);
// TIM1 Update 与 TIM10 共用该中断入口
void TIM1_UP_TIM10_IRQHandler(void);

#endif
