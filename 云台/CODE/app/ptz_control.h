#ifndef PTZ_CONTROL_H
#define PTZ_CONTROL_H

#include "stm32f4xx.h"

/*
 * 界面队列命令（task_ptz 内执行 I2C）。
 * 与 PTZcamera 默认键盘→串口一致（见 ptz_serial_io_worker，未设 PTZ_SERIAL_SWAP_HV_TX）：
 *   上/下 → H-/H+；左/右 → V+/V-；复位 → 同 RST:
 */
#define PTZ_UIQ_H_MINUS 0u
#define PTZ_UIQ_H_PLUS  1u
#define PTZ_UIQ_V_MINUS 2u
#define PTZ_UIQ_V_PLUS  3u
#define PTZ_UIQ_HOME    4u

/* 长按连发节拍（仅 task_ptz 内执行 PWM，避免往队列狂塞指令占堆） */
#ifndef PTZ_UI_HOLD_INTERVAL_MS
#define PTZ_UI_HOLD_INTERVAL_MS 80u
#endif

void Ptz_Init(void);
void Ptz_PollKeysAndUart(void);
/* 方向键按住：仅设置保持方向，由 task_ptz 按节拍直调 Ptz_Step* → I2C/PWM */
void Ptz_UiSetHold(uint8_t op);
void Ptz_UiClearHold(void);
/* 仅用于单次动作（如 HOME），入短队列由 task_ptz 执行 */
void Ptz_UiPost(uint8_t op);

void Ptz_ApplyHome(void);
void Ptz_StepHorizontal(int32_t delta_deg);
void Ptz_StepVertical(int32_t delta_deg);

#endif
