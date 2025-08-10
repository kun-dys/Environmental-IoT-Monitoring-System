#include "DHT11.h"

GPIO_InitTypeDef GPIO_InitStruct;	//后面会改变输入输出状态

static void GPIO_SETOUT(void);
static void GPIO_SETIN(void);
static uint8_t DHT11_Check(void);

/**********************************************
函数名：static void DHT11_Rst(void)
参数说明：无
返回值：无
函数作用：主机发送开始信号
***********************************************/
static void DHT11_Rst(void)
{                 
	GPIO_SETOUT();											//配置成输出模式
	DHT11_LOW; //拉低数据线
	delay_ms(20);    										//拉低至少18ms
	DHT11_HIGH; 	//拉高数据线 
	delay_us(30);     									//主机拉高20~40us	
}


/**********************************************
函数名：static void GPIO_SETOUT(void)
参数说明：无
返回值：无
函数作用：配置IO口为推挽输出模式
***********************************************/
static void GPIO_SETOUT(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // 或者 PULLUP
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
	
}

/**********************************************
函数名：static void GPIO_SETIN(void)
参数说明：无
返回值：无
函数作用：配置IO口为浮空输入模式
***********************************************/
static void GPIO_SETIN(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT11_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;  // DHT11 数据线通常浮空
    HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);
}


/**********************************************
函数名：static u8 DHT11_Check(void)
参数说明：无
返回值：检测到回应-->返回1，否则0
函数作用：检测DHT11的响应信号
***********************************************/
static uint8_t DHT11_Check(void) 	   
{   
	uint8_t retry=0;
	GPIO_SETIN();			//设置为输入模式	
		
	while (!DHT11_IO_IN && retry<100)//DHT11会拉低80us
	{
		retry++;
		delay_us(1);
	}
	if(retry >= 100)	//超时未响应/未收到开始信号，退出检测
		return 0;
	else 
		retry = 0;
	while (DHT11_IO_IN && retry<100)//DHT11拉低后会再次拉高80us
	{
		retry++;
		delay_us(1);
	}
	if(retry>=100)		//超时，DHT11工作出错，退出检测
		return 0;
	
	return 1;					//设备正常响应，可以正常工作
}


/**********************************************
函数名：static u8 DHT11_Read_Bit(void)
参数说明：无
返回值：返回从DHT11上读取的一个Bit数据
函数作用：从DHT11上读取一个Bit数据
***********************************************/
static uint8_t DHT11_Read_Bit(void)
{
	uint8_t retry = 0;
	//DHT11的Bit开始信号为50us低电平
	while(DHT11_IO_IN && retry<100)//等待变为低电平(等待Bit开始信号)
	{
		retry++;
		delay_us(1);
	}
	retry = 0;
	while(!DHT11_IO_IN && retry<100)//等待变高电平（代表数据开始传输）
	{
		retry++;
		delay_us(1);
	}
	delay_us(30);//等待30us
	//0信号为26-28us，1信号则为70us,所以说超过30us去读取引脚状态就可以知道传输的值了
	if(DHT11_IO_IN) return 1;
	else return 0;		   
}


/***********************************************************************
函数名：static u8 DHT11_Read_Byte(void)
参数说明：无
返回值：返回从DHT11上读取的一个byte数据
函数作用：从DHT11上读取一个byte数据
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
函数名：u8 DHT11_Read_Data(u8 *temp,u8 *humi)
参数说明：temp:用于存放温度值(范围:0~50°)，humi:用于存放湿度值(范围:20%~90%)
返回值：1：成功读取数据，0：读取数据出错
函数作用：从DHT11上读取温湿度数据（这里省略小数值）
***************************************************************************/
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi)
{        
	uint8_t buf[5];
	uint8_t i;
	DHT11_Rst();
	if(DHT11_Check()==1)	//设备响应正常
	{
		for(i=0;i<5;i++)//读取40位数据
		{
			buf[i]=DHT11_Read_Byte();
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])//进行校验
		{
			*humi=buf[0];
			*temp=buf[2];
		}
	}
	else return 0;		//设备未成功响应，返回0
	return 1;					//读取数据成功返回1
}

