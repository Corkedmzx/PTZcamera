#ifndef __USART_H
#define __USART_H

#include "stm32f4xx.h"
#include "sys.h"

/* 需容纳 set oled,...GB2312 转十六进制 等较长帧 */
#define 	USART_RX_SIZE 	512
#define 	USART3_RX_SIZE 	USART_RX_SIZE

/* 固定波特率：USART1(PC 调试口) 与 USART3(蓝牙) 互不影响，不在运行时切换 */
#ifndef USART1_PC_BAUDRATE
#define USART1_PC_BAUDRATE  115200
#endif
#ifndef USART3_BT_BAUDRATE
#define USART3_BT_BAUDRATE  9600
#endif

/* 为 1 时 Usart1_SendString 同时发往 USART2(PA2/PA3)，与 USART1(PA9/PA10) 同波特率；板载 USB 串口常接 USART2 */
#ifndef USART_MIRROR_USART2
#define USART_MIRROR_USART2  1
#endif
/* TXE 等待上限，避免 USART 时钟异常时死等导致“无任何打印” */
#ifndef USART_TX_WAIT_MAX
#define USART_TX_WAIT_MAX  500000u
#endif

extern volatile  u8 g_data;

/* USART1 / USART3 各自缓冲与标志，避免两路同时收帧时互相覆盖 */
extern volatile u8 g_rx_buffer_usart1[USART_RX_SIZE];
extern volatile u8 g_rx_buffer_usart3[USART_RX_SIZE];
extern volatile u8 g_flag_usart1;
extern volatile u8 g_flag_usart3;

void Usart3_Init(int BaudRate);

void Usart3_SendString(const char* str);

void Usart1_Init(int BaudRate);
void Usart1_SendString(const char* str);


#endif
