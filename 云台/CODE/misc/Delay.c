#include "Delay.h"

/* 使用 SystemCoreClock（system_stm32f4xx 中定义），须与 SystemInit() 后实际频率一致。
 * 若误用较小常数会导致 Delay_ms 偏短，DHT11 启动低电平不足 18ms → 无应答 ERR=1 */

// 使用 SysTick 产生 1us 基准
void Delay_Init(void)
{
    uint32_t ticks_per_us = SystemCoreClock / 1000000UL; /* 每 1us 所需时钟周期数 */
    if (ticks_per_us < 1U)
        ticks_per_us = 1U;

    // SysTick 计数周期：1us
    SysTick->LOAD = ticks_per_us - 1U;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

// 微秒级延时（忙等待）
void Delay_us(uint32_t us)
{
    while (us--)
    {
        // 等待 COUNTFLAG 置位（每计满一次触发）
        while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk))
        {
        }
    }
}
 
// 毫秒级延时（忙等待）
void Delay_ms(uint32_t ms)
{
	while (ms--)
	{
		Delay_us(1000); // 调用微秒延时实现毫秒延时
	}
}

