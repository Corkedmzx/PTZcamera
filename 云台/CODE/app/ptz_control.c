#include "ptz_control.h"
#include "servo_pwm.h"
#include "board_keys.h"
#include "board_keys_gec_m4.h"
#include "usart.h"
#include "string.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

static QueueHandle_t s_ptz_uiq;

#define PTZ_HOLD_IDLE 0xFFu
#define PTZ_STEP_DEG 2

/* 方向长按：由 task_ptz 定时节拍直驱 PWM，不经过深度队列 */
static uint8_t s_ui_hold = PTZ_HOLD_IDLE;
static TickType_t s_ui_next_pwm;

static void ptz_ui_hold_service(void)
{
	TickType_t now;
	uint8_t h;

	h = s_ui_hold;
	if (h == PTZ_HOLD_IDLE)
		return;
	now = xTaskGetTickCount();
	if (now < s_ui_next_pwm)
		return;

	switch (h)
	{
	case PTZ_UIQ_H_MINUS:
		Ptz_StepHorizontal(-PTZ_STEP_DEG);
		break;
	case PTZ_UIQ_H_PLUS:
		Ptz_StepHorizontal(PTZ_STEP_DEG);
		break;
	case PTZ_UIQ_V_MINUS:
		Ptz_StepVertical(-PTZ_STEP_DEG);
		break;
	case PTZ_UIQ_V_PLUS:
		Ptz_StepVertical(PTZ_STEP_DEG);
		break;
	default:
		break;
	}

	s_ui_next_pwm = now + (TickType_t)pdMS_TO_TICKS((TickType_t)PTZ_UI_HOLD_INTERVAL_MS);
}

static void take_and_clear(volatile u8 *buf, volatile u8 *flag_ptr)
{
	if (*flag_ptr != 1)
		return;
	__disable_irq();
	memset((void *)buf, 0, USART_RX_SIZE);
	*flag_ptr = 0;
	__enable_irq();
}

static void apply_deg_step(uint16_t *deg, int32_t delta)
{
	int32_t a = (int32_t)*deg + delta;

	if (a < 0)
		a = 0;
	if (a > 180)
		a = 180;
	*deg = (uint16_t)a;
}

static void process_usart1_ptz(const u8 *buf, uint16_t *ang_h, uint16_t *ang_v);

static void usart1_snapshot_and_process(uint16_t *ang_h, uint16_t *ang_v)
{
	static u8 s_frame[USART_RX_SIZE];

	if (g_flag_usart1 != 1u)
		return;
	__disable_irq();
	memcpy(s_frame, (const void *)g_rx_buffer_usart1, USART_RX_SIZE);
	g_flag_usart1 = 0u;
	memset((void *)g_rx_buffer_usart1, 0, USART_RX_SIZE);
	__enable_irq();
	process_usart1_ptz(s_frame, ang_h, ang_v);
}

static void process_usart1_ptz(const u8 *buf, uint16_t *ang_h, uint16_t *ang_v)
{
	if (buf == 0 || buf[0] == 0)
		return;

	if (buf[0] == 'H' && buf[1] == '-' && buf[2] == ':')
	{
		apply_deg_step(ang_h, -PTZ_STEP_DEG);
		Servo_SetAngleDegCh(SERVO_CH_HORIZONTAL, *ang_h);
		Usart1_SendString("UART H-\r\n");
	}
	else if (buf[0] == 'H' && buf[1] == '+' && buf[2] == ':')
	{
		apply_deg_step(ang_h, PTZ_STEP_DEG);
		Servo_SetAngleDegCh(SERVO_CH_HORIZONTAL, *ang_h);
		Usart1_SendString("UART H+\r\n");
	}
	else if (buf[0] == 'V' && buf[1] == '-' && buf[2] == ':')
	{
		apply_deg_step(ang_v, -PTZ_STEP_DEG);
		Servo_SetAngleDegCh(SERVO_CH_VERTICAL, *ang_v);
		Usart1_SendString("UART V-\r\n");
	}
	else if (buf[0] == 'V' && buf[1] == '+' && buf[2] == ':')
	{
		apply_deg_step(ang_v, PTZ_STEP_DEG);
		Servo_SetAngleDegCh(SERVO_CH_VERTICAL, *ang_v);
		Usart1_SendString("UART V+\r\n");
	}
	else if (buf[0] == 'R' && buf[1] == 'S' && buf[2] == 'T' && buf[3] == ':')
	{
		*ang_h = PTZ_RST_H_DEG;
		*ang_v = PTZ_RST_V_DEG;
		if (*ang_h > 180u)
			*ang_h = 180u;
		if (*ang_v > 180u)
			*ang_v = 180u;
		Servo_SetAngleDegCh(SERVO_CH_HORIZONTAL, *ang_h);
		Servo_SetAngleDegCh(SERVO_CH_VERTICAL, *ang_v);
		Usart1_SendString("UART RST\r\n");
	}
}

static uint16_t s_ang_h;
static uint16_t s_ang_v;

void Ptz_ApplyHome(void)
{
	s_ang_h = PTZ_RST_H_DEG;
	s_ang_v = PTZ_RST_V_DEG;
	if (s_ang_h > 180u)
		s_ang_h = 180u;
	if (s_ang_v > 180u)
		s_ang_v = 180u;
	Servo_SetAngleDegCh(SERVO_CH_HORIZONTAL, s_ang_h);
	Servo_SetAngleDegCh(SERVO_CH_VERTICAL, s_ang_v);
}

void Ptz_StepHorizontal(int32_t delta_deg)
{
	apply_deg_step(&s_ang_h, delta_deg);
	Servo_SetAngleDegCh(SERVO_CH_HORIZONTAL, s_ang_h);
}

void Ptz_StepVertical(int32_t delta_deg)
{
	apply_deg_step(&s_ang_v, delta_deg);
	Servo_SetAngleDegCh(SERVO_CH_VERTICAL, s_ang_v);
}

void Ptz_Init(void)
{
	s_ang_h = SERVO_H_INIT_DEG;
	s_ang_v = SERVO_V_INIT_DEG;
	if (s_ptz_uiq == NULL)
		s_ptz_uiq = xQueueCreate(8u, sizeof(uint8_t));
}

void Ptz_UiSetHold(uint8_t op)
{
	if (op == PTZ_UIQ_HOME)
		return;
	if (op > PTZ_UIQ_V_PLUS)
		return;
	s_ui_hold = op;
	s_ui_next_pwm = xTaskGetTickCount();
}

void Ptz_UiClearHold(void)
{
	s_ui_hold = PTZ_HOLD_IDLE;
}

void Ptz_UiPost(uint8_t op)
{
	if (s_ptz_uiq == NULL)
		return;
	(void)xQueueSend(s_ptz_uiq, &op, 0);
}

/*
 * UI 队列：四向统一使用 PTZ_STEP_DEG，保证横向与纵向速度一致。
 * 每次轮询最多处理 1 条，避免一次 drain 清空队列造成连跳。
 */
static void ptz_uiq_drain(void)
{
	uint8_t op;

	if (s_ptz_uiq == NULL || xQueueReceive(s_ptz_uiq, &op, 0) != pdPASS)
		return;

	switch (op)
	{
	case PTZ_UIQ_HOME:
		Ptz_ApplyHome();
		break;
	case PTZ_UIQ_H_MINUS:
		Ptz_StepHorizontal(-PTZ_STEP_DEG);
		break;
	case PTZ_UIQ_H_PLUS:
		Ptz_StepHorizontal(PTZ_STEP_DEG);
		break;
	case PTZ_UIQ_V_MINUS:
		Ptz_StepVertical(-PTZ_STEP_DEG);
		break;
	case PTZ_UIQ_V_PLUS:
		Ptz_StepVertical(PTZ_STEP_DEG);
		break;
	default:
		break;
	}
}

void Ptz_PollKeysAndUart(void)
{
	ptz_uiq_drain();
	ptz_ui_hold_service();
	uint8_t ev = BoardKeys_Poll1ms();

#if BOARD_KEYS_SWAP_HV
	if (ev == 1u)
		ev = 3u;
	else if (ev == 2u)
		ev = 4u;
	else if (ev == 3u)
		ev = 1u;
	else if (ev == 4u)
		ev = 2u;
#endif
	switch (ev)
	{
	case 5u:
		Ptz_UiClearHold();
		Ptz_UiPost(PTZ_UIQ_HOME);
		break;
	case 1u:
		Ptz_StepHorizontal(-PTZ_STEP_DEG);
		Usart1_SendString("KEY K1 H-\r\n");
		break;
	case 2u:
		Ptz_StepHorizontal(PTZ_STEP_DEG);
		Usart1_SendString("KEY K2 H+\r\n");
		break;
	case 3u:
		Ptz_StepVertical(-PTZ_STEP_DEG);
		Usart1_SendString("KEY K3 V-\r\n");
		break;
	case 4u:
		Ptz_StepVertical(PTZ_STEP_DEG);
		Usart1_SendString("KEY K4 V+\r\n");
		break;
	default:
		break;
	}

	if (g_flag_usart1)
		usart1_snapshot_and_process(&s_ang_h, &s_ang_v);
	if (g_flag_usart3)
		take_and_clear(g_rx_buffer_usart3, &g_flag_usart3);
}
