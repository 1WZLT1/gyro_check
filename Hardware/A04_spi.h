#ifndef __A04_SPI_H
#define __A04_SPI_H

#include <stdio.h>
#include <stdint.h>
#include "main.h"
#include "tim.h"


#define ADC_RST_GPIO_Port						GPIOC
#define ADC_RST__Pin 								GPIO_PIN_2
#define ADS131A0x_CS_GPIO_Port			GPIOB
#define ADS131A0x_CS_Pin 						GPIO_PIN_12


#define ADS131A0x_WORD_SIZE	4



typedef struct {
	uint32_t Ch1;
	uint32_t Ch2;
	uint32_t Ch3;
	uint32_t Ch4;
	uint16_t Status;
	uint16_t Checksum;
	
	float Volt1;
	float Volt2;
	float Volt3;
	float Volt4;
	
} t_ADS131A0xData;


//System Commands
#define ADS131A04_NULL_COMMAND											0X0000
#define ADS131A04_RESET_COMMAND											0X0011
#define ADS131A04_STANDBY_COMMAND										0X0022
#define ADS131A04_WAKEUP_COMMAND										0X0033
#define ADS131A04_LOCK_COMMAND											0X0555
#define ADS131A04_UNCLOCK_COMMAND										0X0655

//Registers Read/Write Commands
#define ADS131A0x_CMD_RREG													0x2000
#define ADS131A0x_CMD_RED_ID												0x2100
#define ADS131A0x_CMD_RREGS													0x2000
#define ADS131A0x_CMD_WREG													0x4000
#define ADS131A0x_CMD_WREGS													0x6000


//system responses
#define ADS131A04_READY															0xFF04
#define ADS131A04_STANDBY_ACK												0X0022
#define ADS131A04_WAKEUP_ACK												0X0033
#define ADS131A04_LOCK_ACK													0X0555
#define ADS131A04_UNCLOCK_ACK												0X0655

//Register Addresses
#define ID_MSB																			0x00 //ID Control Register MSB
#define ID_LSB																			0x01 //ID Control Register LSB
#define STAT_1																			0x02 //Status 1 Register
#define STAT_P																			0x03 //Positive Input Fault Detect Status
#define STAT_N																			0x04 //Negative Input Fault Detect Status
#define STAT_S																			0x05 //SPI Status Register
#define ERROR_CNT																		0x06 //Error Count Register
#define STAT_M2																			0x07 //Hardware Mode Pin Status Register
//#define RESERVED														0x08
//#define RESERVED														0x09
//#define RESERVED														0x0A
#define A_SYS_CFG																		0x0B //Analog System Configuration Register
#define D_SYS_CFG																		0x0C //Digital System Configuration Register
#define CLK1																				0x0D //Clock Configuration 1 Register
#define CLK2																				0x0E //Clock Configuration 2 Register
#define ADC_ENA																			0x0F //ADC Channel Enable Register
//#define RESERVED														0x10
#define ADS131_ADC1																	0x11 //ADC Channel 1 Digital Gain Configuration
#define ADS131_ADC2																	0x12 //ADC Channel 2 Digital Gain Configuration
#define ADS131_ADC3																	0x13 //ADC Channel 3 Digital Gain Configuration
#define ADS131_ADC4																	0x14 //ADC Channel 4 Digital Gain Configuration

extern void ADS131A04_Init(void);
extern void ADS131A04_Read_data(void);
extern t_ADS131A0xData ADS131A0xGetChannels(void);
void Date_Transmit_All(void);

//extern void delay_us(__IO uint32_t delay);
typedef struct {

	float Volt1;
	float Volt2;
	float Volt3;
	float Volt4;	
} ADC_Volt_t; 
extern ADC_Volt_t ADC_Volt;
extern t_ADS131A0xData ch;
#endif
