#include "hal_delay.h"

#if defined(DELAY_SYS)
#define CPU_FREQUENCY_MHZ    168		// STM32Ê±ÖÓÖ÷Æµ
void delay_us(__IO uint32_t delay)
{
    int last, curr, val;
    int temp;

    while (delay != 0)
    {
        temp = delay > 900 ? 900 : delay;
        last = SysTick->VAL;
        curr = last - CPU_FREQUENCY_MHZ * temp;
        if (curr >= 0)
        {
            do
            {
                val = SysTick->VAL;
            }
            while ((val < last) && (val >= curr));
        }
        else
        {
            curr += CPU_FREQUENCY_MHZ * 1000;
            do
            {
                val = SysTick->VAL;
            }
            while ((val <= last) || (val > curr));
        }
        delay -= temp;
    }
}
#elif defined(DELAY_TIM7)

	void delay_us(uint16_t nus)
	{
		__HAL_TIM_SET_COUNTER(DLY_TIM_Handle, 0);
		__HAL_TIM_ENABLE(DLY_TIM_Handle);
		while (__HAL_TIM_GET_COUNTER(DLY_TIM_Handle) < nus)
		{
		}
		__HAL_TIM_DISABLE(DLY_TIM_Handle);
	}

#endif


