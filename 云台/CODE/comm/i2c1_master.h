#ifndef I2C1_MASTER_H
#define I2C1_MASTER_H

#include "stm32f4xx.h"

/*
 * I2C1：PB6=SCL，PB7=SDA（AF4），与常见 PCA9685 舵机板连接。
 * 若核心板使用其它 I2C 引脚，请改 i2c1_master.c。
 */
void I2C1_Board_Init(void);

/* 释放总线、清 AF（连续探测前建议调用） */
void I2C1_ClearBus(void);

/* 探测 7 位地址是否有应答（内部使用 addr<<1 与 StdPeriph 一致） */
int I2C1_Probe7(uint8_t slave_addr7);

/* 扫描 0x08~0x77，对每个应答调用 cb(addr7)（可为 NULL 仅清总线测试） */
void I2C1_ScanBus(void (*cb)(uint8_t addr7));

/* 7 位从机地址（如 PCA9685 默认 0x40） */
int I2C1_WriteReg8(uint8_t slave_addr7, uint8_t reg, uint8_t val);
int I2C1_ReadReg8(uint8_t slave_addr7, uint8_t reg, uint8_t *val);
/* 连续写（适合 PCA9685 一次写 4 字节脉宽寄存器） */
int I2C1_WriteMem(uint8_t slave_addr7, uint8_t reg, const uint8_t *data, uint8_t len);

#endif
