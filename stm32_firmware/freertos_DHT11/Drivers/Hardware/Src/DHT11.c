#include "DHT11.h"

GPIO_InitTypeDef GPIO_InitStruct;	//�����ı��������״̬

static void GPIO_SETOUT(void);
static void GPIO_SETIN(void);
static uint8_t DHT11_Check(void);

/**********************************************
��������static void DHT11_Rst(void)
����˵������
����ֵ����
�������ã��������Ϳ�ʼ�ź�
***********************************************/
static void DHT11_Rst(void)
{                 
	GPIO_SETOUT();											//���ó����ģʽ
	DHT11_LOW; //����������
	delay_ms(20);    										//��������18ms
	DHT11_HIGH; 	//���������� 
	delay_us(30);     									//��������20~40us	
}


/**********************************************
��������static void GPIO_SETOUT(void)
����˵������
����ֵ����
�������ã�����IO��Ϊ�������ģʽ
***********************************************/
static void GPIO_SETOUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // ���� PULLUP
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
	
}

/**********************************************
��������static void GPIO_SETIN(void)
����˵������
����ֵ����
�������ã�����IO��Ϊ��������ģʽ
***********************************************/
static void GPIO_SETIN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // DHT11 ������ͨ������
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}


/**********************************************
��������static u8 DHT11_Check(void)
����˵������
����ֵ����⵽��Ӧ-->����1������0
�������ã����DHT11����Ӧ�ź�
***********************************************/
static uint8_t DHT11_Check(void) 	   
{   
	uint8_t retry=0;
	GPIO_SETIN();			//����Ϊ����ģʽ	
		
	while (!DHT11_IO_IN && retry<100)//DHT11������80us
	{
		retry++;
		delay_us(1);
	}
	if(retry >= 100)	//��ʱδ��Ӧ/δ�յ���ʼ�źţ��˳����
		return 0;
	else 
		retry = 0;
	while (DHT11_IO_IN && retry<100)//DHT11���ͺ���ٴ�����80us
	{
		retry++;
		delay_us(1);
	}
	if(retry>=100)		//��ʱ��DHT11���������˳����
		return 0;
	
	return 1;					//�豸������Ӧ��������������
}


/**********************************************
��������static u8 DHT11_Read_Bit(void)
����˵������
����ֵ�����ش�DHT11�϶�ȡ��һ��Bit����
�������ã���DHT11�϶�ȡһ��Bit����
***********************************************/
static uint8_t DHT11_Read_Bit(void)
{
	uint8_t retry = 0;
	//DHT11��Bit��ʼ�ź�Ϊ50us�͵�ƽ
	while(DHT11_IO_IN && retry<100)//�ȴ���Ϊ�͵�ƽ(�ȴ�Bit��ʼ�ź�)
	{
		retry++;
		delay_us(1);
	}
	retry = 0;
	while(!DHT11_IO_IN && retry<100)//�ȴ���ߵ�ƽ���������ݿ�ʼ���䣩
	{
		retry++;
		delay_us(1);
	}
	delay_us(30);//�ȴ�30us
	//0�ź�Ϊ26-28us��1�ź���Ϊ70us,����˵����30usȥ��ȡ����״̬�Ϳ���֪�������ֵ��
	if(DHT11_IO_IN) return 1;
	else return 0;		   
}


/***********************************************************************
��������static u8 DHT11_Read_Byte(void)
����˵������
����ֵ�����ش�DHT11�϶�ȡ��һ��byte����
�������ã���DHT11�϶�ȡһ��byte����
************************************************************************/
static uint8_t DHT11_Read_Byte(void)    
{        
	uint8_t i,dat;
	dat=0;	
	for (i=0;i<8;i++) 
	{
		dat<<=1; 
		dat|=DHT11_Read_Bit();
	}	
		
	return dat;
}


/**************************************************************************
��������u8 DHT11_Read_Data(u8 *temp,u8 *humi)
����˵����temp:���ڴ���¶�ֵ(��Χ:0~50��)��humi:���ڴ��ʪ��ֵ(��Χ:20%~90%)
����ֵ��1���ɹ���ȡ���ݣ�0����ȡ���ݳ���
�������ã���DHT11�϶�ȡ��ʪ�����ݣ�����ʡ��С��ֵ��
***************************************************************************/
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi)
{        
	uint8_t buf[5];
	uint8_t i;
	DHT11_Rst();
	if(DHT11_Check()==1)	//�豸��Ӧ����
	{
		for(i=0;i<5;i++)//��ȡ40λ����
		{
			buf[i]=DHT11_Read_Byte();
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])//����У��
		{
			*humi=buf[0];
			*temp=buf[2];
		}
	}
	else return 0;		//�豸δ�ɹ���Ӧ������0
	return 1;					//��ȡ���ݳɹ�����1
}

