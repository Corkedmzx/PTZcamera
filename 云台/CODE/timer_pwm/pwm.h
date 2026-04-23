#ifndef __PWM_H
#define __PWM_H

#include "stm32f4xx.h"

void Pwm_PF9_Init(void);
void PWM_SetDuty(uint16_t duty);

#endif
