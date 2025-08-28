#ifndef __HAL_DELAY_H
#define __HAL_DELAY_H

#include <stdio.h>
#include <stdint.h>
#include "main.h"
#include "tim.h"



#define DELAY_SYS
//#define DELAY_TIM7


#if defined(DELAY_SYS)
    // 代码适用于平台A
		extern void delay_us(__IO uint32_t delay);
#elif defined(DELAY_TIM7)
    // 代码适用于平台B
		#define DLY_TIM_Handle  (&htim7)
		extern void delay_us(uint16_t nus);
		extern TIM_HandleTypeDef htim7;
#endif


#endif

