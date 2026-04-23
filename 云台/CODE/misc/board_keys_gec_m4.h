#ifndef BOARD_KEYS_GEC_M4_H
#define BOARD_KEYS_GEC_M4_H

/*
 * GEC-M4 常见独立按键接法（以原理图为准，可在 board_keys.c 改 GPIO）：
 *  K1 -> PA0   K2 -> PE2   K3 -> PE3   K4 -> PE4
 * 一般为低电平有效（按下接地）。
 */
#ifndef BOARD_KEY_ACTIVE_LOW
#define BOARD_KEY_ACTIVE_LOW  1
#endif
/*
 * 为 1 时：在 main 里交换事件 1,2 与 3,4（KEY0/1 与 KEY2/3 功能对调），
 * 用于丝印「横/纵」与 PCB 实际 KEY0~3 接线不一致时。
 */
#ifndef BOARD_KEYS_SWAP_HV
#define BOARD_KEYS_SWAP_HV  0
#endif
/*
 * 为 1 时：同时按住 K1(KEY0) 与 K2(KEY1)（未做 SWAP 前的物理键）产生一次「回初始位」事件（Poll 返回 5）。
 * 与上位机串口帧 RST: 使用同一套 PTZ_RST_H_DEG / PTZ_RST_V_DEG（servo_pwm.h）。
 * 若硬件未接 KEY0/KEY1 且引脚浮空导致误触发，可在工程 Define 中置 BOARD_KEYS_HOME_COMBO_K1K2=0。
 */
#ifndef BOARD_KEYS_HOME_COMBO_K1K2
#define BOARD_KEYS_HOME_COMBO_K1K2  1
#endif

#endif
