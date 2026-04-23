#include "usart.h"
#include "stm32f4xx_gpio.h"
#include <stdint.h>

volatile u8 g_data = 0;

#if USART_MIRROR_USART2
static uint8_t s_usart2_mirror_ok;
#endif

static void usart_send_byte(USART_TypeDef *USARTx, uint8_t b)
{
	uint32_t w = USART_TX_WAIT_MAX;
	while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
	{
		if (--w == 0u)
			return;
	}
	USART_SendData(USARTx, b);
}

#if USART_MIRROR_USART2
static void Usart2_InitMirror(int BaudRate)
{
	GPIO_InitTypeDef g;
	USART_InitTypeDef u;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	g.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	g.GPIO_Mode = GPIO_Mode_AF;
	g.GPIO_Speed = GPIO_Speed_50MHz;
	g.GPIO_OType = GPIO_OType_PP;
	g.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &g);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

	u.USART_BaudRate = BaudRate;
	u.USART_WordLength = USART_WordLength_8b;
	u.USART_StopBits = USART_StopBits_1;
	u.USART_Parity = USART_Parity_No;
	u.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	u.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(USART2, &u);
	USART_Cmd(USART2, ENABLE);
	s_usart2_mirror_ok = 1u;
}
#endif

static  u8 g_buffer[USART_RX_SIZE] = {0};
volatile u8 g_rx_buffer_usart3[USART_RX_SIZE] = {0};
/* 须与 USART_RX_SIZE（512）同宽：曾用 u8 时 >511 判断恒假，计数在 256 回绕导致写穿缓冲 */
static uint16_t g_cout = 0;
volatile u8 g_flag_usart3 = 0;

static uint16_t g_cout1 = 0;
static u8 g_buffer1[USART_RX_SIZE] = {0};
volatile u8 g_rx_buffer_usart1[USART_RX_SIZE] = {0};
volatile u8 g_flag_usart1 = 0;

/*********************************

PB10  ---- USART3_TX(发送端)
PB11  ---- USART3_RX(接收端)

*********************************/
void Usart3_Init(int BaudRate)
{
	GPIO_InitTypeDef	GPIO_InitStruct;
	USART_InitTypeDef	USART_InitStruct;
	NVIC_InitTypeDef   NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP ;
	GPIO_Init(GPIOB, &GPIO_InitStruct); 
	
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, (uint8_t)7);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, (uint8_t)7);
	
	USART_InitStruct.USART_BaudRate		= BaudRate;
	USART_InitStruct.USART_Mode			= USART_Mode_Tx|USART_Mode_Rx;
	USART_InitStruct.USART_Parity		= USART_Parity_No;
	USART_InitStruct.USART_StopBits		= USART_StopBits_1;
	USART_InitStruct.USART_WordLength   = USART_WordLength_8b;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(USART3, &USART_InitStruct);
	
	NVIC_InitStructure.NVIC_IRQChannel 					 = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority 		 = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd 			     = ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);    
	USART_Cmd(USART3, ENABLE);	

}

void Usart3_SendString(const char* str)
{
	if (str == 0)
		return;
	while (*str)
	{
		while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET)
			;
		USART_SendData(USART3, (u8)(*str));
		str++;
	}
}

/*********************************

PA9   ---- USART1_TX(发送端)
PA10  ---- USART1_RX(接收端)

*********************************/
void Usart1_Init(int BaudRate)
{
	GPIO_InitTypeDef	GPIO_InitStruct;
	USART_InitTypeDef	USART_InitStruct;
	NVIC_InitTypeDef   NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, (uint8_t)7);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, (uint8_t)7);

	USART_InitStruct.USART_BaudRate            = BaudRate;
	USART_InitStruct.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
	USART_InitStruct.USART_Parity             = USART_Parity_No;
	USART_InitStruct.USART_StopBits           = USART_StopBits_1;
	USART_InitStruct.USART_WordLength        = USART_WordLength_8b;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

	USART_Init(USART1, &USART_InitStruct);

	NVIC_InitStructure.NVIC_IRQChannel                   = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);

#if USART_MIRROR_USART2
	Usart2_InitMirror(BaudRate);
#endif
}

void Usart1_SendString(const char* str)
{
	if (str == 0)
		return;
	while (*str)
	{
		usart_send_byte(USART1, (uint8_t)(*str));
#if USART_MIRROR_USART2
		if (s_usart2_mirror_ok)
			usart_send_byte(USART2, (uint8_t)(*str));
#endif
		str++;
	}
}

void USART3_IRQHandler(void)
{
    u8 data=0;
    if (USART_GetFlagStatus(USART3, USART_FLAG_ORE) != RESET)
    {
        (void)USART_ReceiveData(USART3);
    }
    if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)
    {
         data = USART_ReceiveData(USART3);
         if (g_cout == 0 && (data == '\r' || data == '\n'))
         {
             USART_ClearITPendingBit(USART3, USART_IT_RXNE);
             return;
         }
         if(g_cout > USART_RX_SIZE-1)
         {
            g_cout = 0;
            memset(g_buffer, 0, sizeof(g_buffer));             
         }
        else
        {
            g_buffer[g_cout++] = data;
            
            if(data == ':' )
            {
                int i;
                for(i=0; i< g_cout; i++)
                {
                    g_rx_buffer_usart3[i] = g_buffer[i];
                }
                if (g_cout < USART_RX_SIZE)
                    g_rx_buffer_usart3[g_cout] = '\0';
                g_flag_usart3 = 1;
                g_cout = 0;
                memset(g_buffer, 0, sizeof(g_buffer));                
            }
        }
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }

}

void USART1_IRQHandler(void)
{
	u8 data = 0;
	if (USART_GetFlagStatus(USART1, USART_FLAG_ORE) != RESET)
	{
		(void)USART_ReceiveData(USART1);
	}
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		data = USART_ReceiveData(USART1);
		if (g_cout1 == 0 && (data == '\r' || data == '\n'))
		{
			USART_ClearITPendingBit(USART1, USART_IT_RXNE);
			return;
		}

		if (g_cout1 > USART_RX_SIZE - 1)
		{
			g_cout1 = 0;
			memset(g_buffer1, 0, sizeof(g_buffer1));
		}
		else
		{
			g_buffer1[g_cout1++] = data;
			if (data == ':')
			{
				int i;
				for (i = 0; i < (int)g_cout1; i++)
				{
					g_rx_buffer_usart1[i] = g_buffer1[i];
				}
				if (g_cout1 < USART_RX_SIZE)
					g_rx_buffer_usart1[g_cout1] = '\0';
				g_flag_usart1 = 1;
				g_cout1 = 0;
				memset(g_buffer1, 0, sizeof(g_buffer1));
			}
		}

		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}
