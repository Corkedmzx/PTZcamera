#include "pwm.h"

/************************************
引脚连接说明
LED0连接在PF9,低电平灯亮；高电平，灯灭
TIM14_CH1(TIM14 -- APB1 16位  84MHZ)
*************************************/
void Pwm_PF9_Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;


	
	//使能定时器14
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14, ENABLE);

	//使能GPIOF组时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);


	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;   //复用模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(GPIOF, &GPIO_InitStructure); 

	//引脚映射  选定引脚与外设关系
	GPIO_PinAFConfig(GPIOF, GPIO_PinSource9, GPIO_AF_TIM14);


	
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period 	= (1000-1); //Period(ARR)
	TIM_TimeBaseStructure.TIM_Prescaler = 83; //84分频 1MHZ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM14, &TIM_TimeBaseStructure);

	/* PWM1 Mode configuration: Channel1 */
	TIM_OCInitStructure.TIM_OCMode 		= TIM_OCMode_PWM1; //模式1
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse 		= 0; //捕获/比较器的初始值
	TIM_OCInitStructure.TIM_OCPolarity 	= TIM_OCPolarity_Low;//输出极性，这里设置为低
	//通道1对应的是OC1
	TIM_OC1Init(TIM14, &TIM_OCInitStructure);

	//使能预装载寄存器：
	TIM_OC1PreloadConfig(TIM14, TIM_OCPreload_Enable);

	//使能自动重装载的预装载寄存器允许位    
	TIM_ARRPreloadConfig(TIM14, ENABLE);

	/* TIM14 enable counter */
	TIM_Cmd(TIM14, ENABLE);

}

void PWM_SetDuty(uint16_t duty)
{
	// duty 范围：0~ARR(999)
	if (duty > 999)
		duty = 999;
	TIM_SetCompare1(TIM14, duty);
}

