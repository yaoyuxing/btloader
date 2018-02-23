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


//��������
stBleDataType  							gstBleData; 
stBleInfoType 							gstBleInfo;
//������
TaskHandle_t  BleTask_Handler;
//���ȼ�
#define  BLE_TASK_PRIO   5
//ջ��С
#define  BLE_STK_SIZE   4096 

//������������
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
	xTaskCreate((TaskFunction_t )BleTask,            //������
				(const char*    )"ble_task",         //��������
				(uint16_t       )BLE_STK_SIZE,        //�����ջ��С
				(void*          )NULL,                //���ݸ��������Ĳ���
				(UBaseType_t    )BLE_TASK_PRIO,       //�������ȼ�
				(TaskHandle_t*  )&BleTask_Handler);   //������  	 	
}
static void BleTask(void* param)
{   
	char SaveOKChars[]={'S',0x01};   //����ɹ����� 
	char JumpOKChars[]={'J',0x01};;   //��ת�ɹ�
	BleInit(); 			//��ʼ������
	BlePrintf("ble init ok\r\n"); 
	while(1)       //��ʼ�����ӳɹ���ȴ��ֻ�����
	{ 
		btIWDG_Refresh();
		if(gBleAPPUpdateStatus==eAPP_JUMP_UPDATE) //app��ת����
		{ 
			gBleAPPUpdateStatus=eNO_UPDATE;
		 	SaveSysInfo(); 
			gBleAPPUpdateStatus=ePREPARE_UPDATE;
			BleSend2Phone(JumpOKChars,2);//��ת�ɹ�
		} 
		#if 1
		if(BL_CONNECTION_STATUS())//�����й����ж�����
		{  
			gBleConnectStatus=eBLE_UPDATE_STA_NONE;
			//BleInit();         //��������
		}  
		else 
		{
			BleDelay_ms(200);
			if(!BL_CONNECTION_STATUS())//��ʱ�����ж�
			gBleConnectStatus=eBLE_CONNECTED;
		}
		#endif  
		if(BLE_RX_FRAME_FLAG==SET)      //���յ�������������֡
		{ 
			BlePrintf("ble got command\r\n");
			BleAnalysisData((char*)gBleRxBuffer,gBleRxBufferLen);
			BLE_RX_FRAME_FLAG=RESET;         //������յ�֡��־
			switch(gstBleData.ucCmd)
			{
				case BLE_CMD_UPDATE:
					BleAnalysisFireInfo(gstBleData.ucData); //��ȡ�̼���Ϣ
					BleUpdateApp();    			//����
					break;
				case BLE_CMD_GET_INFO:
					BleGetFireInfo(); 			//��ȡ��Ϣ
					break;
				case BLE_FACTORY_SET:     //�������� 
					BleFactorySet();
					gBleAPPUpdateStatus=eGET_FACTORY_DATA;
					break;
				case BLE_SAVE_INFO: 			//У��ɹ� ��������
					if(gBleAPPUpdateStatus==eGET_FACTORY_DATA)//���ù�������Ϣ
					{
						gBleAPPUpdateStatus=eNO_UPDATE;				//���ʼ״̬
						SaveSysInfo();												//��������
						FactoryOK();
						gBleAPPUpdateStatus=eFACTORY_SET_OK; //�����������
						BleSend2Phone(SaveOKChars,2);        //����OKӦ���ֻ�����
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
  * @brief  ������ʼ��
  */
static void BleInit(void)
{
	#if  NEW_FIRE_VER
	#else
	unsigned int TimeOut=2000;  
	unsigned char ucTimes=BLE_CMD_TRY_TIMES;
	int res=0;
  
//	if((gBleBaud!=BLE_DEFAULT_BAUD)&&(gBleBaud!=BLE_PENETRATE_MODE_BAUD))//�������Ĳ����ʲ���ȷ
//	{
//		gBleBaud=BLE_PENETRATE_MODE_BAUD;//����Ϊ����Ĭ�ϲ�����
//	}	
	//BleUartInit(gBleBaud);                    //���ڲ���������Ϊ�ϴδ���Ĳ����� 
	#endif
	BleUartInit(19200);                    //���ڲ���������Ϊ�ϴδ���Ĳ����� 
	BluetoothGPIOInit();                  //��ʼ��ble IO
	gBleCurrentMode=BLE_CMD_MODE_NONE;
	gBleConnectStatus=eBLE_UPDATE_STA_NONE;
	if(BL_CONNECTION_STATUS()!=0) //δ����
	{	
		#if  NEW_FIRE_VER
		#else
		BleReset();        //����  
		BL_MRDY_PIN_RESET();  //�л���RX����ģʽ
		BleToothSendCmd(BL_MAC,BLE_CMD_READ,"");
		BleToothSendCmd(BL_ROLE,BLE_CMD_SET,"0");
    res=BleBaudSet(BLE_PENETRATE_MODE_BAUD);    //����Ϊ͸���Ĳ����� 
		if(res!=0) 
		{
			BlePrintf("ble baud set failed res =%d\r\n",res);
		} 
		//res=BleToothSendCmd(BL_NAME,BLE_CMD_READ,"yyx");
		if(res!=0) 
		{
			while(1);
		} 
		//res=BleToothSendCmd(BL_TXPWR,BLE_CMD_SET,"-3");    //���÷���ǿ��
		if(res!=0)        
		{
			BlePrintf("ble txpwr failed res =%d\r\n",res);
		}
		#endif
		while(1)//TimeOut--)          //����
		{
			btIWDG_Refresh();
			if(!BL_CONNECTION_STATUS()) //���豸������
			{
				BleDelay_ms(200);
				if(!BL_CONNECTION_STATUS())//��ʱ�����ж�
				{
					#if  NEW_FIRE_VER                  //�¹̼�ֱ�����ñ�־
					gBleConnectStatus=eBLE_CONNECTED;
					break;
					#else
					while(ucTimes--)
					{
						res=BleToothSendCmd(BL_MODE,BLE_CMD_SET,NULL);       //����͸������
						if((res==BLE_NO_ERR)&&(gBleCurrentMode==BLE_PENETRATE_TRANS_MODE))//����͸��ģʽ
						{
							gBleConnectStatus=SET;           //����Ϊ͸��
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
		gBleConnectStatus=eBLE_CONNECTED;               //������ֱ�����ñ�־
		BlePrintf("ble already connected\r\n");
	}
}
/**
  * @brief  �������ڳ�ʼ�� 
  * @param  Baud: ���ô��ڲ�����  
  */
static void BleUartInit(unsigned int Baud )
{  
	gBleUartHandle.Instance= UART5;
	gBleUartHandle.Init.BaudRate=Baud;
	gBleUartHandle.Init.WordLength=UART_WORDLENGTH_8B;   //�ֳ�Ϊ8λ���ݸ�ʽ
	gBleUartHandle.Init.StopBits=UART_STOPBITS_1;	    //һ��ֹͣλ
	gBleUartHandle.Init.Parity=UART_PARITY_NONE;		    //����żУ��λ 
	gBleUartHandle.Init.Mode=UART_MODE_TX_RX;		    //
	HAL_UART_Init( (UART_HandleTypeDef*)&gBleUartHandle); 
	HAL_UART_Receive_IT(&gstUart5.UartHandle,(uint8_t *)&gstUart5.ucRecByte,1); 
	 gstUart5.eRxMode=UART_RX_FRAME_MODE;
	//BleCmdNormalRxMode_Exit();
	//BleRxByteITSet(gBleRxByte);
}
/**
  * @brief  ��������IO��ʼ��   
  */
static void BluetoothGPIOInit(void )
{
	#if NEW_FIRE_VER
	GPIO_InitTypeDef GPIO_Initure;
	BL_CONNECTION_RCC()	;
	GPIO_Initure.Pin=BL_CONNECTION_PIN; 		//
	GPIO_Initure.Mode=GPIO_MODE_INPUT;  		//����ģʽ
	HAL_GPIO_Init(BL_CONNECTION_GPIO,&GPIO_Initure);
	#else
	GPIO_InitTypeDef GPIO_Initure;
	BL_MRDY_RCC();           		//����GPIOBʱ��
	BL_CONNECTION_RCC()	;
	//DIO_6 ����/��� MRDY ���ţ����ڴ���ģ��Ĵ��ڽ��չ��ܣ��͵�ƽ���գ�
	GPIO_Initure.Pin=BL_MRDY_PIN; 					//
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //�������
	GPIO_Initure.Pull=GPIO_PULLUP;          //����
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //����
	
	HAL_GPIO_Init(BL_MRDY_GPIO,&GPIO_Initure);
	HAL_GPIO_WritePin(BL_MRDY_GPIO,BL_MRDY_PIN,GPIO_PIN_RESET);	
	//DIO_8 ����/��� ����ָʾ����ģ�齨�����Ӻ�����͵�ƽ
	GPIO_Initure.Pin=BL_CONNECTION_PIN; 		//
	GPIO_Initure.Mode=GPIO_MODE_INPUT;  		//����ģʽ
	HAL_GPIO_Init(BL_CONNECTION_GPIO,&GPIO_Initure);
	//nRESET ���� ��λ�ţ��ڲ������������ CC2650 �ֲᣩ
	BL_RESET_RCC();
	GPIO_Initure.Pin=BL_RESET_PIN; 					//
	GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;  //�������
	GPIO_Initure.Pull=GPIO_PULLUP;          //����
	GPIO_Initure.Speed=GPIO_SPEED_HIGH;     //����
	
	HAL_GPIO_Init(BL_RESET_GPIO,&GPIO_Initure);
	HAL_GPIO_WritePin(BL_RESET_GPIO,BL_RESET_PIN,GPIO_PIN_SET); 
	#endif
}
/**
  * @brief  ����Ӳ����λ
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
  * @brief  ��������������
  * @note   ��ѯ�Ƿ�Ϊ�趨�����ʣ��������������򷵻سɹ�
  * @param  Baud: �û�ʵ�ʵ�ַ
  * @retval ������� eBleErrorType
  */
#define BAUD_STR_MAX       10
eBleErrorType BleBaudSet(unsigned int Baud)
{ 
	eBleErrorType res; 
	char temp[BAUD_STR_MAX]={0};  
	unsigned char ucTimes=BLE_CMD_TRY_TIMES;
	if((gBleBaud!=BLE_DEFAULT_BAUD)&&(gBleBaud!=BLE_PENETRATE_MODE_BAUD))//�������Ĳ����ʲ���ȷ
	{
		gBleBaud=BLE_DEFAULT_BAUD;//����Ϊ����Ĭ�ϲ�����
	}	
	BleUartInit(gBleBaud);                    //���ڲ���������Ϊ�ϴδ���Ĳ�����
	
	if(gBleBaud==BLE_PENETRATE_MODE_BAUD)      //�Ѿ����ó�͸��������
	{
		while(ucTimes--)                          //�˳�͸����������ģʽ
		{
			res=BleToothSendCmd(BL_MODE,BLE_CMD_SET,NULL); 
			if(res==BLE_NO_ERR)
			{
				if(gBleCurrentMode==BLE_PENETRATE_TRANS_MODE)       //���óɹ�
					break;
			}
		}
	}
	else
	{
		while(ucTimes--)                          //�˳�͸����������ģʽ
		{
			res=BleToothSendCmd(BL_MODE,BLE_CMD_SET,NULL); 
			if(res==BLE_NO_ERR)
			{
				if(gBleCurrentMode==BLE_CMD_MODE)       //���óɹ�
					break;
			}
		}
		if(ucTimes==0) // �˳�����
		{
			if(res!=BLE_NO_ERR) 
			{
				BlePrintf("ble set baud :baud err\r\n");
				return BLE_BAUD_ERR;
			}
			else if(gBleCurrentMode!=BLE_CMD_MODE)
			{
				BlePrintf("ble set baud :try err\r\n");
				return BLE_TRY_ERR;//δ����Ϊ����ģʽ ���س�������ʧ��
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
					BleUartInit(Baud);  //��ǰ������
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
  * @brief  ���������̼���Ϣ���� "	U{\"Version\":{\"ModuleName\":\"%s\",\"HardVersion\":\"%s\",\"FireVersion\":\"%s\",\"DataTime\":\"%s\"}}" 
  * @note   ����Э��̶�ƫ��ȡ����
  * @param  pData: ���ݲ���ָ��    
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
  * @brief  ������ȡ�̼���Ϣ����
  * @note   ���ֻ��˷��͹̼���Ϣ  
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
  * @brief  ������������
  * @note   ����sn�ŵ�
  * @retval ������� eBleErrorType
  */
static eBleErrorType BleFactorySet(void)    
{
  char sntemp[15]={0};
	SetSerialNumber(gstBleData.ucData,gstBleData.usDataLen);		 
	sprintf(sntemp,"F%s",gpBleSerialNum);
	BleSend2Phone(sntemp,strlen(sntemp));//���͸��ֻ�����У��SN
	return BLE_NO_ERR;
}
/**
  * @brief  ������������
  * @note   ��ȡ�̼���Ϣ���洢��ȡbin�ļ������洢��ת 
  * @retval ������� eBleErrorType
  */
static eBleErrorType BleUpdateApp(void)            
{
  char ucStartChar[]={'U',0x02};
	unsigned 	char *pData=NULL;            //bin�ļ�����ָ��
	unsigned int unBinDatalen=0; //bin�ļ���С 
//	if((gBleUpdateMode!=eUPDATE_BLE)&&gBleUpdateMode) //�������������
//		 return BLE_UPDATE_BSY; 							//��������æ���� 
	gBleAPPUpdateStatus=eSTART_UPDATE;  
 	BLE_RX_BYTE_FLAG=RESET; 
	BleSend2Phone(ucStartChar,2); //���Ϳ�ʼymodemӦ��
	BleDelay_ms(200);//��ʱ����
	BleCmdNormalRxMode_Enter();   //�������ڽ���normal����״̬ 
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
		gstUpdate.unCurrentAppSize= Ymodem_Receive(gucAppBuf);      //ymodem���� bin�ļ�
		//BleDelay_ms(1000);
	}while(gstUpdate.unCurrentAppSize==0);
//	taskEXIT_CRITICAL();
	BleCmdNormalRxMode_Exit(); //���������˳�normal����״̬
	gstUpdate.unCurrentAppADDR=(gstUpdate.unCurrentAppADDR==USER1_APP_ADDR)?USER2_APP_ADDR:USER1_APP_ADDR; //ȡ���ϴ�����λ�ò�ͬ��λ�ý�������
	pData=pGetAppBin(gucAppBuf,gstUpdate.unCurrentAppSize,gstUpdate.unCurrentAppADDR); //ȡbin�ļ�λ��ָ��
	if(pData==NULL)
	{		
		BlePrintf("ble update addr err");
		while(1);
	}
	unBinDatalen=(*(unsigned int *)pData); 		//ȡbin�ļ�����
	gstUpdate.unCurrentAppSize=unBinDatalen;
	pData+=4;									//ƫ�Ƶ�bin�ļ���ʼλ��
	if(CheckAppBin((unsigned int *)pData)!=0)	//���APPbin�ļ�
	{
		BlePrintf("ble update AppBin err");
		while(1);
	}
	//SaveAppBackup(pData,unBinDatalen);             //APP���� 
	
	AppWrite2Flash(pData);
	btIWDG_Refresh();
	//InterFlash_Program(gstUpdate.unCurrentAppADDR,pData,gstUpdate.unCurrentAppSize, FLASH_TYPEPROGRAM_BYTE);//д��APP
	//BleDelay_ms(1000);
	gBleAPPUpdateStatus=eWRITE_APP_OK;
	delay_ms(1000);
	if(gstUpdate.unCurrentAppADDR==USER1_APP_ADDR)      //�ж�λ�ý���Ӧ״̬��¼
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
	SaveSysInfo();//�洢״̬��Ϣ
//	RefreshSysInfo(); 
	
	btResetSysForJump();           //��תAPP
	return BLE_NO_ERR;
}
/**
  * @brief  �����������մ������ݺ���
  * @note   ���յ��Ĵ�������ȡ����������ݺ����ݳ���
  * @param  pData: ���ݲ���ָ�� 
  * @param  Len: ���ݳ���
  * @retval ������� eBleErrorType
  */
static void BleAnalysisData(char *pData,unsigned int Len)
{
	unsigned short int crctemp=0;
	gstBleData.ucCmd=pData[3];                          //��������
	gstBleData.usDataLen=(pData[1]<<8)+pData[2]-1;      //���ݳ���
	gstBleData.usCRC16=(pData[Len-3]<<8)+pData[Len-2];
	crctemp=usCrc16((unsigned char *)&pData[3],gstBleData.usDataLen+1);
	if(crctemp!=gstBleData.usCRC16)
		 crctemp=crctemp;//У��ʧ��
	if(gstBleData.usDataLen<BLE_DATA_BUF_MAX)               //����С��buf��С
	{
		memset(gstBleData.ucData,0,sizeof(gstBleData.ucData));
		memcpy(gstBleData.ucData,(void*)&pData[4],gstBleData.usDataLen);//�������ݵ�buf
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
  * @brief  �������ݵ��ֻ�����
  * @note   ���ݽ���ת��У���֡ͷβ���з��ͣ�У��ת��ǰ����
  * @param  pData: ����ָ��
  * @param  len: ���ݳ���
  */
void BleSend2Phone(char *pData,unsigned int len )
{ 
	char cDataBuf[SEND_2PHONE_BUF_MAX]={0};
	char cDataBufTemp[SEND_2PHONE_BUF_MAX]={0};
	char cHead=BLE_FRAME_HEAD_STR,cTail=BLE_FRAME_TAIL_STR;
	unsigned short  crc16=0,sendlen; 
	crc16=bt_crc16((uint8_t*)&pData[1],len);   //ȥ������ͷ����У��
	
	//cTemp[0]=BLE_FRAME_HEAD_STR ;//ȡͷ 
	cDataBufTemp[1]=len&0x00ff;          //ȡ���ֽ�
	cDataBufTemp[0]=len>>8;          //ȡ���ֽ�  
	
	memcpy((char *)&cDataBufTemp[2],pData,len);                 //ƴ������ 
	
	cDataBufTemp[2+len+1]=crc16&0x00ff;          //ȡ���ֽ�
	cDataBufTemp[2+len]=crc16>>8;          //ȡ���ֽ�  
	
	sendlen=str_esc(cDataBuf,cDataBufTemp,BLE_FRAME_HEAD_STR,BLE_FRAME_TAIL_STR,len+4); //�������к�֡β�ַ���ͬ�Ľ���ת��
   
	Uart5SendBytes(&cHead,1); //����ͷ
	Uart5SendBytes(( char *)cDataBuf,sendlen);//���� 
	Uart5SendBytes(&cTail,1);//����β  
}




