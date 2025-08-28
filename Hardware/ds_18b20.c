#include "ds_18b20.h"

// 【01】可能存在数据为0的情况：https://blog.csdn.net/liuyy_2000/article/details/113754150
// 【02】https://shequ.stmicroelectronics.cn/thread-606910-1-1.html


/****************************************************************************
函数名：DS18B20_IO_IN
功能：使DS18B20_DQ引脚变为输入模式
输入：无
输出：无
返回值：无
备注：DQ引脚为PA11
****************************************************************************/
void DS18B20_IO_IN(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = GPIO_PIN_11;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(GPIOA,&GPIO_InitStructure);
}


/****************************************************************************
函数名：DS18B20_IO_OUT
功能：使DS18B20_DQ引脚变为推挽输出模式
输入：无
输出：无
返回值：无
备注：DQ引脚为PA11
****************************************************************************/
void DS18B20_IO_OUT(void){
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin = GPIO_PIN_11;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOA,&GPIO_InitStructure);
}


/***************************************************************************
函数名：DS18B20_Rst
功  能：发送复位信号
输  入: 无
输  出：无
返回值：无
备  注：
***************************************************************************/
void DS18B20_Rst(void){
	DS18B20_IO_OUT();//引脚输出模式
	
	//拉低总线并延时750us
	DS18B20_DQ_OUT_LOW;
	delay_us(750);     
	
	//释放总线为高电平并延时等待15~60us
	DS18B20_DQ_OUT_HIGH;
	delay_us(15);
}

/***************************************************************************
函数名：DS18B20_Check
功  能：检测DS18B20返回的存在脉冲
输  入: 无
输  出：无
返回值：0:成功  1：失败   2:释放总线失败
备  注：
***************************************************************************/
uint8_t DS18B20_Check(void){
	
	uint8_t retry = 0;																// 定义一个脉冲持续时间
	
	DS18B20_IO_IN();																	// 引脚设为输入模式
	
	while(DS18B20_DQ_IN && retry < 200){							// 等待回复为低电平-->冗余检测低电平200微妙，如果为低电平则直接跳出循环
		retry++;
		delay_us(1);
	}
	
	if(retry >= 200)																	// 如果重试次数大于200，则认为没有返回低电平
		return 1;
	else
		retry = 0;
	
	while(!DS18B20_DQ_IN && retry < 240){							//判断DS18B20是否释放总线-->冗余检测高电平200微妙
		retry++;
		delay_us(1);
	}
	
	if(retry >= 240)
		return 2;
	
	return 0;
}


/***************************************************************************
函数名：DS18B20_Write_Byte
功  能：向DS18B20写一个字节
输  入: 要写入的字节
输  出：无
返回值：无
备  注：@1# 写时序周期60~120微妙，作为从机的DS18B20则在检测到总线被拉底后等待15微秒
						然后从15us到45us开始对总线采样，在采样期内总线为高电平则为1，若采样期内
						总线为低电平则为0。
				@2# 注意时序结束后都需要将电平拉高
***************************************************************************/
void DS18B20_Write_Byte(uint8_t data){
	uint8_t j;
	uint8_t databit;
	
	DS18B20_IO_OUT();												// 设置DQ为输出模式，写命令是主机输出
	
	for(j=1;j<=8;j++){
		
		// 为下一次写做准备
		databit=data&0x01;										// 取数据最低位
		data=data>>1;     										// 右移一位
		
		// 实际开始写操作
		if(databit){      										// 当前位写1
			DS18B20_DQ_OUT_LOW;
			delay_us(2);												// 15us到45us采样周期内为高电平
			DS18B20_DQ_OUT_HIGH;								
			delay_us(60);
		}else{          											// 当前位写0
			DS18B20_DQ_OUT_LOW;
			delay_us(60);												// 15us到45us采样周期内为低电平
			DS18B20_DQ_OUT_HIGH;
			delay_us(2);
		}
	}
}

/***************************************************************************
函数名：DS18B20_Read_Bit
功  能：向DS18B20读一个位
输  入: 无
输  出：无
返回值：读入数据
备  注：@1# 读时序：当总线控制器把数据线从高电平拉到低电平时，读时序开始，数
						据线必须至少保持 1us,然后总线被释放。
				@2# 在总线控制器发出读时序后，DS18B20通过拉高或拉低总线上来传输1或0
				@3# 从DS18B20输出的数据在读时序的下降沿出现后15us内有效
***************************************************************************/
uint8_t DS18B20_Read_Bit(void){
	uint8_t data;
	
	DS18B20_IO_OUT();											// 读取之前为什么要先输出并拉低两微秒？						
	DS18B20_DQ_OUT_LOW;
	delay_us(2);													// 当总线控制器把数据线从高电平拉到低电平时，读时序开始，数
																				// 据线必须至少保持 1us,然后总线被释放
	DS18B20_DQ_OUT_HIGH;
	
	DS18B20_IO_IN();											// 转换为输入模式，对电平进行检测
	delay_us(12);
	
	if(DS18B20_DQ_IN)
		data = 1;
	else
		data = 0;
	
	delay_us(50);													// 所有读时序必须最少60us
	
	return data;
}

/***************************************************************************
函数名：DS18B20_Read_Byte
功  能：向DS18B20读一个字节
输  入: 无
输  出：无
返回值：读入数据
备  注：
***************************************************************************/
uint8_t DS18B20_Read_Byte(void){
	uint8_t i,j,data;
	data = 0;
	
	for(i=1;i<=8;i++){
		j = DS18B20_Read_Bit();
		data = (j<<7)|(data>>1);						// 先读的是低位，而后依次往高位写入
		
		/*j=0或1，j<<7=0x00或0x80，和data右移一位相或，即把1/0写入最高位，下次再往右移位*/

	}
	return data;
}


/***************************************************************************
函数名：DS18B20_Start
功  能：DS18B20开启
输  入: 无
输  出：无
返回值：无
备  注：@1# 注意：每次进行数据读取的时候，都需要从复位信号开始，也就是说：
						无论进行何种操作，都需要先和模块握手成功。成功后才能经行读写操作。
***************************************************************************/
void DS18B20_Start(void){
			DS18B20_Rst();											// 发送复位信号
			DS18B20_Check();										// 握手检测
			DS18B20_Write_Byte(0xcc);						// 跳过ROM
			DS18B20_Write_Byte(0x44);						// 温度变换命令
}


/***************************************************************************
函数名：DS18B20_Init
功  能：DS18B20初始化
输  入: 无
输  出：无
返回值：无
备  注：
***************************************************************************/
uint8_t DS18B20_Init(void)
{
			//引脚初始化
			GPIO_InitTypeDef GPIO_InitStructure;
			GPIO_InitStructure.Pin = DQ_Pin;
			GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
			GPIO_InitStructure.Pull = GPIO_PULLUP;
			GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
			HAL_GPIO_Init(DQ_GPIO_Port,&GPIO_InitStructure);

			DS18B20_Rst();
			return DS18B20_Check();
}

/***************************************************************************
函数名：DS18B20_Read_Temperature
功  能：读取一次温度
输  入: 无
输  出：无
返回值：读取到的温度数据
备  注：适用于总线上只有一个DS18B20的情况
***************************************************************************/
float DS18B20_Get_Temperature_float(void)
{
			uint8_t tpmsb, tplsb;
			short s_tem;
			float f_tem;


			DS18B20_Start();																	// @1# 发送握手以及44命令，以告知模块进行数据的准备
																												// @2# 手册中写到：在发送过0X44命令后需要【DQ引脚保持至少500ms高电平，以完成温度转换】
			DS18B20_Rst();																		// 还是没看懂这里为什么再次进行复位和握手操作，我觉得可以省略，但是
			DS18B20_Check();
			DS18B20_Write_Byte(0xcc);													//跳过ROM
			DS18B20_Write_Byte(0xbe);													//读暂存器
			tplsb = DS18B20_Read_Byte();													//低八位
			tpmsb = DS18B20_Read_Byte();													//高八位

			s_tem = tpmsb<<8;
			s_tem = s_tem | tplsb;

			if( s_tem < 0 )		/* 负温度 */
			{
					f_tem = (~s_tem+1) * 0.0625;
			}	
			else
			{
					f_tem = s_tem * 0.0625;
			}
			
			return f_tem; 
}
