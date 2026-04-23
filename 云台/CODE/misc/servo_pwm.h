#ifndef SERVO_PWM_H
#define SERVO_PWM_H

#include "stm32f4xx.h"

/*
 * 舵机经 PCA9685；I2C 引脚见 CODE/comm/i2c1_board.h（GEC-M4 用 PB8/PB9）。
 * 横向/纵向接驱动板第 1/2 路 -> CH0/CH1；舵机供电接模块 V+，勿只靠 MCU VCC。
 * 若舵机完全不动：模块若有 ~OE 引脚，一般接 GND（使能输出），勿悬空。
 */
#ifndef SERVO_PCA9685_ADDR7
#define SERVO_PCA9685_ADDR7   0x40u
#endif
/* MODE2：0x04 推挽常见；个别模块需 0x00 开漏，可在此处改 */
#ifndef PCA9685_MODE2_VAL
#define PCA9685_MODE2_VAL     0x04u
#endif
#ifndef SERVO_CH_HORIZONTAL
#define SERVO_CH_HORIZONTAL   0u   /* 横向：驱动板 1 路 -> PCA9685 CH0 */
#endif
#ifndef SERVO_CH_VERTICAL
#define SERVO_CH_VERTICAL     1u   /* 纵向：驱动板 2 路 -> PCA9685 CH1 */
#endif
/*
 * DM：0°=500µs、90°=1500µs、180°=2500µs。
 * 初值：横向已按一次 K2+ 再多次 K1- 校到约 0°；纵向若与实机不符请只改 SERVO_V_INIT_DEG。
 * 步长按 SERVO_PULSE_STEP_US（约 20µs）换算为整数度。
 */
#ifndef SERVO_PULSE_STEP_US
#define SERVO_PULSE_STEP_US  20u
#endif
#ifndef SERVO_ANGLE_STEP_DEG
#define SERVO_ANGLE_STEP_DEG  (((uint32_t)(SERVO_PULSE_STEP_US)*180u + 1000u) / 2000u)
#endif
/* 串口 V+/V- 纵向单帧步进（°）；跟踪过冲大可改为 8～12；扫描需覆盖角度可略大 */
#ifndef SERVO_ANGLE_STEP_VERT_DEG
#define SERVO_ANGLE_STEP_VERT_DEG  10u
#endif
/* 实测校准入位（约 1×H+ 后约 46×H- → 0°） */
#ifndef SERVO_H_INIT_DEG
#define SERVO_H_INIT_DEG  0u
#endif
#ifndef SERVO_V_INIT_DEG
#define SERVO_V_INIT_DEG  90u
#endif
/*
 * 上位机发送 "RST:" 时置位角度（与上电初值默认一致，可在包含本头文件前重定义）。
 * 人脸扫描前 PC 端会先 RST，再按步进做水平扫；纵向 90° 与 SERVO_V_INIT_DEG 一致时无需再扭俯仰。
 */
#ifndef PTZ_RST_H_DEG
#define PTZ_RST_H_DEG  SERVO_H_INIT_DEG
#endif
#ifndef PTZ_RST_V_DEG
#define PTZ_RST_V_DEG  SERVO_V_INIT_DEG
#endif
/* 仅 Servo_AllChannelsSetPulseUs 等接口使用 */
#ifndef SERVO_ALLCH_DEFAULT_US
#define SERVO_ALLCH_DEFAULT_US  1500u
#endif

/* 0 成功；非 0 表示 I2C 无应答或 PCA9685 初始化失败 */
int Servo_Init(void);
/* 将 0~15 路全部设为同一脉宽（打开/刷新全部 PWM 输出） */
void Servo_AllChannelsSetPulseUs(uint16_t us);
void Servo_AllChannelsSetAngleDeg(uint16_t deg); /* 0~180 */

void Servo_SetPulseUsCh(uint8_t ch, uint16_t us);
void Servo_SetAngleDegCh(uint8_t ch, uint16_t deg); /* 0~180 */

/* 兼容旧接口：等效于横向通道 */
void Servo_SetPulseUs(uint16_t us);
void Servo_SetAngleDeg(uint16_t deg);

#endif
