#include "board_keys.h"
#include "board_keys_gec_m4.h"
#include "stm32f4xx_gpio.h"

#define KEY0_GPIO    GPIOA
#define KEY0_PIN     GPIO_Pin_0
#define KEY0_RCC     RCC_AHB1Periph_GPIOA

#define KEY1_GPIO    GPIOE
#define KEY1_PIN     GPIO_Pin_2
#define KEY1_RCC     RCC_AHB1Periph_GPIOE

#define KEY2_GPIO    GPIOE
#define KEY2_PIN     GPIO_Pin_3
#define KEY2_RCC     RCC_AHB1Periph_GPIOE

#define KEY3_GPIO    GPIOE
#define KEY3_PIN     GPIO_Pin_4
#define KEY3_RCC     RCC_AHB1Periph_GPIOE

#if BOARD_KEY_ACTIVE_LOW
#define KEY_RAW_DOWN(x)  ((x) == Bit_RESET)
#else
#define KEY_RAW_DOWN(x)  ((x) == Bit_SET)
#endif

#define KEY0_DOWN()  KEY_RAW_DOWN(GPIO_ReadInputDataBit(KEY0_GPIO, KEY0_PIN))
#define KEY1_DOWN()  KEY_RAW_DOWN(GPIO_ReadInputDataBit(KEY1_GPIO, KEY1_PIN))
#define KEY2_DOWN()  KEY_RAW_DOWN(GPIO_ReadInputDataBit(KEY2_GPIO, KEY2_PIN))
#define KEY3_DOWN()  KEY_RAW_DOWN(GPIO_ReadInputDataBit(KEY3_GPIO, KEY3_PIN))

void BoardKeys_Init(void)
{
	GPIO_InitTypeDef g;

	RCC_AHB1PeriphClockCmd(KEY0_RCC | KEY1_RCC | KEY2_RCC | KEY3_RCC, ENABLE);

	g.GPIO_Pin = KEY0_PIN;
	g.GPIO_Mode = GPIO_Mode_IN;
	g.GPIO_PuPd = GPIO_PuPd_UP;
	g.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_Init(KEY0_GPIO, &g);

	g.GPIO_Pin = KEY1_PIN;
	GPIO_Init(KEY1_GPIO, &g);

	g.GPIO_Pin = KEY2_PIN;
	GPIO_Init(KEY2_GPIO, &g);

	g.GPIO_Pin = KEY3_PIN;
	GPIO_Init(KEY3_GPIO, &g);
}

uint8_t BoardKeys_Poll1ms(void)
{
	static uint8_t sr0 = 0xFFu, sr1 = 0xFFu, sr2 = 0xFFu, sr3 = 0xFFu;
	static uint8_t l0 = 0u, l1 = 0u, l2 = 0u, l3 = 0u;
#if BOARD_KEYS_HOME_COMBO_K1K2
	static uint8_t s_combo_armed = 1u; /* 松手后允许再次触发回中 */
#endif

	sr0 = (uint8_t)((sr0 << 1) | (KEY0_DOWN() ? 0u : 1u));
	sr1 = (uint8_t)((sr1 << 1) | (KEY1_DOWN() ? 0u : 1u));
	sr2 = (uint8_t)((sr2 << 1) | (KEY2_DOWN() ? 0u : 1u));
	sr3 = (uint8_t)((sr3 << 1) | (KEY3_DOWN() ? 0u : 1u));

	{
		/* 连续 8 次采样为按下才认为稳定按下，抗抖更好 */
		uint8_t d0 = (sr0 & 0xFFu) == 0u;
		uint8_t d1 = (sr1 & 0xFFu) == 0u;
		uint8_t d2 = (sr2 & 0xFFu) == 0u;
		uint8_t d3 = (sr3 & 0xFFu) == 0u;
		uint8_t ev = 0u;

#if BOARD_KEYS_HOME_COMBO_K1K2
		/* K1+K2 同时按下：云台回约定初始角（与串口 RST: 一致），优先于单键 */
		if (d0 && d1)
		{
			if (s_combo_armed)
			{
				s_combo_armed = 0u;
				l0 = d0;
				l1 = d1;
				l2 = d2;
				l3 = d3;
				return 5u;
			}
			l0 = d0;
			l1 = d1;
			l2 = d2;
			l3 = d3;
			return 0u;
		}
		s_combo_armed = 1u;
#endif

		if (d0 && !l0)
			ev = 1u;
		else if (d1 && !l1)
			ev = 2u;
		else if (d2 && !l2)
			ev = 3u;
		else if (d3 && !l3)
			ev = 4u;

		l0 = d0;
		l1 = d1;
		l2 = d2;
		l3 = d3;
		return ev;
	}
}
