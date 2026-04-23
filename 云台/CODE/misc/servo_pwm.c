#include "servo_pwm.h"
#include "i2c1_master.h"
#include "Delay.h"
#include "usart.h"
#include "stm32f4xx.h"

#define PCA9685_REG_MODE1       0x00u
#define PCA9685_REG_MODE2       0x01u
#define PCA9685_REG_LED0_ON_L   0x06u
#define PCA9685_REG_PRESCALE    0xFEu

#define PCA9685_MODE1_SLEEP     0x10u
#define PCA9685_MODE1_AI        0x20u
#define PCA9685_MODE1_RESTART   0x80u

static int pca9685_init_device(void)
{
	if (I2C1_WriteReg8(SERVO_PCA9685_ADDR7, PCA9685_REG_MODE1, PCA9685_MODE1_SLEEP) != 0)
		return -1;
	if (I2C1_WriteReg8(SERVO_PCA9685_ADDR7, PCA9685_REG_PRESCALE, 121u) != 0)
		return -1;
	if (I2C1_WriteReg8(SERVO_PCA9685_ADDR7, PCA9685_REG_MODE1,
			   (uint8_t)(PCA9685_MODE1_AI | PCA9685_MODE1_RESTART)) != 0)
		return -1;
	Delay_ms(10u);
	if (I2C1_WriteReg8(SERVO_PCA9685_ADDR7, PCA9685_REG_MODE2, (uint8_t)PCA9685_MODE2_VAL) != 0)
		return -1;
	return 0;
}

static void usart1_hex8(uint8_t v)
{
	static const char h[] = "0123456789ABCDEF";
	char s[3];

	s[0] = h[(v >> 4) & 0x0Fu];
	s[1] = h[v & 0x0Fu];
	s[2] = '\0';
	Usart1_SendString(s);
}

static void scan_hit(uint8_t addr7)
{
	Usart1_SendString(" 0x");
	usart1_hex8(addr7);
}

/* PCA9685 手册：LEDn_OFF_H 的 bit4=Full-OFF，该路输出恒低，舵机侧无有效 PWM */
static void pca9685_channel_full_off(uint8_t ch)
{
	uint8_t reg = (uint8_t)(PCA9685_REG_LED0_ON_L + 4u * ch);
	uint8_t buf[4];

	if (ch > 15u)
		return;
	buf[0] = 0u;
	buf[1] = 0u;
	buf[2] = 0u;
	buf[3] = 0x10u;
	(void)I2C1_WriteMem(SERVO_PCA9685_ADDR7, reg, buf, 4u);
}

static void pca9685_set_channel_pwm(uint8_t ch, uint16_t on_ticks, uint16_t off_ticks)
{
	uint8_t reg = (uint8_t)(PCA9685_REG_LED0_ON_L + 4u * ch);
	uint8_t buf[4];

	buf[0] = (uint8_t)(on_ticks & 0xFFu);
	buf[1] = (uint8_t)((on_ticks >> 8) & 0x0Fu);
	buf[2] = (uint8_t)(off_ticks & 0xFFu);
	buf[3] = (uint8_t)((off_ticks >> 8) & 0x0Fu);
	if (I2C1_WriteMem(SERVO_PCA9685_ADDR7, reg, buf, 4u) != 0)
	{
		static uint8_t s_once;

		if (s_once == 0u)
		{
			s_once = 1u;
			Usart1_SendString("WARN: PCA9685 PWM write I2C fail\r\n");
		}
	}
}

void Servo_SetPulseUsCh(uint8_t ch, uint16_t us)
{
	uint32_t off_ticks;

	if (ch > 15u)
		return;

	if (us < 500u)
		us = 500u;
	if (us > 2500u)
		us = 2500u;

	off_ticks = ((uint32_t)us * 4096u) / 20000u;
	if (off_ticks > 4095u)
		off_ticks = 4095u;

	pca9685_set_channel_pwm(ch, 0u, (uint16_t)off_ticks);
}

void Servo_SetAngleDegCh(uint8_t ch, uint16_t deg)
{
	uint32_t us;

	if (deg > 180u)
		deg = 180u;
	/* 与 DMTest_STM32 / DMREG.h 一致：500µs~2500µs 对应 0°~180° */
	us = 500u + ((uint32_t)deg * 2000u) / 180u;
	Servo_SetPulseUsCh(ch, (uint16_t)us);
}

void Servo_AllChannelsSetPulseUs(uint16_t us)
{
	uint8_t ch;

	for (ch = 0u; ch < 16u; ch++)
		Servo_SetPulseUsCh(ch, us);
}

void Servo_AllChannelsSetAngleDeg(uint16_t deg)
{
	uint8_t ch;
	uint32_t pulse_us;

	if (deg > 180u)
		deg = 180u;
	pulse_us = 500u + ((uint32_t)deg * 2000u) / 180u;

	for (ch = 0u; ch < 16u; ch++)
		Servo_SetPulseUsCh(ch, (uint16_t)pulse_us);
}

int Servo_Init(void)
{
	uint8_t mode1 = 0u;

	I2C1_Board_Init();

	if (I2C1_Probe7(SERVO_PCA9685_ADDR7) != 0)
	{
		Usart1_SendString("PCA9685 ERR: NACK at 0x");
		usart1_hex8(SERVO_PCA9685_ADDR7);
		Usart1_SendString(" (7bit). Scan 0x08-0x77:\r\n");
		I2C1_ScanBus(scan_hit);
		Usart1_SendString("\r\n");
		Usart1_SendString("None: GND/VCC? SCL/SDA是否接在 CODE/i2c1_board.h 所选脚?\r\n");
		Usart1_SendString("  PB8/PB9 与 PB6/PB7 二选一，改 I2C1_BOARD_PB8_PB9 后重编译\r\n");
		Usart1_SendString("Found addr: set SERVO_PCA9685_ADDR7 in servo_pwm.h\r\n");
		return -1;
	}
	Usart1_SendString("PCA9685: bus ACK\r\n");

	if (pca9685_init_device() != 0)
	{
		Usart1_SendString("PCA9685 ERR: register init failed\r\n");
		return -2;
	}

	if (I2C1_ReadReg8(SERVO_PCA9685_ADDR7, PCA9685_REG_MODE1, &mode1) == 0)
	{
		Usart1_SendString("MODE1=0x");
		usart1_hex8(mode1);
		Usart1_SendString("\r\n");
	}

	Usart1_SendString("PCA9685: 50Hz, servo power on V+/PWM rail\r\n");
	Usart1_SendString("TIP: many boards have ~OE; must tie OE to GND to enable PWM out\r\n");

	{
		uint8_t ch;

		for (ch = 0u; ch < 16u; ch++)
			pca9685_channel_full_off(ch);
	}
	Usart1_SendString("PCA9685: CH0-CH15 Full-OFF, no PWM until key\r\n");

	return 0;
}

void Servo_SetPulseUs(uint16_t us)
{
	Servo_SetPulseUsCh((uint8_t)SERVO_CH_HORIZONTAL, us);
}

void Servo_SetAngleDeg(uint16_t deg)
{
	Servo_SetAngleDegCh((uint8_t)SERVO_CH_HORIZONTAL, deg);
}
