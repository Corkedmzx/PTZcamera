#include "i2c1_master.h"
#include "i2c1_board.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_i2c.h"

#define I2C_TIMEOUT  20000u

static int I2C1_WaitFlag(uint32_t flag, FlagStatus status)
{
	uint32_t t = I2C_TIMEOUT;
	while (I2C_GetFlagStatus(I2C1, flag) != status)
	{
		if (--t == 0u)
			return -1;
	}
	return 0;
}

static int I2C1_WaitEvent(uint32_t event)
{
	uint32_t t = I2C_TIMEOUT;
	while (!I2C_CheckEvent(I2C1, event))
	{
		if (--t == 0u)
			return -1;
	}
	return 0;
}

static int I2C1_WaitNotBusy(void)
{
	uint32_t t = I2C_TIMEOUT * 10u;
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
	{
		if (--t == 0u)
			return -1;
	}
	return 0;
}

void I2C1_ClearBus(void)
{
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
		I2C_GenerateSTOP(I2C1, ENABLE);

	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);

	(void)I2C1->SR1;
	(void)I2C1->SR2;
}

void I2C1_Board_Init(void)
{
	GPIO_InitTypeDef g;
	I2C_InitTypeDef i2c;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

#if I2C1_BOARD_PB8_PB9
	/* PB8=SCL PB9=SDA，与多数 F407 外接 I2C 教程一致 */
	g.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
#else
	/* PB6=SCL PB7=SDA */
	g.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
#endif
	g.GPIO_Mode = GPIO_Mode_AF;
	g.GPIO_Speed = GPIO_Speed_50MHz;
	g.GPIO_OType = GPIO_OType_OD;
	g.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &g);

#if I2C1_BOARD_PB8_PB9
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource8, GPIO_AF_I2C1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_I2C1);
#else
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1);
#endif

	I2C_DeInit(I2C1);
	I2C_SoftwareResetCmd(I2C1, ENABLE);
	I2C_SoftwareResetCmd(I2C1, DISABLE);
	I2C_AnalogFilterCmd(I2C1, ENABLE);

	i2c.I2C_Mode = I2C_Mode_I2C;
	i2c.I2C_DutyCycle = I2C_DutyCycle_2;
	i2c.I2C_OwnAddress1 = 0;
	i2c.I2C_Ack = I2C_Ack_Enable;
	i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	/* 先降到 50kHz，线较长或上拉较弱时更易成功 */
	i2c.I2C_ClockSpeed = 50000;
	I2C_Init(I2C1, &i2c);
	I2C_Cmd(I2C1, ENABLE);
}

/*
 * StdPeriph：写帧首字节 = (7位地址<<1)|R/W，故传入 (slave_addr7<<1)。
 * 与 ST EEPROM 例程里传 0xA0 一类写法一致。
 */
int I2C1_Probe7(uint8_t slave_addr7)
{
	uint8_t addr_w = (uint8_t)(slave_addr7 << 1);

	I2C1_ClearBus();
	(void)I2C1_WaitNotBusy();

	I2C_GenerateSTART(I2C1, ENABLE);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != 0)
		goto err;
	I2C_Send7bitAddress(I2C1, addr_w, I2C_Direction_Transmitter);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != 0)
		goto err;
	I2C_GenerateSTOP(I2C1, ENABLE);
	I2C1_ClearBus();
	return 0;
err:
	I2C_GenerateSTOP(I2C1, ENABLE);
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);
	(void)I2C1->SR1;
	(void)I2C1->SR2;
	I2C1_ClearBus();
	return -1;
}

void I2C1_ScanBus(void (*cb)(uint8_t addr7))
{
	uint16_t a;

	for (a = 0x08u; a <= 0x77u; a++)
	{
		if (I2C1_Probe7((uint8_t)a) == 0)
		{
			if (cb != 0)
				cb((uint8_t)a);
		}
	}
}

int I2C1_WriteReg8(uint8_t slave_addr7, uint8_t reg, uint8_t val)
{
	uint8_t addr_w = (uint8_t)(slave_addr7 << 1);

	I2C1_ClearBus();
	I2C_GenerateSTART(I2C1, ENABLE);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != 0)
		goto err;
	I2C_Send7bitAddress(I2C1, addr_w, I2C_Direction_Transmitter);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != 0)
		goto err;
	I2C_SendData(I2C1, reg);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != 0)
		goto err;
	I2C_SendData(I2C1, val);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != 0)
		goto err;
	I2C_GenerateSTOP(I2C1, ENABLE);
	I2C1_ClearBus();
	return 0;
err:
	I2C_GenerateSTOP(I2C1, ENABLE);
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);
	(void)I2C1->SR1;
	(void)I2C1->SR2;
	I2C1_ClearBus();
	return -1;
}

int I2C1_ReadReg8(uint8_t slave_addr7, uint8_t reg, uint8_t *val)
{
	uint8_t addr_w = (uint8_t)(slave_addr7 << 1);

	I2C1_ClearBus();
	I2C_GenerateSTART(I2C1, ENABLE);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != 0)
		goto err;
	I2C_Send7bitAddress(I2C1, addr_w, I2C_Direction_Transmitter);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != 0)
		goto err;
	I2C_SendData(I2C1, reg);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != 0)
		goto err;

	I2C_GenerateSTART(I2C1, ENABLE);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != 0)
		goto err;
	I2C_Send7bitAddress(I2C1, addr_w, I2C_Direction_Receiver);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) != 0)
		goto err;
	(void)I2C1->SR2;

	I2C_AcknowledgeConfig(I2C1, DISABLE);
	I2C_GenerateSTOP(I2C1, ENABLE);
	if (I2C1_WaitFlag(I2C_FLAG_RXNE, SET) != 0)
		goto err;
	*val = I2C_ReceiveData(I2C1);
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	I2C1_ClearBus();
	return 0;
err:
	I2C_GenerateSTOP(I2C1, ENABLE);
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);
	(void)I2C1->SR1;
	(void)I2C1->SR2;
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	I2C1_ClearBus();
	return -1;
}

int I2C1_WriteMem(uint8_t slave_addr7, uint8_t reg, const uint8_t *data, uint8_t len)
{
	uint8_t addr_w = (uint8_t)(slave_addr7 << 1);
	uint8_t i;

	I2C1_ClearBus();
	I2C_GenerateSTART(I2C1, ENABLE);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_MODE_SELECT) != 0)
		goto err;
	I2C_Send7bitAddress(I2C1, addr_w, I2C_Direction_Transmitter);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != 0)
		goto err;
	I2C_SendData(I2C1, reg);
	if (I2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != 0)
		goto err;
	for (i = 0; i < len; i++)
	{
		I2C_SendData(I2C1, data[i]);
		if (I2C1_WaitEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED) != 0)
			goto err;
	}
	I2C_GenerateSTOP(I2C1, ENABLE);
	I2C1_ClearBus();
	return 0;
err:
	I2C_GenerateSTOP(I2C1, ENABLE);
	if (I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
		I2C_ClearFlag(I2C1, I2C_FLAG_AF);
	(void)I2C1->SR1;
	(void)I2C1->SR2;
	I2C1_ClearBus();
	return -1;
}
