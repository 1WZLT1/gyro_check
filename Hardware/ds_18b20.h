#ifndef __DS_18B20_H
#define __DS_18B20_H

#include <stdio.h>
#include <stdint.h>
#include "main.h"
#include "hal_delay.h"

//#define PAout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //输出 
//#define PAin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //输入


////IO方向设置
//#define DS18B20_IO_IN()  {GPIOG->MODER&=~(3<<(9*2));GPIOG->MODER|=0<<(9*2);}	//PG9输入模式
//#define DS18B20_IO_OUT() {GPIOG->MODER&=~(3<<(9*2));GPIOG->MODER|=1<<(9*2);} 	//PG9输出模式
// 
//////IO操作函数											   
//#define	DS18B20_DQ_OUT PGout(9) 					//数据端口	PG9
//#define	DS18B20_DQ_IN  PGin(9)  					//数据端口	PG9 
//   	
//uint8_t DS18B20_Init(void);								//初始化DS18B20
//short DS18B20_Get_Temp(void);							//获取温度
//void DS18B20_Start(void);									//开始温度转换
//void DS18B20_Write_Byte(uint8_t dat);			//写入一个字节
//uint8_t DS18B20_Read_Byte(void);					//读出一个字节
//uint8_t DS18B20_Read_Bit(void);						//读出一个位
//uint8_t DS18B20_Check(void);							//检测是否存在DS18B20
//void DS18B20_Rst(void);										//复位DS18B20 

#define 	DQ_GPIO_Port GPIOA
#define 	DQ_Pin			 GPIO_PIN_11

#define 	DS18B20_DQ_OUT_LOW 				HAL_GPIO_WritePin(DQ_GPIO_Port , DQ_Pin, GPIO_PIN_RESET)
#define 	DS18B20_DQ_OUT_HIGH 			HAL_GPIO_WritePin(DQ_GPIO_Port , DQ_Pin, GPIO_PIN_SET)
#define  	DS18B20_DQ_IN             HAL_GPIO_ReadPin(DQ_GPIO_Port, DQ_Pin)

extern void DS18B20_Rst(void);
extern uint8_t DS18B20_Init(void);
extern float DS18B20_Get_Temperature_float(void);

#endif

