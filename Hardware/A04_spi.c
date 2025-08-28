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
	//0.����reset
	HAL_GPIO_WritePin(GPIOC, ADC_RST__Pin, GPIO_PIN_RESET);
	HAL_Delay(5);
	
	//1.����reset�����ڸ�ADS131A04�����ϵ�
	HAL_GPIO_WritePin(GPIOC, ADC_RST__Pin, GPIO_PIN_SET);
	HAL_Delay(20);
	return 1;
}
/**
 * @brief ��SPIͨ������ת���ɵ�ѹֵ
 * 
 * Steps:
 *  1. Adds Core interfaces into bsp_led_driver instance target.
 *  2. Adds OS interfaces into bsp_led_driver instance target.
 *  3. Adds timebase interfaces into bsp_led_driver instance target.
 *  
 * @param[in] Data        : SPI��ȡ��32bit����
 * @param[in] Vref    		: �ο���ѹ
 * @param[in] PGA			    : ��������
 * 
 * @return ReadVoltage 		: �����ĵ�ѹֵ
 * 
 * */
double DataFormatting(uint32_t Data , double Vref ,uint8_t PGA)
{
	/*
	��ѹ���㹫ʽ��
			�裺AD�����ĵ�ѹΪVin ,AD����������ֵΪX���ο���ѹΪ Vr ,�ڲ������˷�����ΪG
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
 * @brief  ���� ADS131A0x оƬѡ�� (CS) ����״̬
 * 
 * �ú������ڿ��� ADS131A0x �豸��Ƭѡ�����ţ�CS����ͨ����������Ϊ�͵�ƽ��ߵ�ƽ��
 * ����ѡ���ȡ��ѡ���豸���Ӷ����� SPI ͨ�š�
 * 
 * @param  state  Ƭѡ������״̬��0 ��ʾѡ�����ͣ���1 ��ʾȡ��ѡ�����ߣ�
 */
void ADS131A0xSetCS(uint8_t state) {
    // ���״̬������ CS ����
    if (0 == state) {
        // ���� CS ������ѡ���豸
        HAL_GPIO_WritePin(ADS131A0x_CS_GPIO_Port, ADS131A0x_CS_Pin, GPIO_PIN_RESET);
    }
    else if (1 == state) {
        // ���� CS ������ȡ��ѡ���豸
        HAL_GPIO_WritePin(ADS131A0x_CS_GPIO_Port, ADS131A0x_CS_Pin, GPIO_PIN_SET);
    }
}

/**
 * @brief  ���� SPI ���ݴ���
 * 
 * �ú���ͨ�� SPI �ӿ�ͬʱ���ͺͽ������ݡ��������� CS ������ѡ���豸������ָ����С�����ݣ�
 * Ȼ������ CS ������ȡ��ѡ���豸��
 * 
 * @param  txData  ָ��Ҫ���͵����ݵ�ָ��
 * @param  rxData  ָ��������ݵĴ洢λ�õ�ָ��
 */
void ADS131A0xXferWord(uint8_t* txData, uint8_t* rxData) {
    

	// ���� CS ������ѡ���豸
    ADS131A0xSetCS(0);
    
    // �ȴ� SPI �ӿڲ�æ
//    while(hspi2.State == HAL_SPI_STATE_BUSY);
    
    // �������ݴ��䣺���� txData���������ݵ� rxData
     HAL_SPI_TransmitReceive(&hspi2, txData, rxData, ADS131A0x_WORD_SIZE, 100);
//		HAL_SPI_TransmitReceive_DMA(&hspi2, txData, rxData, 4);
	
// 		while(hspi2.State!=HAL_SPI_STATE_READY);   
	
    // ���� CS ������ȡ��ѡ���豸
    ADS131A0xSetCS(1);
}


/**
 * @brief  �������������Ӧ
 * 
 * �ú�����һ�� 16 λ����ֳ����� 8 λ�����ֽڣ�ͨ�� SPI �ӿڷ��͸� ADS131A0x ��������Ӧ��
 * ��һ�η����������Ӧ������һ�η����л�ã������Ҫ����һ���������Խ�����Ӧ��
 * 
 * @param  cmd  Ҫ���͵� 16 λ����
 * @return  16 λ����Ӧ����
 */
uint8_t txData[4] = {0};  			// ���ڴ洢Ҫ���͵���������
uint8_t zeros[4] = {0};  			  // ���ڷ��Ϳ������Ի�ȡ��Ӧ
uint8_t rxData[4];  						// ���ڴ洢���յ�����Ӧ����
uint16_t ADS131A0xSendCmd(uint16_t cmd) {

    
    uint16_t res = 0;                             		// �洢���յ���Ӧ���

    // �� 16 λ������Ϊ 8 λ��������
    txData[1] = (cmd & 0xff);                     		// ���ֽ�
    txData[0] = (cmd >> 8);                       		// ���ֽ�

    ADS131A0xXferWord(txData, rxData);								// ��������

    ADS131A0xXferWord(zeros, rxData);									// ������һ���������Ӧ����һ����Ӧ�з��أ���˷�����һ���������Ի�ȡ��Ӧ

    // �ϲ���Ӧ����
    res = (((uint16_t)rxData[0] << 8) | rxData[1]); 	// �����յ��������ֽںϲ�Ϊ 16 λ����

    return res;                                   		// ������Ӧ���
}


/**
 * @brief  д�뵥���Ĵ���
 * 
 * �ú����� ADS131A0x �豸д��һ���Ĵ���ֵ����������Ĵ�����ַ�����ݺϲ�Ϊһ�� 16 λ�֣�
 * Ȼ��ͨ�� SPI �ӿڷ��͸��������д�������
 * 
 * @param  addr  Ҫд��ļĴ�����ַ
 * @param  data  Ҫд���ֵ
 * @return  16 λ����Ӧ���ݣ�ͨ����д������Ľ��
 */
uint16_t ADS131A0xWriteRegister(uint8_t addr, uint8_t data) {

    uint16_t word = ADS131A0x_CMD_WREG | (addr << 8) | data;	// ��������֣�����д�Ĵ�������Ĵ�����ַ������

    uint16_t value = ADS131A0xSendCmd(word);									// ���������ȡ��Ӧ

    return value;  																						// ������Ӧ���
}


int Unclock_id = 0;
int Wakeup 		 = 0;
/**
 * @brief  ��ʼ�� ADS131A04 �豸
 * 
 * �ú������ڳ�ʼ�� ADS131A04 ADC�����������豸�������Ĵ�������������ؼĴ�����׼���豸������
 * 
 * @note   �˺������豸����ʱ���ã���ȷ���豸������ȷ��״̬��
 */
uint16_t A_SYS_CFG_rec = 0;
uint16_t STAT_N_rec = 0;
uint16_t STAT_M2_rec = 0;
void ADS131A04_Init(void) 
{
    uint16_t RECEIVE = 0x0000; 																					// ���� SPI ���ص��ַ�  

    ADS131A04_Reset(); 																									// ���� ADS131A04 �豸

    do {  
        RECEIVE = ADS131A0xSendCmd(ADS131A04_RESET_COMMAND); 						// ������������
        HAL_Delay(5); 																									// �ȴ��豸����
    } while (RECEIVE != ADS131A04_READY && RECEIVE != 0xFF02); 					// ��ʼ���ɹ�������ѭ��

    Dev_ID = RECEIVE & 0x000F; 																					// ��ȡ�豸 ID������Ƿ�Ϊ A04 �ͺ�

    RECEIVE = ADS131A0xSendCmd(ADS131A04_UNCLOCK_COMMAND); 							// �����Ĵ���
    Unclock_id = RECEIVE; 																							// �洢���� ID

    RECEIVE = ADS131A0xSendCmd(ADS131A0x_CMD_RREG); 										// ��ȡ�豸 ID
    RECEIVE = ADS131A0xSendCmd(ADS131A0x_CMD_RED_ID); 									// ��ȡ�豸 ID

    ADS131A0xWriteRegister(A_SYS_CFG, 0x60); 														// �����ⲿ�ο���ѹ 
    ADS131A0xWriteRegister(D_SYS_CFG, 0x3E); 														// ����ÿ֡�̶������豸��
    ADS131A0xWriteRegister(CLK1, 0x04); 																// ADC CLK1: fICLK = fCLKIN(16.384MHz) / 8 = 2.048Khz
    ADS131A0xWriteRegister(CLK2, 0x01); 																// fMOD = fICLK / 2, fDATA=fMOD/1024 = 1 024 000/1024=1000 ��ǰΪ 1000Hz
    ADS131A0xWriteRegister(ADC_ENA, 0x0F); 															// �������� ADC ͨ��

    RECEIVE = ADS131A0xSendCmd(0x2B00); 																// ��ȡ A_SYS_CFG
		A_SYS_CFG_rec = RECEIVE;
    RECEIVE = ADS131A0xSendCmd(0x2C00); 																// ��ȡ D_SYS_CFG
		
		/* ��ȡSTAT_N */
		STAT_N_rec = ADS131A0xSendCmd(0x0400); 	

		/* ��ȡSTAT_M2 */
		STAT_M2_rec = ADS131A0xSendCmd(0x0700); 	
		
    RECEIVE = ADS131A0xSendCmd(ADS131A04_WAKEUP_COMMAND); 							// ���� ADS131 �豸
		Wakeup =  RECEIVE;
    adcenable_flag = 1; 																								// ���ó�ʼ����ɱ�־λ
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
//��д�ص�����
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    
	/*�жϴ����жϵ�·��*/
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
* ���¶�ȡADS131A04�ķ�ʽ��ͨ����ȡD_SYS_CFG�е�����׼��λ���ٶ��������ݶ�ȡ����
* ���Ǵ��ַ�ʽ��ȡЧ�ʵף����������Խϲ�
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
 * @brief  ��ȡD_SYS_CFG
 * 
 * �ú����� ADS131A0X ��ȡD_SYS_CFG ��
 * 
 * @param  channel: Ҫ��ȡ�� ADC ͨ����0-3��
 * @retval ���ض�ȡ�� ADC ֵ��24 λ��
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
 * @brief  ��ȡָ��ͨ���� ADC ֵ
 * 
 * �ú����� ADS131A0X ��ȡָ��ͨ���� ADC ֵ��������ת����� 24 λ��ѹֵ��
 * 
 * @param  channel: Ҫ��ȡ�� ADC ͨ����0-3��
 * @retval ���ض�ȡ�� ADC ֵ��24 λ��
 */
unsigned long Read_ADS131A0X_Value(unsigned char channel) {
    unsigned long value = 0; 																		// �洢��ȡ�� ADC ֵ
    union {
        unsigned long voltage; 																	// ���ڴ洢��Ϻ�ĵ�ѹֵ
        struct {
            unsigned char LSB; 																	// �����Ч�ֽ�
            unsigned char NSB; 																	// �м���Ч�ֽ�
            unsigned char MSB; 																	// �����Ч�ֽ�
            unsigned char d;   																	// �����ֽڣ�δʹ�ã�
        } byte;
    } volt0, volt1, volt2, volt3;

    while (ADC_READY()); 																				// �ȴ� ADC ת�����

    // ��ȡ�������� 24 λ
    Read_ADS131A0X_BUFFER_DATA();

    // ��ȡÿ��ͨ�����ֽ�����
    volt0.byte.MSB = recBuffer[4]; 															// ��ȡ��һ��ͨ���� MSB
    volt0.byte.NSB = recBuffer[5]; 															// ��ȡ��һ��ͨ���� NSB
    volt0.byte.LSB = recBuffer[6]; 															// ��ȡ��һ��ͨ���� LSB
    volt0.byte.d 	 = recBuffer[7];    													// ��ȡ��һ��ͨ���Ķ����ֽ�

    volt1.byte.MSB = recBuffer[8]; 															// ��ȡ�ڶ���ͨ���� MSB
    volt1.byte.NSB = recBuffer[9]; 															// ��ȡ�ڶ���ͨ���� NSB
    volt1.byte.LSB = recBuffer[10]; 														// ��ȡ�ڶ���ͨ���� LSB
    volt1.byte.d 	 = recBuffer[11];    													// ��ȡ�ڶ���ͨ���Ķ����ֽ�

    volt2.byte.MSB = recBuffer[12]; 														// ��ȡ������ͨ���� MSB
    volt2.byte.NSB = recBuffer[13]; 														// ��ȡ������ͨ���� NSB
    volt2.byte.LSB = recBuffer[14]; 														// ��ȡ������ͨ���� LSB
    volt2.byte.d 	 = recBuffer[15];    													// ��ȡ������ͨ���Ķ����ֽ�

    volt3.byte.MSB = recBuffer[16]; 														// ��ȡ���ĸ�ͨ���� MSB
    volt3.byte.NSB = recBuffer[17]; 														// ��ȡ���ĸ�ͨ���� NSB
    volt3.byte.LSB = recBuffer[18]; 														// ��ȡ���ĸ�ͨ���� LSB
    volt3.byte.d 	 = recBuffer[19];    													// ��ȡ���ĸ�ͨ���Ķ����ֽ�

    // ����ͨ��ѡ����Ӧ�ĵ�ѹֵ
    switch (channel) {
        case 0: value = volt0.voltage; break; 									// ��ȡ��һ��ͨ����ֵ
        case 1: value = volt1.voltage; break; 									// ��ȡ�ڶ���ͨ����ֵ
        case 2: value = volt2.voltage; break; 									// ��ȡ������ͨ����ֵ
        case 3: value = volt3.voltage; break; 									// ��ȡ���ĸ�ͨ����ֵ
    }

    return value; 																							// ���ض�ȡ�� ADC ֵ
}


/**
 * @brief  ��ȡ ADS131A04 ������
 * 
 * �ú������ڶ�ȡ ADS131A04 ADC ������ͨ�����ݣ�������ת��Ϊ��ѹֵ��
 * 
 * @note   �ڶ�ȡÿ��ͨ���󣬺������ӳ� 10 ���룬�Ա�������ȡ��ɵĸ��š�
 * 
 * @param  None
 * @retval None
 */
float value = 0; // ���ڴ洢��ǰ��ȡ�ĵ�ѹֵ
float volt0 = 0; // ���ڴ洢��һ��ͨ���ĵ�ѹֵ

ADC_Volt_t ADC_Volt;

void ADS131A04_Read_data(void) {
    static unsigned long iTemp; 																// �洢��ȡ��ԭʼ ADC ֵ

    for (int i = 0; i < Dev_ID; i++) { 													// ѭ����ȡÿ�� ADC ͨ��
        iTemp = Read_ADS131A0X_Value(i); 												// ��ָ��ͨ����ȡ ADC ֵ
        
        value = DataFormatting(iTemp, 2.5, 1); 									// ����ȡ�� ADC ֵת��Ϊ��ѹֵ
        if (i == 0) { 																					// ����ǵ�һ��ͨ��
            ADC_Volt.Volt1 = value; 														// �洢��һ��ͨ���ĵ�ѹֵ
        } else if( i == 1){
						ADC_Volt.Volt2 = value;
				} else if( i == 2){
						ADC_Volt.Volt3 = value;
				}	else if( i == 3){
						ADC_Volt.Volt4 = value;
				}
        HAL_Delay(10); 																					// �ӳ� 10 �����Ա�������ȡ
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
				
				
			//�ϳ��¶�����	
			IXZ_F50X_Z.Temp_raw = rxBuf_2[7] & 0x7f;	
			IXZ_F50X_Z.Temp_raw |= (unsigned short)(rxBuf_2[8] & 0x7f) << 7;   	
				
			//���ݱ���ת��	
			IXZ_F50X_Z.Dg		= 	((float)IXZ_F50X_Z.Dg_raw / (F50X_Z_PPM / TIM_TRANSMIT_HZ));	
			IXZ_F50X_Z.Temp =		IXZ_F50X_Z.Temp_raw * 0.0625;	
		
		  //HAL_UART_Receive_DMA(&huart6, rxBuf_2, 10);
	}
}

#pragma pack(push, 1)  // ���� 1 �ֽڶ���
typedef struct
{
	  uint8_t Frame_header_01;			/* ֡ͷ						 			*/
		uint8_t Frame_header_02;			/* ֡ͷ						 			*/
		uint8_t Send_count;						/* ���ݰ����ͼ���				*/
	
		float Dg_X;										/* ��������X������ 			*/
		float Dg_Y;										/* ��������Y������ 			*/
		float Dg_Z;										/* ��������Z������ 			*/
	
		float Acc_X;									/* ʯӢ�ӱ�X������ 			*/
		float Acc_Y;									/* ʯӢ�ӱ�Y������ 			*/
		float Acc_Z;									/* ʯӢ�ӱ�Z������ 			*///27
	
		int16_t Temp_X;								/* ��������X���¶����� 	*/
		int16_t Temp_Y;								/* ��������Y���¶����� 	*/
		int16_t Temp_Z;								/* ��������Z���¶����� 	*/
	
		int16_t Acc_Temp;							/* ʯӢ�ӱ��¶����� 		*///8
	
		uint8_t Check_sum;						/* У���						 		*/
		uint8_t Frame_tail;						/* ֡β						 			*/ //37
}Fog_Transmit_t;

#define TEMP_K 16
Fog_Transmit_t Date_Transmit;

uint8_t calculate_checksum(Fog_Transmit_t* transmit) 
{
			/* �������մ洢���͵ı��� */
			uint8_t checksum = 0;																			

			/* �� float �������ݵ��ֽ���һ������� */
			uint8_t* data_ptr = (uint8_t*)&transmit->Dg_X; 						/* ��ȡ Dg_X ��ָ��					  */
			for (size_t i = 0; i < sizeof(float) * 3; ++i) 						/* ��ȡָ���ѭ������ÿ���ֽ� */
			{
					checksum ^= data_ptr[i]; 															/* Dg_X, Dg_Y, Dg_Z						*/
			}

			data_ptr = (uint8_t*)&transmit->Acc_X; 										/* ��ȡ Acc_X ��ָ��					*/
			for (size_t i = 0; i < sizeof(float) * 3; ++i) 
			{
					checksum ^= data_ptr[i]; 															/* Acc_X, Acc_Y, Acc_Z				*/
			}

			/* �� int16_t ����������һ������� */
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
			Date_Transmit.Frame_header_01 = 0xAA;																			/* ֡ͷ��ֵ							  */
			Date_Transmit.Frame_header_02 = 0xAA;																			/* ֡ͷ��ֵ							  */
			Date_Transmit.Send_count			= 00;																				/* ֡ͷ��ֵ							  */

			Date_Transmit.Dg_X 						= IXZ_F50X_X.Dg;														/* X�����ݸ�ֵ					  */
			Date_Transmit.Dg_Y 						= IXZ_F50X_Y.Dg;														/* Y�����ݸ�ֵ					  */
			Date_Transmit.Dg_Z 						= IXZ_F50X_Z.Dg;														/* Z�����ݸ�ֵ					  */

			Date_Transmit.Temp_X 					= IXZ_F50X_X.Temp * TEMP_K;									/* X�������¶ȸ�ֵ			  */
			Date_Transmit.Temp_Y 					= IXZ_F50X_Y.Temp * TEMP_K;									/* Y�������¶ȸ�ֵ			  */
			Date_Transmit.Temp_Z 					= IXZ_F50X_Z.Temp * TEMP_K;									/* Z�������¶ȸ�ֵ			  */

			Date_Transmit.Acc_X 					= ch.Volt1;																	/* X��ӱ�ֵ					  */
			Date_Transmit.Acc_Y 					= ch.Volt2;																	/* Y��ӱ�ֵ					  */
			Date_Transmit.Acc_Z 					= ch.Volt3;																	/* Z��ӱ�ֵ					  */

			Date_Transmit.Acc_Temp				= tempreature;			/* �ӱ��¶ȸ�ֵ					  */


			Date_Transmit.Check_sum 			= calculate_checksum(&Date_Transmit);				/* ���㲢��ֵ Check_sum  	*/
			Date_Transmit.Frame_tail			= 0xBB;


			HAL_UART_Transmit_DMA(&huart5, 
														(uint8_t*)&Date_Transmit, 
														sizeof(Date_Transmit));															/* ͨ��DMA�����ݷ���		 	*/
}

//********************************************Read_Method2*********************************************//


