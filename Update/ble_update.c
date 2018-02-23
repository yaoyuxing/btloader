#include "ble_update.h" 
#include "stm32f7xx.h"
#include "uart.h"
#include "system_info.h"
#include "FreeRTOS.h"
#include "task.h" 
#include "delay.h"
#include "string.h"
#include "bootloader.h"
#include "ble_tooth.h"
#include "system_info.h"
#include "interflash.h"   
#include "stdlib.h"
#include "ymodem.h" 
#include "crc16.h"


//蓝牙变量
stBleDataType  							gstBleData; 
stBleInfoType 							gstBleInfo;
//任务句柄
TaskHandle_t  BleTask_Handler;
//优先级
#define  BLE_TASK_PRIO   5
//栈大小
#define  BLE_STK_SIZE   4096 

//蓝牙操作函数
static void BleUartInit(unsigned int Baud );
static void BluetoothGPIOInit(void );
void BleReset(void);
static void BleInit(void);
eBleErrorType BleBaudSet(unsigned int Baud);
static void BleTask(void* param);
static void BleAnalysisData(char *pData,unsigned int Len);
static eBleErrorType BleUpdateApp(void);
static void BleGetFireInfo(void);
static void BleAnalysisFireInfo(unsigned char *pData);
static eBleErrorType BleFactorySet(void);

void CreateBleTask(void)
{
	xTaskCreate((TaskFunction_t )BleTask,            //任务函数
				(const char*    )"ble_task",         //任务名称
				(uint16_t       )BLE_STK_SIZE,        //任务堆栈大小
				(void*          )NULL,                //传递给任务函数的参数
				(UBaseType_t    )BLE_TASK_PRIO,       //任务优先级
				(TaskHandle_t*  )&BleTask_Handler);   //任务句柄  	 	
}
static void BleTask(void* param)
{   
	char SaveOKChars[]={'S',0x01};   //保存成功数组 
	char JumpOKChars[]={'J',0x01};;   //跳转成功
	BleInit(); 			//初始化蓝牙
	BlePrintf("ble init ok\r\n"); 
	while(1)       //初始化连接成功后等待手机命令
	{ 
		btIWDG_Refresh();
		if(gBleAPPUpdateStatus==eAPP_JUMP_UPDATE) //app跳转升级
		{ 
			gBleAPPUpdateStatus=eNO_UPDATE;
		 	SaveSysInfo(); 
			gBleAPPUpdateStatus=ePREPARE_UPDATE;
			BleSend2Phone(JumpOKChars,2);//跳转成功
		} 
		#if 1
		if(BL_CONNECTION_STATUS())//在运行过程中断连了
		{  
			gBleConnectStatus=eBLE_UPDATE_STA_NONE;
			//BleInit();         //重新连接
		}  
		else 
		{
			BleDelay_ms(200);
			if(!BL_CONNECTION_STATUS())//延时防抖判断
			gBleConnectStatus=eBLE_CONNECTED;
		}
		#endif  
		if(BLE_RX_FRAME_FLAG==SET)      //接收到蓝牙串口数据帧
		{ 
			BlePrintf("ble got command\r\n");
			BleAnalysisData((char*)gBleRxBuffer,gBleRxBufferLen);
			BLE_RX_FRAME_FLAG=RESET;         //清除接收到帧标志
			switch(gstBleData.ucCmd)
			{
				case BLE_CMD_UPDATE:
					BleAnalysisFireInfo(gstBleData.ucData); //获取固件信息
					BleUpdateApp();    			//升级
					break;
				case BLE_CMD_GET_INFO:
					BleGetFireInfo(); 			//获取信息
					break;
				case BLE_FACTORY_SET:     //出厂设置 
					BleFactorySet();
					gBleAPPUpdateStatus=eGET_FACTORY_DATA;
					break;
				case BLE_SAVE_INFO: 			//校验成功 保存设置
					if(gBleAPPUpdateStatus==eGET_FACTORY_DATA)//设置过出厂信息
					{
						gBleAPPUpdateStatus=eNO_UPDATE;				//存初始状态
						SaveSysInfo();												//保存数据
						FactoryOK();
						gBleAPPUpdateStatus=eFACTORY_SET_OK; //出厂设置完成
						BleSend2Phone(SaveOKChars,2);        //保存OK应答到手机蓝牙
					}
					break;
				case BLE_JUMP:
					gBleAPPUpdateStatus=eAPP_JUMP_UPDATE;
					break;
			}
		}
	}  
} 
/**
  * @brief  蓝牙初始化
  */
static void BleInit(void)
{
	#if  NEW_FIRE_VER
	#else
	unsigned int TimeOut=2000;  
	unsigned char ucTimes=BLE_CMD_TRY_TIMES;
	int res=0;
  
//	if((gBleBaud!=BLE_DEFAULT_BAUD)&&(gBleBaud!=BLE_PENETRATE_MODE_BAUD))//读出来的波特率不正确
//	{
//		gBleBaud=BLE_PENETRATE_MODE_BAUD;//设置为蓝牙默认波特率
//	}	
	//BleUartInit(gBleBaud);                    //串口波特率设置为上次存入的波特率 
	#endif
	BleUartInit(19200);                    //串口波特率设置为上次存入的波特率 
	BluetoothGPIOInit();                  //初始化ble IO
	gBleCurrentMode=BLE_CMD_MODE_NONE;
	gBleConnectStatus=eBLE_UPDATE_STA_NONE;
	if(BL_CONNECTION_STATUS()!=0) //未连接
	{	
		#if  NEW_FIRE_VER
		#else
		BleReset();        //重启  
		BL_MRDY_PIN_RESET();  //切换到RX接收模式
		BleToothSendCmd(BL_MAC,BLE_CMD_READ,"");
		BleToothSendCmd(BL_ROLE,BLE_CMD_SET,"0");
    res=BleBaudSet(BLE_PENETRATE_MODE_BAUD);    //设置为透传的波特率 
		if(res!=0) 
		{
			BlePrintf("ble baud set failed res =%d\r\n",res);
		} 
		//res=BleToothSendCmd(BL_NAME,BLE_CMD_READ,"yyx");
		if(res!=0) 
		{
			while(1);
		} 
		//res=BleToothSendCmd(BL_TXPWR,BLE_CMD_SET,"-3");    //设置发射强度
		if(res!=0)        
		{
			BlePrintf("ble txpwr failed res =%d\r\n",res);
		}
		#endif
		while(1)//TimeOut--)          //连接
		{
			btIWDG_Refresh();
			if(!BL_CONNECTION_STATUS()) //有设备连接上
			{
				BleDelay_ms(200);
				if(!BL_CONNECTION_STATUS())//延时防抖判断
				{
					#if  NEW_FIRE_VER                  //新固件直接设置标志
					gBleConnectStatus=eBLE_CONNECTED;
					break;
					#else
					while(ucTimes--)
					{
						res=BleToothSendCmd(BL_MODE,BLE_CMD_SET,NULL);       //发射透传命令
						if((res==BLE_NO_ERR)&&(gBleCurrentMode==BLE_PENETRATE_TRANS_MODE))//进入透传模式
						{
							gBleConnectStatus=SET;           //设置为透传
							BlePrintf("ble connected\r\n");
							break;
						}
						else
						{
							BlePrintf("ble connected but set mode err=%d\r\n",res);
						}
					}
					if(gBleCurrentMode==BLE_PENETRATE_TRANS_MODE)
					{
						gBleConnectStatus=eBLE_CONNECTED;
						BlePrintf("ble connected\r\n");
						break;
					}
					#endif
				} 
			}
			//BleDelay_ms(500); 
			//BlePrintf("ble waiting for connecting\r\n");
		} 
		BlePrintf("ble no connecting\r\n");  
	}
	else
	{
		gBleConnectStatus=eBLE_CONNECTED;               //已连接直接设置标志
		BlePrintf("ble already connected\r\n");
	}
}
/**
  * @brief  蓝牙串口初始化 
  * @param  Baud: 设置串口波特率  
  */
static void BleUartInit(unsigned int Baud )
{  
	gBleUartHandle.Instance= UART5;
	gBleUartHandle.Init.BaudRate=Baud;
	gBleUartHandle.Init.WordLength=UART_WORDLENGTH_8B;   //字长为8位数据格式
	gBleUartHandle.Init.StopBits=UART_STOPBITS_1;	    //一个停止位
	gBleUartHandle.Init.Parity=UART_PARITY_NONE;		    //无奇偶校验位 
	gBleUartHandle.Init.Mode=UART_MODE_TX_RX;		    //
	HAL_UART_Init( (UART_HandleTypeDef*)&gBleUartHandle); 
	HAL_UART_Receive_IT(&gstUart5.UartHandle,(uint8_t *)&gstUart5.ucRecByte,1); 
	 gstUart5.eRxMode=UART_RX_FRAME_MODE;
	//BleCmdNormalRxMode_Exit();
	//BleRxByteITSet(gBleRxByte);
}
/**
  * @brief  蓝牙串口IO初始化   
  */
static void BluetoothGPIOInit(void )
{
	#if NEW_FIRE_VER
	GPIO_InitTypeDef GPIO_Initure;
	BL_CONNECTION_RCC()	;
	GPIO_Initure.Pin=BL_CONNECTION_PIN; 		//
	GPIO_Initure.Mode=GPIO_MODE_INPUT;  		//输入模式
	HAL_GPIO_Init(BL_CONNECTION_GPIO,&GPIO_Initure);
	#else
	GPIO_InitTypeDef GPIO_Initure;
	BL_MRDY_RCC();           		//开启GPIOB时钟
	BL_CONNECTION_RCC()	;
	//DIO_6 输入/输出 MRDY 引脚，用于触发模块的串口接收功能（低电平接收）
	GPIO_Initure.Pin=BL_MRDY_PIN; 					//
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
	
	HAL_GPIO_Init(BL_MRDY_GPIO,&GPIO_Initure);
	HAL_GPIO_WritePin(BL_MRDY_GPIO,BL_MRDY_PIN,GPIO_PIN_RESET);	
	//DIO_8 输入/输出 连接指示，当模块建立连接后输出低电平
	GPIO_Initure.Pin=BL_CONNECTION_PIN; 		//
	GPIO_Initure.Mode=GPIO_MODE_INPUT;  		//输入模式
	HAL_GPIO_Init(BL_CONNECTION_GPIO,&GPIO_Initure);
	//nRESET 输入 复位脚，内部有上拉（详见 CC2650 手册）
	BL_RESET_RCC();
	GPIO_Initure.Pin=BL_RESET_PIN; 					//
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //推挽输出
	GPIO_Initure.Pull=GPIO_PULLUP;          //上拉
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //高速
	
	HAL_GPIO_Init(BL_RESET_GPIO,&GPIO_Initure);
	HAL_GPIO_WritePin(BL_RESET_GPIO,BL_RESET_PIN,GPIO_PIN_SET); 
	#endif
}
/**
  * @brief  蓝牙硬件复位
  */
void BleReset(void)
{
	#if NEW_FIRE_VER
	#else
	BL_RESET_PIN_RESET();
	BleDelay_ms(20);
	BL_RESET_PIN_SET();
	#endif
}


/**
  * @brief  设置蓝牙波特率
  * @note   查询是否为设定波特率，不是则设置是则返回成功
  * @param  Baud: 用户实际地址
  * @retval 错误代码 eBleErrorType
  */
#define BAUD_STR_MAX       10
eBleErrorType BleBaudSet(unsigned int Baud)
{ 
	eBleErrorType res; 
	char temp[BAUD_STR_MAX]={0};  
	unsigned char ucTimes=BLE_CMD_TRY_TIMES;
	if((gBleBaud!=BLE_DEFAULT_BAUD)&&(gBleBaud!=BLE_PENETRATE_MODE_BAUD))//读出来的波特率不正确
	{
		gBleBaud=BLE_DEFAULT_BAUD;//设置为蓝牙默认波特率
	}	
	BleUartInit(gBleBaud);                    //串口波特率设置为上次存入的波特率
	
	if(gBleBaud==BLE_PENETRATE_MODE_BAUD)      //已经设置成透传波特率
	{
		while(ucTimes--)                          //退出透传进入命令模式
		{
			res=BleToothSendCmd(BL_MODE,BLE_CMD_SET,NULL); 
			if(res==BLE_NO_ERR)
			{
				if(gBleCurrentMode==BLE_PENETRATE_TRANS_MODE)       //设置成功
					break;
			}
		}
	}
	else
	{
		while(ucTimes--)                          //退出透传进入命令模式
		{
			res=BleToothSendCmd(BL_MODE,BLE_CMD_SET,NULL); 
			if(res==BLE_NO_ERR)
			{
				if(gBleCurrentMode==BLE_CMD_MODE)       //设置成功
					break;
			}
		}
		if(ucTimes==0) // 退出分析
		{
			if(res!=BLE_NO_ERR) 
			{
				BlePrintf("ble set baud :baud err\r\n");
				return BLE_BAUD_ERR;
			}
			else if(gBleCurrentMode!=BLE_CMD_MODE)
			{
				BlePrintf("ble set baud :try err\r\n");
				return BLE_TRY_ERR;//未设置为命令模式 返回尝试设置失败
			}
		}   
		res=BleToothSendCmd(BL_UART,BLE_CMD_READ,NULL);
		if(res==BLE_NO_ERR)
		{ 
			if(gBleBaud==Baud)
			{
				return BLE_NO_ERR;
			}
			else 
			{
				sprintf(temp,"%d",Baud);
				res=BleToothSendCmd(BL_UART,BLE_CMD_SET,temp);
				if(res==BLE_NO_ERR) 
				{
					BlePrintf("ble set baud ok\r\n");
					BleUartInit(Baud);  //当前波特率
				}
				res=BleToothSendCmd(BL_UART,BLE_CMD_READ,NULL); 
				if(gBleBaud==Baud)
				{
					SaveSysInfo();
					return BLE_NO_ERR;
				}
				else 
				{
					BlePrintf("ble set baud :err=%d\r\n",BLE_BAUD_ERR);
					return BLE_BAUD_ERR;
				}
			} 
		}
		else 
		{ 
			BlePrintf("ble set baud :err=%d\r\n",res);
			return res;
		}	
	}	
	return	BLE_NO_ERR;
} 
/**
  * @brief  蓝牙解析固件信息函数 "	U{\"Version\":{\"ModuleName\":\"%s\",\"HardVersion\":\"%s\",\"FireVersion\":\"%s\",\"DataTime\":\"%s\"}}" 
  * @note   根据协议固定偏移取数据
  * @param  pData: 数据操作指针    
  */
static void BleAnalysisFireInfo(unsigned char *pData)
{
	unsigned char cnt=0;
	memset((void*)&gstUpdate.stFireInfo,0,sizeof (gstUpdate.stFireInfo)-sizeof(gstUpdate.stFireInfo.SN));
	pData+=MODULE_NAME_POS;
	for(cnt=0;pData[0]!=END_BYTE_2;cnt++)
	{
		gstUpdate.stFireInfo.ModuleName[cnt]=*pData++;
	}
	pData+=HARD_VERSION_POS;
	for(cnt=0;pData[0]!=END_BYTE_2;cnt++)
	{
		gstUpdate.stFireInfo.HardVersion[cnt]=*pData++;
	}
	pData+=FIRE_VERSION_POS;
	for(cnt=0;pData[0]!=END_BYTE_2;cnt++)
	{
		gstUpdate.stFireInfo.FireVersion[cnt]=*pData++;
	}
	pData+=DATA_TIME_POS;
	for(cnt=0;pData[0]!=END_BYTE_2;cnt++)
	{
		gstUpdate.stFireInfo.Time[cnt]=*pData++;
	}
}
#define TEMP_BUF_MAX   1024
/**
  * @brief  蓝牙获取固件信息函数
  * @note   向手机端发送固件信息  
  */
static void BleGetFireInfo(void)
{ 
	char   pdata[TEMP_BUF_MAX]={0}; 
	memset(pdata,0,TEMP_BUF_MAX);  
	sprintf(pdata,"G{\"Version\":{\"ModuleName\":\"%s\",\"HardVersion\":\"%s\",\"FireVersion\":\"%s\",\"DataTime\":\"%s\"}}"\
	,gstUpdate.stFireInfo.ModuleName\
	,gstUpdate.stFireInfo.HardVersion\
	,gstUpdate.stFireInfo.FireVersion\
	,gstUpdate.stFireInfo.Time);  
	BleSend2Phone(pdata,strlen(pdata)); 
	gBleAPPUpdateStatus=ePREPARE_UPDATE;      // 
} 
/**
  * @brief  蓝牙出厂设置
  * @note   设置sn号等
  * @retval 错误代码 eBleErrorType
  */
static eBleErrorType BleFactorySet(void)    
{
  char sntemp[15]={0};
	SetSerialNumber(gstBleData.ucData,gstBleData.usDataLen);		 
	sprintf(sntemp,"F%s",gpBleSerialNum);
	BleSend2Phone(sntemp,strlen(sntemp));//发送给手机进行校验SN
	return BLE_NO_ERR;
}
/**
  * @brief  蓝牙升级函数
  * @note   获取固件信息并存储，取bin文件解析存储跳转 
  * @retval 错误代码 eBleErrorType
  */
static eBleErrorType BleUpdateApp(void)            
{
  char ucStartChar[]={'U',0x02};
	unsigned 	char *pData=NULL;            //bin文件操作指针
	unsigned int unBinDatalen=0; //bin文件大小 
//	if((gBleUpdateMode!=eUPDATE_BLE)&&gBleUpdateMode) //如果有其他升级
//		 return BLE_UPDATE_BSY; 							//返回升级忙错误 
	gBleAPPUpdateStatus=eSTART_UPDATE;  
 	BLE_RX_BYTE_FLAG=RESET; 
	BleSend2Phone(ucStartChar,2); //发送开始ymodem应答
	BleDelay_ms(200);//延时发送
	BleCmdNormalRxMode_Enter();   //蓝牙串口进入normal接收状态 
	BLE_RX_BYTE_FLAG=RESET;
	gstRxCtrl.ucUseEscape=0;
	//while(1)
	{
		
//		gBleRxBufferLen=0;
//		memset(gBleRxBuffer,0,sizeof (gBleRxBuffer));
//		BleSendByte("C"); 
//		while(1);
//				gBleRxBufferLen=0;
//		memset(gBleRxBuffer,0,sizeof (gBleRxBuffer));
//		BleSendByte("C"); 
//				gBleRxBufferLen=0;
//		memset(gBleRxBuffer,0,sizeof (gBleRxBuffer));
//		BleSendByte("C"); 
//				gBleRxBufferLen=0;
//		memset(gBleRxBuffer,0,sizeof (gBleRxBuffer));
//		BleSendByte("C"); 
	}
//	taskENTER_CRITICAL();
	do
	{
		//__set_FAULTMASK(1);
		gstUpdate.unCurrentAppSize= Ymodem_Receive(gucAppBuf);      //ymodem接收 bin文件
		//BleDelay_ms(1000);
	}while(gstUpdate.unCurrentAppSize==0);
//	taskEXIT_CRITICAL();
	BleCmdNormalRxMode_Exit(); //蓝牙串口退出normal接收状态
	gstUpdate.unCurrentAppADDR=(gstUpdate.unCurrentAppADDR==USER1_APP_ADDR)?USER2_APP_ADDR:USER1_APP_ADDR; //取与上次升级位置不同的位置进行升级
	pData=pGetAppBin(gucAppBuf,gstUpdate.unCurrentAppSize,gstUpdate.unCurrentAppADDR); //取bin文件位置指针
	if(pData==NULL)
	{		
		BlePrintf("ble update addr err");
		while(1);
	}
	unBinDatalen=(*(unsigned int *)pData); 		//取bin文件长度
	gstUpdate.unCurrentAppSize=unBinDatalen;
	pData+=4;									//偏移到bin文件开始位置
	if(CheckAppBin((unsigned int *)pData)!=0)	//检测APPbin文件
	{
		BlePrintf("ble update AppBin err");
		while(1);
	}
	//SaveAppBackup(pData,unBinDatalen);             //APP备份 
	
	AppWrite2Flash(pData);
	btIWDG_Refresh();
	//InterFlash_Program(gstUpdate.unCurrentAppADDR,pData,gstUpdate.unCurrentAppSize, FLASH_TYPEPROGRAM_BYTE);//写入APP
	//BleDelay_ms(1000);
	gBleAPPUpdateStatus=eWRITE_APP_OK;
	delay_ms(1000);
	if(gstUpdate.unCurrentAppADDR==USER1_APP_ADDR)      //判断位置将相应状态记录
	{
		gBleAPPUpdateStatus=eUSR1_UPDATE_CHECK;
	}
	else if(gstUpdate.unCurrentAppADDR==USER2_APP_ADDR)
	{
		gBleAPPUpdateStatus=eUSR2_UPDATE_CHECK;
	}
	else 
	{
		BlePrintf("ble update current adr err");
		while(1);
	}
	SaveSysInfo();//存储状态信息
//	RefreshSysInfo(); 
	
	btResetSysForJump();           //跳转APP
	return BLE_NO_ERR;
}
/**
  * @brief  蓝牙解析接收串口数据函数
  * @note   接收到的串口数据取出命令和数据和数据长度
  * @param  pData: 数据操作指针 
  * @param  Len: 数据长度
  * @retval 错误代码 eBleErrorType
  */
static void BleAnalysisData(char *pData,unsigned int Len)
{
	unsigned short int crctemp=0;
	gstBleData.ucCmd=pData[3];                          //接收命令
	gstBleData.usDataLen=(pData[1]<<8)+pData[2]-1;      //数据长度
	gstBleData.usCRC16=(pData[Len-3]<<8)+pData[Len-2];
	crctemp=usCrc16((unsigned char *)&pData[3],gstBleData.usDataLen+1);
	if(crctemp!=gstBleData.usCRC16)
		 crctemp=crctemp;//校验失败
	if(gstBleData.usDataLen<BLE_DATA_BUF_MAX)               //数据小于buf大小
	{
		memset(gstBleData.ucData,0,sizeof(gstBleData.ucData));
		memcpy(gstBleData.ucData,(void*)&pData[4],gstBleData.usDataLen);//拷贝数据到buf
	}
} 


int  str_esc(char *pReturn,char*pSource,char target1,char target2,unsigned int len )
{
	unsigned int nreturn=0;
	while(len--)
	{
		if(*pSource==target1)
		{
			*pReturn++=ESCAPE_CHAR;
			*pReturn++=(*pSource)^XOR_CHAR;
			nreturn++;
			pSource++;
		}
		else if(*pSource==target2)
		{
			*pReturn++=ESCAPE_CHAR;
			*pReturn++=(*pSource)^XOR_CHAR;
			nreturn++;
			pSource++;
		}
		else if(*pSource==ESCAPE_CHAR)
		{
			*pReturn++=ESCAPE_CHAR;
			*pReturn++=(*pSource)^XOR_CHAR;
			nreturn++;
			pSource++;
		}
		else 
		{
			*pReturn++=*pSource++;
		}
		nreturn++;
	}
	return nreturn;
}

#define SEND_2PHONE_BUF_MAX   1024
/**
  * @brief  发送数据到手机蓝牙
  * @note   数据进行转义校验加帧头尾进行发送，校验转义前数据
  * @param  pData: 数据指针
  * @param  len: 数据长度
  */
void BleSend2Phone(char *pData,unsigned int len )
{ 
	char cDataBuf[SEND_2PHONE_BUF_MAX]={0};
	char cDataBufTemp[SEND_2PHONE_BUF_MAX]={0};
	char cHead=BLE_FRAME_HEAD_STR,cTail=BLE_FRAME_TAIL_STR;
	unsigned short  crc16=0,sendlen; 
	crc16=bt_crc16((uint8_t*)&pData[1],len);   //去掉命令头计算校验
	
	//cTemp[0]=BLE_FRAME_HEAD_STR ;//取头 
	cDataBufTemp[1]=len&0x00ff;          //取低字节
	cDataBufTemp[0]=len>>8;          //取高字节  
	
	memcpy((char *)&cDataBufTemp[2],pData,len);                 //拼接数据 
	
	cDataBufTemp[2+len+1]=crc16&0x00ff;          //取低字节
	cDataBufTemp[2+len]=crc16>>8;          //取高字节  
	
	sendlen=str_esc(cDataBuf,cDataBufTemp,BLE_FRAME_HEAD_STR,BLE_FRAME_TAIL_STR,len+4); //对数据中和帧尾字符相同的进行转义
   
	Uart5SendBytes(&cHead,1); //发送头
	Uart5SendBytes(( char *)cDataBuf,sendlen);//发送 
	Uart5SendBytes(&cTail,1);//发送尾  
}




