#ifndef __DS_18B20_H
#define __DS_18B20_H

#include <stdio.h>
#include <stdint.h>
#include "main.h"
#include "hal_delay.h"

//#define PAout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //��� 
//#define PAin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //����


////IO��������
//#define DS18B20_IO_IN()  {GPIOG->MODER&=~(3<<(9*2));GPIOG->MODER|=0<<(9*2);}	//PG9����ģʽ
//#define DS18B20_IO_OUT() {GPIOG->MODER&=~(3<<(9*2));GPIOG->MODER|=1<<(9*2);} 	//PG9���ģʽ
// 
//////IO��������											   
//#define	DS18B20_DQ_OUT PGout(9) 					//���ݶ˿�	PG9
//#define	DS18B20_DQ_IN  PGin(9)  					//���ݶ˿�	PG9 
//   	
//uint8_t DS18B20_Init(void);								//��ʼ��DS18B20
//short DS18B20_Get_Temp(void);							//��ȡ�¶�
//void DS18B20_Start(void);									//��ʼ�¶�ת��
//void DS18B20_Write_Byte(uint8_t dat);			//д��һ���ֽ�
//uint8_t DS18B20_Read_Byte(void);					//����һ���ֽ�
//uint8_t DS18B20_Read_Bit(void);						//����һ��λ
//uint8_t DS18B20_Check(void);							//����Ƿ����DS18B20
//void DS18B20_Rst(void);										//��λDS18B20 

#define 	DQ_GPIO_Port GPIOA
#define 	DQ_Pin			 GPIO_PIN_11

#define 	DS18B20_DQ_OUT_LOW 				HAL_GPIO_WritePin(DQ_GPIO_Port , DQ_Pin, GPIO_PIN_RESET)
#define 	DS18B20_DQ_OUT_HIGH 			HAL_GPIO_WritePin(DQ_GPIO_Port , DQ_Pin, GPIO_PIN_SET)
#define  	DS18B20_DQ_IN             HAL_GPIO_ReadPin(DQ_GPIO_Port, DQ_Pin)

extern void DS18B20_Rst(void);
extern uint8_t DS18B20_Init(void);
extern float DS18B20_Get_Temperature_float(void);

#endif

