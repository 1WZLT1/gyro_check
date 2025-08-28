#include "A04_spi.h"

extern SPI_HandleTypeDef hspi2;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;
extern UART_HandleTypeDef huart5;

int Dev_ID;
uint8_t adcenable_flag;
volatile uint8_t ADS131A0x_Ready_flag = 0;

uint8_t ADS131A04_Reset(){
	//0.拉低reset
	HAL_GPIO_WritePin(GPIOC, ADC_RST__Pin, GPIO_PIN_RESET);
	HAL_Delay(5);
	
	//1.拉高reset：用于给ADS131A04重新上电
	HAL_GPIO_WritePin(GPIOC, ADC_RST__Pin, GPIO_PIN_SET);
	HAL_Delay(20);
	return 1;
}
/**
 * @brief 把SPI通道读数转化成电压值
 * 
 * Steps:
 *  1. Adds Core interfaces into bsp_led_driver instance target.
 *  2. Adds OS interfaces into bsp_led_driver instance target.
 *  3. Adds timebase interfaces into bsp_led_driver instance target.
 *  
 * @param[in] Data        : SPI读取的32bit数据
 * @param[in] Vref    		: 参考电压
 * @param[in] PGA			    : 内置增益
 * 
 * @return ReadVoltage 		: 计算后的电压值
 * 
 * */
double DataFormatting(uint32_t Data , double Vref ,uint8_t PGA)
{
	/*
	电压计算公式；
			设：AD采样的电压为Vin ,AD采样二进制值为X，参考电压为 Vr ,内部集成运放增益为G
			Vin = ( (Vr) / G ) * ( x / (2^23 -1))
	*/
	double ReadVoltage;
	if(Data & 0x00800000)
	{
		Data = (~Data) & 0x00FFFFFF;
		ReadVoltage = -(((double)Data) / 8388607) * ((Vref) / ((double)PGA));
	}
	else
		ReadVoltage =  (((double)Data) / 8388607) * ((Vref) / ((double)PGA));
	
	return(ReadVoltage);
}


/**
 * @brief  设置 ADS131A0x 芯片选择 (CS) 引脚状态
 * 
 * 该函数用于控制 ADS131A0x 设备的片选择引脚（CS）。通过设置引脚为低电平或高电平，
 * 可以选择或取消选择设备，从而控制 SPI 通信。
 * 
 * @param  state  片选择引脚状态：0 表示选择（拉低），1 表示取消选择（拉高）
 */
void ADS131A0xSetCS(uint8_t state) {
    // 检查状态并设置 CS 引脚
    if (0 == state) {
        // 拉低 CS 引脚以选择设备
        HAL_GPIO_WritePin(ADS131A0x_CS_GPIO_Port, ADS131A0x_CS_Pin, GPIO_PIN_RESET);
    }
    else if (1 == state) {
        // 拉高 CS 引脚以取消选择设备
        HAL_GPIO_WritePin(ADS131A0x_CS_GPIO_Port, ADS131A0x_CS_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief  进行 SPI 数据传输
 * 
 * 该函数通过 SPI 接口同时发送和接收数据。它会拉低 CS 引脚以选择设备，传输指定大小的数据，
 * 然后拉高 CS 引脚以取消选择设备。
 * 
 * @param  txData  指向要发送的数据的指针
 * @param  rxData  指向接收数据的存储位置的指针
 */
void ADS131A0xXferWord(uint8_t* txData, uint8_t* rxData) {
    

	// 拉低 CS 引脚以选择设备
    ADS131A0xSetCS(0);
    
    // 等待 SPI 接口不忙
//    while(hspi2.State == HAL_SPI_STATE_BUSY);
    
    // 进行数据传输：发送 txData，接收数据到 rxData
     HAL_SPI_TransmitReceive(&hspi2, txData, rxData, ADS131A0x_WORD_SIZE, 100);
//		HAL_SPI_TransmitReceive_DMA(&hspi2, txData, rxData, 4);
	
// 		while(hspi2.State!=HAL_SPI_STATE_READY);   
	
    // 拉高 CS 引脚以取消选择设备
    ADS131A0xSetCS(1);
}


/**
 * @brief  发送命令并接收响应
 * 
 * 该函数将一个 16 位命令分成两个 8 位数据字节，通过 SPI 接口发送给 ADS131A0x 并接收响应。
 * 第一次发送命令后，响应将在下一次发送中获得，因此需要发送一个空命令以接收响应。
 * 
 * @param  cmd  要发送的 16 位命令
 * @return  16 位的响应数据
 */
uint8_t txData[4] = {0};  			// 用于存储要发送的命令数据
uint8_t zeros[4] = {0};  			  // 用于发送空命令以获取响应
uint8_t rxData[4];  						// 用于存储接收到的响应数据
uint16_t ADS131A0xSendCmd(uint16_t cmd) {

    
    uint16_t res = 0;                             		// 存储最终的响应结果

    // 将 16 位命令拆分为 8 位数据数组
    txData[1] = (cmd & 0xff);                     		// 低字节
    txData[0] = (cmd >> 8);                       		// 高字节

    ADS131A0xXferWord(txData, rxData);								// 发送命令

    ADS131A0xXferWord(zeros, rxData);									// 由于上一个命令的响应在下一个响应中返回，因此发送另一个空命令以获取响应

    // 合并响应数据
    res = (((uint16_t)rxData[0] << 8) | rxData[1]); 	// 将接收到的两个字节合并为 16 位数据

    return res;                                   		// 返回响应结果
}


/**
 * @brief  写入单个寄存器
 * 
 * 该函数向 ADS131A0x 设备写入一个寄存器值。它将命令、寄存器地址和数据合并为一个 16 位字，
 * 然后通过 SPI 接口发送该字以完成写入操作。
 * 
 * @param  addr  要写入的寄存器地址
 * @param  data  要写入的值
 * @return  16 位的响应数据，通常是写入操作的结果
 */
uint16_t ADS131A0xWriteRegister(uint8_t addr, uint8_t data) {

    uint16_t word = ADS131A0x_CMD_WREG | (addr << 8) | data;	// 组合命令字，包含写寄存器命令、寄存器地址和数据

    uint16_t value = ADS131A0xSendCmd(word);									// 发送命令并获取响应

    return value;  																						// 返回响应结果
}


int Unclock_id = 0;
int Wakeup 		 = 0;
/**
 * @brief  初始化 ADS131A04 设备
 * 
 * 该函数用于初始化 ADS131A04 ADC。它会重置设备、解锁寄存器，并配置相关寄存器以准备设备工作。
 * 
 * @note   此函数在设备启动时调用，以确保设备处于正确的状态。
 */
uint16_t A_SYS_CFG_rec = 0;
uint16_t STAT_N_rec = 0;
uint16_t STAT_M2_rec = 0;
void ADS131A04_Init(void) 
{
    uint16_t RECEIVE = 0x0000; 																					// 接收 SPI 返回的字符  

    ADS131A04_Reset(); 																									// 重置 ADS131A04 设备

    do {  
        RECEIVE = ADS131A0xSendCmd(ADS131A04_RESET_COMMAND); 						// 发送重置命令
        HAL_Delay(5); 																									// 等待设备重置
    } while (RECEIVE != ADS131A04_READY && RECEIVE != 0xFF02); 					// 初始化成功则跳出循环

    Dev_ID = RECEIVE & 0x000F; 																					// 获取设备 ID，检查是否为 A04 型号

    RECEIVE = ADS131A0xSendCmd(ADS131A04_UNCLOCK_COMMAND); 							// 解锁寄存器
    Unclock_id = RECEIVE; 																							// 存储解锁 ID

    RECEIVE = ADS131A0xSendCmd(ADS131A0x_CMD_RREG); 										// 读取设备 ID
    RECEIVE = ADS131A0xSendCmd(ADS131A0x_CMD_RED_ID); 									// 读取设备 ID

    ADS131A0xWriteRegister(A_SYS_CFG, 0x60); 														// 设置外部参考电压 
    ADS131A0xWriteRegister(D_SYS_CFG, 0x3E); 														// 设置每帧固定六个设备字
    ADS131A0xWriteRegister(CLK1, 0x04); 																// ADC CLK1: fICLK = fCLKIN(16.384MHz) / 8 = 2.048Khz
    ADS131A0xWriteRegister(CLK2, 0x01); 																// fMOD = fICLK / 2, fDATA=fMOD/1024 = 1 024 000/1024=1000 当前为 1000Hz
    ADS131A0xWriteRegister(ADC_ENA, 0x0F); 															// 启用所有 ADC 通道

    RECEIVE = ADS131A0xSendCmd(0x2B00); 																// 读取 A_SYS_CFG
		A_SYS_CFG_rec = RECEIVE;
    RECEIVE = ADS131A0xSendCmd(0x2C00); 																// 读取 D_SYS_CFG
		
		/* 读取STAT_N */
		STAT_N_rec = ADS131A0xSendCmd(0x0400); 	

		/* 读取STAT_M2 */
		STAT_M2_rec = ADS131A0xSendCmd(0x0700); 	
		
    RECEIVE = ADS131A0xSendCmd(ADS131A04_WAKEUP_COMMAND); 							// 唤醒 ADS131 设备
		Wakeup =  RECEIVE;
    adcenable_flag = 1; 																								// 设置初始化完成标志位
		HAL_Delay(100);
}

#define A04_VREF 2.5
#define A04_GAIN 1
#define ADC_TO_ACC 98.0f
uint8_t emptyTxBuffer[ADS131A0x_WORD_SIZE*5] = {0};
uint8_t ADS131A0x_DataBuf[ADS131A0x_WORD_SIZE*5];
t_ADS131A0xData ch;

float data[3] = {0};
int Index = 0;
int ckcnt = 0;

t_ADS131A0xData ADS131A0xGetChannels(void){

		ch.Status = ADS131A0x_DataBuf[0]<<8 | ADS131A0x_DataBuf[1];
	
		ch.Ch1 = (ADS131A0x_DataBuf[4] << 16)  | (ADS131A0x_DataBuf[5] << 8)  | (ADS131A0x_DataBuf[6]);
		ch.Ch2 = (ADS131A0x_DataBuf[8] << 16)  | (ADS131A0x_DataBuf[9] << 8)  | (ADS131A0x_DataBuf[10]);
		ch.Ch3 = (ADS131A0x_DataBuf[12] << 16) | (ADS131A0x_DataBuf[13] << 8) | (ADS131A0x_DataBuf[14]);
		ch.Ch4 = (ADS131A0x_DataBuf[16] << 16) | (ADS131A0x_DataBuf[17] << 8) | (ADS131A0x_DataBuf[18]);

		ch.Volt1 = DataFormatting(ch.Ch1, A04_VREF, A04_GAIN) * ADC_TO_ACC;
		ch.Volt2 = DataFormatting(ch.Ch2, A04_VREF, A04_GAIN) * ADC_TO_ACC;
		ch.Volt3 = DataFormatting(ch.Ch3, A04_VREF, A04_GAIN) * ADC_TO_ACC;
		ch.Volt4 = DataFormatting(ch.Ch4, A04_VREF, A04_GAIN) * ADC_TO_ACC;
	
		data[Index] = ch.Volt3;
	
		if(data[0] == data[1] && data[1] == data[2])
		{
				ckcnt += 1;
		}
		
		if(Index++ == 3)Index = 0;
	
		return ch;
}



int Rx_Dry_count = 0;
//重写回调函数
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    
	/*判断触发中断的路线*/
	if(GPIO_Pin == GPIO_PIN_12) 
		{
		
			Rx_Dry_count++;
			ADS131A0x_Ready_flag = 1;	

			ADS131A0xSetCS(0);
			HAL_SPI_TransmitReceive(&hspi2, 
																	&emptyTxBuffer[0], 
																	&ADS131A0x_DataBuf[0], 
																	ADS131A0x_WORD_SIZE*5, 
																	50);
			ADS131A0xSetCS(1);
		
	}

}
//********************************************Read_Method2*********************************************//

/**
* 以下读取ADS131A04的方式是通过读取D_SYS_CFG中的数据准备位，再而发送数据读取命令
* 但是此种方式读取效率底，数据连续性较差
*/


uint8_t dummy_send[ADS131A0x_WORD_SIZE*5]={0};
uint8_t recBuffer[ADS131A0x_WORD_SIZE*5];
void Read_ADS131A0X_BUFFER_DATA(void)
{
//		ADS131A0xSetCS(0);
//		HAL_SPI_TransmitReceive(&hspi2, &dummy_send[0], &recBuffer[0], ADS131A0x_WORD_SIZE*5, 100);
////		HAL_SPI_TransmitReceive_DMA(&hspi2, &dummy_send[0], &recBuffer[0], ADS131A0x_WORD_SIZE*5);
////		while(hspi2.State!=HAL_SPI_STATE_READY);
//		ADS131A0xSetCS(1);
}

/**
 * @brief  读取D_SYS_CFG
 * 
 * 该函数从 ADS131A0X 读取D_SYS_CFG 中
 * 
 * @param  channel: 要读取的 ADC 通道（0-3）
 * @retval 返回读取的 ADC 值（24 位）
 */

uint8_t ADC_READY(void)
{
		uint16_t ret=0;
		uint16_t RECEIVE;

		RECEIVE =  	ADS131A0xSendCmd(0x2200);																	//read D_SYS_CFG
		RECEIVE = 	RECEIVE & 0x0002;

		ret = (uint8_t) RECEIVE;
	
		return ret;
}



/**
 * @brief  读取指定通道的 ADC 值
 * 
 * 该函数从 ADS131A0X 读取指定通道的 ADC 值，并返回转换后的 24 位电压值。
 * 
 * @param  channel: 要读取的 ADC 通道（0-3）
 * @retval 返回读取的 ADC 值（24 位）
 */
unsigned long Read_ADS131A0X_Value(unsigned char channel) {
    unsigned long value = 0; 																		// 存储读取的 ADC 值
    union {
        unsigned long voltage; 																	// 用于存储组合后的电压值
        struct {
            unsigned char LSB; 																	// 最低有效字节
            unsigned char NSB; 																	// 中间有效字节
            unsigned char MSB; 																	// 最高有效字节
            unsigned char d;   																	// 额外字节（未使用）
        } byte;
    } volt0, volt1, volt2, volt3;

    while (ADC_READY()); 																				// 等待 ADC 转换完成

    // 获取总线数据 24 位
    Read_ADS131A0X_BUFFER_DATA();

    // 读取每个通道的字节数据
    volt0.byte.MSB = recBuffer[4]; 															// 读取第一个通道的 MSB
    volt0.byte.NSB = recBuffer[5]; 															// 读取第一个通道的 NSB
    volt0.byte.LSB = recBuffer[6]; 															// 读取第一个通道的 LSB
    volt0.byte.d 	 = recBuffer[7];    													// 读取第一个通道的额外字节

    volt1.byte.MSB = recBuffer[8]; 															// 读取第二个通道的 MSB
    volt1.byte.NSB = recBuffer[9]; 															// 读取第二个通道的 NSB
    volt1.byte.LSB = recBuffer[10]; 														// 读取第二个通道的 LSB
    volt1.byte.d 	 = recBuffer[11];    													// 读取第二个通道的额外字节

    volt2.byte.MSB = recBuffer[12]; 														// 读取第三个通道的 MSB
    volt2.byte.NSB = recBuffer[13]; 														// 读取第三个通道的 NSB
    volt2.byte.LSB = recBuffer[14]; 														// 读取第三个通道的 LSB
    volt2.byte.d 	 = recBuffer[15];    													// 读取第三个通道的额外字节

    volt3.byte.MSB = recBuffer[16]; 														// 读取第四个通道的 MSB
    volt3.byte.NSB = recBuffer[17]; 														// 读取第四个通道的 NSB
    volt3.byte.LSB = recBuffer[18]; 														// 读取第四个通道的 LSB
    volt3.byte.d 	 = recBuffer[19];    													// 读取第四个通道的额外字节

    // 根据通道选择相应的电压值
    switch (channel) {
        case 0: value = volt0.voltage; break; 									// 读取第一个通道的值
        case 1: value = volt1.voltage; break; 									// 读取第二个通道的值
        case 2: value = volt2.voltage; break; 									// 读取第三个通道的值
        case 3: value = volt3.voltage; break; 									// 读取第四个通道的值
    }

    return value; 																							// 返回读取的 ADC 值
}


/**
 * @brief  读取 ADS131A04 的数据
 * 
 * 该函数用于读取 ADS131A04 ADC 的所有通道数据，并将其转换为电压值。
 * 
 * @note   在读取每个通道后，函数会延迟 10 毫秒，以避免过快读取造成的干扰。
 * 
 * @param  None
 * @retval None
 */
float value = 0; // 用于存储当前读取的电压值
float volt0 = 0; // 用于存储第一个通道的电压值

ADC_Volt_t ADC_Volt;

void ADS131A04_Read_data(void) {
    static unsigned long iTemp; 																// 存储读取的原始 ADC 值

    for (int i = 0; i < Dev_ID; i++) { 													// 循环读取每个 ADC 通道
        iTemp = Read_ADS131A0X_Value(i); 												// 从指定通道读取 ADC 值
        
        value = DataFormatting(iTemp, 2.5, 1); 									// 将读取的 ADC 值转换为电压值
        if (i == 0) { 																					// 如果是第一个通道
            ADC_Volt.Volt1 = value; 														// 存储第一个通道的电压值
        } else if( i == 1){
						ADC_Volt.Volt2 = value;
				} else if( i == 2){
						ADC_Volt.Volt3 = value;
				}	else if( i == 3){
						ADC_Volt.Volt4 = value;
				}
        HAL_Delay(10); 																					// 延迟 10 毫秒以避免过快读取
    }
}

int cntcs = 0;

#define F50X_X_PPM 98098335
#define F50X_Y_PPM 98693086
#define F50X_Z_PPM 98708613

#define TIM_TRANSMIT_HZ 1000

typedef struct
{ 
	int32_t Dg_raw;
	int16_t Temp_raw;
	float Dg;
	int16_t Temp;
}IXZ_F50X_t;

IXZ_F50X_t IXZ_F50X_X;
IXZ_F50X_t IXZ_F50X_Y;
IXZ_F50X_t IXZ_F50X_Z;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	if(huart == &huart1)
	{
			IXZ_F50X_X.Dg_raw = rxBuf[1] & 0x7f;
			IXZ_F50X_X.Dg_raw |= (unsigned int)(rxBuf[2] & 0x7f) << 7;
			IXZ_F50X_X.Dg_raw |= (unsigned int)(rxBuf[3] & 0x7f) << 14;
			IXZ_F50X_X.Dg_raw |= (unsigned int)(rxBuf[4] & 0x7f) << 21;
			IXZ_F50X_X.Dg_raw |= (unsigned int)(rxBuf[5] & 0x0f) << 28;
			
			IXZ_F50X_X.Temp_raw  =  rxBuf[7] & 0x7f;
			IXZ_F50X_X.Temp_raw |= (unsigned short)(rxBuf[8] & 0x7f) << 7;   

			IXZ_F50X_X.Dg		= 	((float)IXZ_F50X_X.Dg_raw / (F50X_X_PPM/TIM_TRANSMIT_HZ)); 
			IXZ_F50X_X.Temp =		IXZ_F50X_X.Temp_raw * 0.0625;		

			//HAL_UART_Receive_DMA(&huart1, rxBuf, 10);
	}
	
	if(huart == &huart2)
	{
			IXZ_F50X_Y.Dg_raw = rxBuf_1[1] & 0x7f;	
			IXZ_F50X_Y.Dg_raw |= (unsigned int)(rxBuf_1[2] & 0x7f) << 7;
			IXZ_F50X_Y.Dg_raw |= (unsigned int)(rxBuf_1[3] & 0x7f) << 14;
			IXZ_F50X_Y.Dg_raw |= (unsigned int)(rxBuf_1[4] & 0x7f) << 21;
			IXZ_F50X_Y.Dg_raw |= (unsigned int)(rxBuf_1[5] & 0x0f) << 28;
			
			IXZ_F50X_Y.Temp_raw = rxBuf_1[7] & 0x7f;
			IXZ_F50X_Y.Temp_raw |= (unsigned short)(rxBuf_1[8] & 0x7f) << 7;

			IXZ_F50X_Y.Dg		= 	((float)IXZ_F50X_Y.Dg_raw / (F50X_Y_PPM / TIM_TRANSMIT_HZ));
			IXZ_F50X_Y.Temp =		IXZ_F50X_Y.Temp_raw * 0.0625;
			
			//HAL_UART_Receive_DMA(&huart2, rxBuf_1, 10);
	}
	
	if(huart == &huart6)
	{
			IXZ_F50X_Z.Dg_raw = rxBuf_2[1] & 0x7f;	
			IXZ_F50X_Z.Dg_raw |= (unsigned int)(rxBuf_2[2] & 0x7f) << 7;	
			IXZ_F50X_Z.Dg_raw |= (unsigned int)(rxBuf_2[3] & 0x7f) << 14;	
			IXZ_F50X_Z.Dg_raw |= (unsigned int)(rxBuf_2[4] & 0x7f) << 21;	
			IXZ_F50X_Z.Dg_raw |= (unsigned int)(rxBuf_2[5] & 0x0f) << 28;	
				
				
			//合成温度数据	
			IXZ_F50X_Z.Temp_raw = rxBuf_2[7] & 0x7f;	
			IXZ_F50X_Z.Temp_raw |= (unsigned short)(rxBuf_2[8] & 0x7f) << 7;   	
				
			//数据比例转换	
			IXZ_F50X_Z.Dg		= 	((float)IXZ_F50X_Z.Dg_raw / (F50X_Z_PPM / TIM_TRANSMIT_HZ));	
			IXZ_F50X_Z.Temp =		IXZ_F50X_Z.Temp_raw * 0.0625;	
		
		  //HAL_UART_Receive_DMA(&huart6, rxBuf_2, 10);
	}
}

#pragma pack(push, 1)  // 设置 1 字节对齐
typedef struct
{
	  uint8_t Frame_header_01;			/* 帧头						 			*/
		uint8_t Frame_header_02;			/* 帧头						 			*/
		uint8_t Send_count;						/* 数据包发送计数				*/
	
		float Dg_X;										/* 光纤陀螺X轴数据 			*/
		float Dg_Y;										/* 光纤陀螺Y轴数据 			*/
		float Dg_Z;										/* 光纤陀螺Z轴数据 			*/
	
		float Acc_X;									/* 石英加表X轴数据 			*/
		float Acc_Y;									/* 石英加表Y轴数据 			*/
		float Acc_Z;									/* 石英加表Z轴数据 			*///27
	
		int16_t Temp_X;								/* 光纤陀螺X轴温度数据 	*/
		int16_t Temp_Y;								/* 光纤陀螺Y轴温度数据 	*/
		int16_t Temp_Z;								/* 光纤陀螺Z轴温度数据 	*/
	
		int16_t Acc_Temp;							/* 石英加表温度数据 		*///8
	
		uint8_t Check_sum;						/* 校验和						 		*/
		uint8_t Frame_tail;						/* 帧尾						 			*/ //37
}Fog_Transmit_t;

#define TEMP_K 16
Fog_Transmit_t Date_Transmit;

uint8_t calculate_checksum(Fog_Transmit_t* transmit) 
{
			/* 定义最终存储异或和的变量 */
			uint8_t checksum = 0;																			

			/* 将 float 类型数据的字节逐一进行异或 */
			uint8_t* data_ptr = (uint8_t*)&transmit->Dg_X; 						/* 获取 Dg_X 的指针					  */
			for (size_t i = 0; i < sizeof(float) * 3; ++i) 						/* 获取指针后循环遍历每个字节 */
			{
					checksum ^= data_ptr[i]; 															/* Dg_X, Dg_Y, Dg_Z						*/
			}

			data_ptr = (uint8_t*)&transmit->Acc_X; 										/* 获取 Acc_X 的指针					*/
			for (size_t i = 0; i < sizeof(float) * 3; ++i) 
			{
					checksum ^= data_ptr[i]; 															/* Acc_X, Acc_Y, Acc_Z				*/
			}

			/* 将 int16_t 类型数据逐一进行异或 */
			checksum ^= (uint8_t)(transmit->Temp_X);
			checksum ^= (uint8_t)(transmit->Temp_X >> 8);
			checksum ^= (uint8_t)(transmit->Temp_Y);
			checksum ^= (uint8_t)(transmit->Temp_Y >> 8);
			checksum ^= (uint8_t)(transmit->Temp_Z);
			checksum ^= (uint8_t)(transmit->Temp_Z >> 8);
			checksum ^= (uint8_t)(transmit->Acc_Temp);
			checksum ^= (uint8_t)(transmit->Acc_Temp >> 8);

			return checksum;
}


void Date_Transmit_All()
{
			Date_Transmit.Frame_header_01 = 0xAA;																			/* 帧头赋值							  */
			Date_Transmit.Frame_header_02 = 0xAA;																			/* 帧头赋值							  */
			Date_Transmit.Send_count			= 00;																				/* 帧头赋值							  */

			Date_Transmit.Dg_X 						= IXZ_F50X_X.Dg;														/* X轴陀螺赋值					  */
			Date_Transmit.Dg_Y 						= IXZ_F50X_Y.Dg;														/* Y轴陀螺赋值					  */
			Date_Transmit.Dg_Z 						= IXZ_F50X_Z.Dg;														/* Z轴陀螺赋值					  */

			Date_Transmit.Temp_X 					= IXZ_F50X_X.Temp * TEMP_K;									/* X轴陀螺温度赋值			  */
			Date_Transmit.Temp_Y 					= IXZ_F50X_Y.Temp * TEMP_K;									/* Y轴陀螺温度赋值			  */
			Date_Transmit.Temp_Z 					= IXZ_F50X_Z.Temp * TEMP_K;									/* Z轴陀螺温度赋值			  */

			Date_Transmit.Acc_X 					= ch.Volt1;																	/* X轴加表赋值					  */
			Date_Transmit.Acc_Y 					= ch.Volt2;																	/* Y轴加表赋值					  */
			Date_Transmit.Acc_Z 					= ch.Volt3;																	/* Z轴加表赋值					  */

			Date_Transmit.Acc_Temp				= tempreature;			/* 加表温度赋值					  */


			Date_Transmit.Check_sum 			= calculate_checksum(&Date_Transmit);				/* 计算并赋值 Check_sum  	*/
			Date_Transmit.Frame_tail			= 0xBB;


			HAL_UART_Transmit_DMA(&huart5, 
														(uint8_t*)&Date_Transmit, 
														sizeof(Date_Transmit));															/* 通过DMA将数据发送		 	*/
}

//********************************************Read_Method2*********************************************//


