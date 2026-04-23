#include "stm32f4xx.h"
#include "Delay.h"
#include "usart.h"
#include "servo_pwm.h"
#include "board_keys.h"
#include "board_keys_gec_m4.h"
#include "i2c1_board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ptz_control.h"

#if (configUSE_TICK_HOOK == 1)
void vApplicationTickHook(void)
{
	/* 保留空钩子 */
}
#endif

static TaskHandle_t s_task_ptz;

static void task_ptz(void *pv)
{
	(void)pv;
	for (;;)
	{
		Ptz_PollKeysAndUart();
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

int main(void)
{
	SystemCoreClockUpdate();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

	Delay_Init();
	BoardKeys_Init();

	Usart1_Init(USART1_PC_BAUDRATE);
	Usart3_Init(USART3_BT_BAUDRATE);
	Usart1_SendString("\r\n");

#if I2C1_BOARD_PB8_PB9
	Usart1_SendString("GIMBAL+FREERTOS: I2C1 PB8/PB9\r\n");
#else
	Usart1_SendString("GIMBAL+FREERTOS: I2C1 PB6/PB7\r\n");
#endif
	Usart1_SendString("HOME: K1+K2 or UART \"RST:\"\r\n");

	Ptz_Init();
	if (Servo_Init() != 0)
		Usart1_SendString("WARN: PCA9685 not ready; servo may not move\r\n");

	for (u32 i = 0; i < 3u; i++)
	{
		Usart1_SendString("STM32READY\r\n");
		Usart3_SendString("STM32READY\r\n");
		Delay_ms(200);
	}

	if (xTaskCreate(task_ptz, "ptz", (uint16_t)3072, NULL, (UBaseType_t)3, &s_task_ptz) != pdPASS)
	{
		for (;;)
		{
		}
	}

	vTaskStartScheduler();

	for (;;)
	{
	}
}
