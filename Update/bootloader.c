#include "bootloader.h"
#include "ff.h"
#include "stdlib.h"
#include "interflash.h"
#include "system_info.h" 
#include "string.h" 
#include "FreeRTOS.h"
#include "task.h"

stUpdateInfoType  gstUpdate={0}; 
const unsigned char *btSerialNum= (uint8_t *)FACTORY_SET_VALUES_ADDR;
const unsigned char *btFactoryString= (uint8_t *)(FACTORY_SET_VALUES_ADDR+MODULE_SN_MAX);

static void UpadateError(void); 

typedef void(*OledUpdateOkType)(u8 x,u8 y);
typedef void(*BleSend2PhoneType)(char *pData,unsigned int len ); 
 
#define bt_RX_BUFFER_APP_MAX    1024
unsigned char btRxBuffer_App[bt_RX_BUFFER_APP_MAX]={0};
unsigned int  btRxBufferLen=0;
volatile unsigned char btBLE_RX_FRAME_FLAG =0;
TaskHandle_t  btTask_Handler_App;
#define  BT_TASK_PRIO   6
#define  BT_STK_SIZE   1024*10
/**
  * @brief  ��������������
  * @note   ����APP�н���������ת
  * @param  param: ������
  * @retval ��
  */
#define TEMP_BUF_MAX   1024
unsigned char JumpCmd[]={'#',0x00,0x01,'J',0xff,0xff,'@'};//��ת����֡
unsigned char GetFireInfo[]={'#',0x00,0x01,'G',0xff,0xff,'@'};//��ȡ�̼���Ϣ����֡
char   pdata[TEMP_BUF_MAX]={0}; 
void btBleTask_App(void* param)
{

	BleSend2PhoneType  BleSend2PhoneFun =( BleSend2PhoneType)((BLE_SEND_2_PHONE_FUN_ADDR+1 ));
	RefreshSysInfo();//��ȡ��Ϣ
  btUpdateCheckStatus();
	btIWDG_Init();
	while(1)
	{
		vTaskDelay(1); 
		btIWDG_Refresh();
		if(btBLE_RX_FRAME_FLAG==SET)      //���յ�������������֡
		{  
			if(memcmp(btRxBuffer_App,JumpCmd,sizeof JumpCmd)==0)//�Ƚ�֡
			{ 
				gstUpdate.eAppUpdateStatus=eAPP_JUMP_UPDATE;//��������״̬Ϊ��ת
				SaveSysInfo();
				NVIC_SystemReset();//���������
			}
			if(memcmp(btRxBuffer_App,GetFireInfo,sizeof GetFireInfo)==0)//�Ƚ�֡
			{   
				memset(pdata,0,TEMP_BUF_MAX);  
				sprintf(pdata,"G{\"Version\":{\"ModuleName\":\"%s\",\"HardVersion\":\"%s\",\"FireVersion\":\"%s\",\"DataTime\":\"%s\"}}"\
				,gstUpdate.stFireInfo.ModuleName\
				,gstUpdate.stFireInfo.HardVersion\
				,gstUpdate.stFireInfo.FireVersion\
				,gstUpdate.stFireInfo.Time);  
				BleSend2PhoneFun(pdata,strlen(pdata));  
			}
			memset(btRxBuffer_App,0,bt_RX_BUFFER_APP_MAX);//��ս���
			btBLE_RX_FRAME_FLAG=RESET;//��λ��־
		}
  	    
	}
}

/**
  * @brief  ������������
  * @note   ����APP�н���������ת
  * @param  param: ������
  * @retval ��
  */
void btCreateBtTask()
{
	
	//������ʼ����
  xTaskCreate((TaskFunction_t ) btBleTask_App,            //������
						(const char*    ) "btBle_task",          //��������
						(uint16_t       )BT_STK_SIZE,         //�����ջ��С
						(void*          )NULL,                   //���ݸ��������Ĳ���
						(UBaseType_t    )BT_TASK_PRIO,        //�������ȼ�
						(TaskHandle_t*  )&btTask_Handler_App);    //������ 

}
/**
  * @brief  ���������жϽ��ջص�����
  * @note   ����App���������ж��У���hal���жϻص�������ǰ��֤byte�ܽ��յ�ÿ���жϹ����Ĵ�������
  * @param  byte: ���ڽ�������
  * @retval ��������������ƺ�������λ��
  */
int btBleRxByteFromIT_App(unsigned char byte)
{
	static unsigned char ucRecByte=0,EscapeFlag=0,FrameStart=0;
	const static unsigned char ESCAPE_CH=0x7d,XOR_CH=0x20;
#define    HEAD_CH  '#'
#define    TAIL_CH  '@'
	ucRecByte=byte; 
	if((ucRecByte==ESCAPE_CH)&&(EscapeFlag==RESET))//�ӵ�ת���ַ�
	{
		EscapeFlag=SET;  //���ñ��
		return 0;
	}
	else if(EscapeFlag==SET) 
	{ 
		ucRecByte=ucRecByte^XOR_CH; //  ȡת����ַ�
		EscapeFlag=RESET; 
	}   
	if(ucRecByte==HEAD_CH)//�ҵ�֡ͷ
	{ 
		btRxBufferLen=0;
		btRxBuffer_App[btRxBufferLen]=HEAD_CH;  //ȡ֡ͷ
		btRxBufferLen+=1;       //���ȼ�1
		FrameStart=SET; //֡��ʼ
	}
	else if((ucRecByte!=TAIL_CH)&&(FrameStart==SET)) //֡��ʼ��������
	{
		btRxBuffer_App[btRxBufferLen++]=ucRecByte; 
	}
	else if((FrameStart==SET)&&(ucRecByte==TAIL_CH)) //���յ�֡β
	{
		btRxBuffer_App[btRxBufferLen++]=ucRecByte;//ȡ֡β 
		btBLE_RX_FRAME_FLAG=SET;  
	}
	return 0;
}
IWDG_HandleTypeDef IwdgHandle;
void btIWDG_Init(void)
{  
	IwdgHandle.Init.Prescaler=IWDG_PRESCALER_256;	/* ����ʱ�ӷ�Ƶ,32K/256=125HZ(8ms)*/
	IwdgHandle.Init.Reload=2500;	/* ι��ʱ�� 20s/8MS=625 .ע�ⲻ�ܴ���0xfff*/
	IwdgHandle.Init.Window=IWDG_WINR_WIN;
	IwdgHandle.Instance= IWDG; 
	HAL_IWDG_Init(&IwdgHandle); 
	
}
void btIWDG_Refresh(void)
{
	HAL_IWDG_Refresh(&IwdgHandle);
}

void UpdateInit(void)
{ 
	CheckFactory();			//��������
	InterFlash_SetBankMode(SINGLE_BANK_MODE); //����Ϊsingle bank mode  	 
	SystemInfoInit_BtLoader(); 
	if(gstUpdate.ucAppJumpFlag==JUMP_VALUE)   //��ת��־��λ
	{
		gstUpdate.ucAppJumpFlag=APP_RUN_VALUE; 
		SaveSysInfo();
		RunAPP();
	}	
	switch((eUpdateStatusType )gstUpdate.eAppUpdateStatus)
	{
		case  eUSR1_UPDATE_FINISH:
					gstUpdate.eAppUpdateStatus=eNO_UPDATE;
					SaveSysInfo();
					break;
		case  eUSR2_UPDATE_FINISH: 
					gstUpdate.eAppUpdateStatus=eNO_UPDATE;
					SaveSysInfo();
					break;
		case  eUSR1_UPDATE_CHECK:
					gstUpdate.unCurrentAppADDR=USER2_APP_ADDR;
					RefreshCurrentAppFireInfo(); 
					UpadateError();
					break;
		case  eUSR2_UPDATE_CHECK:
					gstUpdate.unCurrentAppADDR=USER1_APP_ADDR;
					RefreshCurrentAppFireInfo(); 
					UpadateError(); 
					break; 
		case eAPP_JUMP_UPDATE:
					break;
		case eFACTORY_SET:
					break;
		default : 
					gstUpdate.eAppUpdateStatus=eNO_UPDATE;
					SaveSysInfo();
					break;
	} 
}
void CheckFactory(void)
{
	unsigned int cnt=0,addr=gstUpdate.unCurrentAppADDR;
	unsigned char *pFireData=(unsigned char *)&gstUpdate.stFireInfo;
	gstUpdate.stFireInfo.BackupAddr=gstUpdate.unCurrentAppADDR;
	if(memcmp(btFactoryString,FACTORY_STRING,sizeof(FACTORY_STRING))!=0)
	{
		EraseSysInfoBlock();
		memset((void*)&gstUpdate,0,sizeof(stUpdateInfoType));
		strcpy((void*)gstUpdate.stFireInfo.FireVersion,"1.0.0");  //����̼���Ϣ
		strcpy((void*)gstUpdate.stFireInfo.HardVersion,"1.0");
		strcpy((void*)gstUpdate.stFireInfo.ModuleName,"GNSS.BR");
		strcpy((void*)gstUpdate.stFireInfo.Time,"1000000");
		gstUpdate.eAppUpdateStatus=eFACTORY_SET;
		gstUpdate.eDeviceMode=eDEV_MODE_BASE_STATION; //����Ϊ��վģʽ
		gstUpdate.unCurrentAppADDR=USER1_APP_ADDR;
		gstUpdate.unCurrentAppSize=0;  
		HAL_FLASH_Unlock();
		for(cnt=0,addr=FIRE_INFO_BACKUP_ADDR;cnt<sizeof (gstUpdate.stFireInfo);cnt++)//д��fire��Ϣ
		{
				 HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,addr,pFireData[cnt]);   //д��byte 
				 addr++;
		}
//		if(CheckProgram(pFireData,sizeof(gstUpdate.stFireInfo),FIRE_INFO_BACKUP_ADDR)==1)
//			 while(1);
		HAL_FLASH_Lock();
	}
}
/*
�ṹ���ʼ������ 
*typedef struct updateinfo
{ 
  stFireInfoType     stFireInfo;
	unsigned  int      unCurrentAppADDR;
	unsigned  int      unCurrentAppSize;
	eUpdateStatusType  eAppUpdateStatus; 
	eUpdateModeType    eUpdateMode;
	eDeviceModeType    eDeviceMode;
	stBleInfoType      stBleInfo;
	
}stUpdateInfoType;
*
*/
void BootLoaderStructInit(void)
{
	
	if(memcmp(gstUpdate.stFireInfo.SN,btSerialNum,MODULE_SN_MAX)!=0)
	{
		memcpy(gstUpdate.stFireInfo.SN,btSerialNum,MODULE_SN_MAX);
		SaveSysInfo(); 
	}  
	RefreshCurrentAppFireInfo();
	
}
void SaveAppBackup(char *pData,unsigned int len )
{
//	unsigned char res=0;
//	unsigned int  len=0;
//	FIL *file=malloc(sizeof (FIL));
//	res=f_open(file,APP_PATH,FA_CREATE_ALWAYS|FA_WRITE|FA_READ);
//	res=f_write(file,gucAppBuf,gstUpdate.unCurrentAppSize,&len);
//	res=f_close(file);
//	free(file);
//	file=NULL;
}
void RunAPP(void)
{ 
	
	unsigned int jumpAdr=gstUpdate.unCurrentAppADDR;
	pFunType JumpFun; 
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//ϵͳ�Ѿ�����
	{
		taskENTER_CRITICAL(); 
		__set_FAULTMASK(1);//  ��תǰ���ж�
		JumpFun=(pFunType)(*(__IO u32 *)(jumpAdr+4)); //����APP
		JumpFun(); 
		taskEXIT_CRITICAL();
	}
	else
  {  
		__set_FAULTMASK(1); //  ��תǰ���ж�  
		JumpFun=(pFunType)(*(__IO u32 *)(jumpAdr+4)); //����APP
		JumpFun(); 
  }
	
	
}

void UpadateError(void)
{
	
}
/**
  * @brief  ��������д��flash����
  * @note   ���ö�Ӧflash����,д��bin�ļ�����д���Ӧ�̼���Ϣ
  * @retval ������� eBleErrorType
  */
eUpdateErrType AppWrite2Flash(unsigned char *pData)
{
	unsigned int cnt=0,addr=gstUpdate.unCurrentAppADDR;
	unsigned char *pFireData=(unsigned char *)&gstUpdate.stFireInfo;
	gstUpdate.stFireInfo.BackupAddr=gstUpdate.unCurrentAppADDR;
	InterFlash_EraseSector(addr,APP_SIZE_MAX); 
//	InterFlash_Program(gstUpdate.unCurrentAppADDR,pData,gstUpdate.unCurrentAppSize, FLASH_TYPEPROGRAM_BYTE);//д��APP
	HAL_FLASH_Unlock();
	for(cnt=0;cnt<gstUpdate.unCurrentAppSize;cnt++)  //д��APP
	{
			 HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,addr,pData[cnt]);   //д��byte 
			 addr++;
	}
	if(CheckProgram(pData,gstUpdate.unCurrentAppSize,gstUpdate.unCurrentAppADDR)==1)
			 while(1);

	for(cnt=0,addr=FIRE_INFO_BACKUP_ADDR;cnt<sizeof (gstUpdate.stFireInfo);cnt++)//д��fire��Ϣ
	{
			 HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,addr,pFireData[cnt]);   //д��byte 
			 addr++;
	}
	if(CheckProgram(pFireData,sizeof(gstUpdate.stFireInfo),FIRE_INFO_BACKUP_ADDR)==1)
			 while(1);
	HAL_FLASH_Lock();
	return BOOT_NO_ERR;
}
/**
  * @brief  ��ȡ��ǰ���е�APP�Ĺ̼���Ϣ
  * @note   ��Ҫ����������δ�ɹ���ȡ��ǰ�Ĺ̼���Ϣ
  * @retval ������� eBleErrorType
  */
eUpdateErrType RefreshCurrentAppFireInfo(void)
{
	unsigned char *pFireData=(unsigned char *)&gstUpdate.stFireInfo;
	unsigned int cnt=0,addr=FIRE_INFO_BACKUP_ADDR;
	for(cnt=0,addr=FIRE_INFO_BACKUP_ADDR;cnt<sizeof (gstUpdate.stFireInfo);cnt++)//д��fire��Ϣ
	{
		*pFireData++=*(unsigned char *)addr;
		addr++;
	}
	return BOOT_NO_ERR;
}
void btUpdateCheckStatus(void)
{
	char UpdateOKChars[]={'U',0x03};
	OledUpdateOkType   OledUpdateAppOkFun=( OledUpdateOkType)((OLED_UPDATE_OK_FUN_ADDR+1 ));
	BleSend2PhoneType  BleSend2PhoneFun=( BleSend2PhoneType)((BLE_SEND_2_PHONE_FUN_ADDR+1 ));
	if(gstUpdate.eAppUpdateStatus==eUSR1_UPDATE_CHECK)      //usr1����
	{
		if(gstUpdate.unCurrentAppADDR==USER1_APP_ADDR)        //�жϵ�ַ
		{
			gstUpdate.eAppUpdateStatus=eUSR1_UPDATE_FINISH;      //�����ɹ�
			SaveSysInfo();    
			OledUpdateAppOkFun(18,24);
			BleSend2PhoneFun(UpdateOKChars,2);
			
		}
	} 
	else if(gstUpdate.eAppUpdateStatus==eUSR2_UPDATE_CHECK)// usr2����
	{
		if(gstUpdate.unCurrentAppADDR==USER2_APP_ADDR)   
		{
			gstUpdate.eAppUpdateStatus=eUSR2_UPDATE_FINISH;  //�����ɹ�
			SaveSysInfo(); 
			OledUpdateAppOkFun(18,24);
			BleSend2PhoneFun(UpdateOKChars,2);
			
		}
	}
}

/**
  * @brief  APP��ʼ��������Ϣ��������SystemInfoInit_App�� ���ж�ǰ
  */
void btUpdateInit_App(void)
{

	//__set_FAULTMASK(0);  
	RefreshSysInfo();              //��ȡϵͳ��Ϣ
	if((gstUpdate.unCurrentAppADDR==USER1_APP_ADDR)||(gstUpdate.unCurrentAppADDR==USER2_APP_ADDR)) //��ȷ������λ�þ������ж�����ƫ��
		SCB->VTOR = FLASH_BASE|(gstUpdate.unCurrentAppADDR-FLASH_BASE);
	
}

eUpdateErrType CheckAppBin(unsigned int *AppBuf)
{ 
	if((AppBuf[0]&0xf0000000)!=0x20000000)
		return RAM_ADDR_ERR;
	return BOOT_NO_ERR;
} 
void btResetSysForJump(void)
{
		gstUpdate.ucAppJumpFlag=JUMP_VALUE;
		SaveSysInfo();
		NVIC_SystemReset();
}
unsigned char * pGetAppBin(unsigned char *pBinData,unsigned int len,unsigned int Addr)
{
	unsigned char * pReturn=pBinData;
	if(Addr==USER1_APP_ADDR)
	{
		return pBinData;
	}
	else if(Addr==USER2_APP_ADDR)
	{
		while(len--)
		{
			if(*pReturn==((char *)UPDATE_APP_SEPARATE_STR)[0] )//�Ƚ��Ƿ�ͷָ��ַ���ͷ��ͬ
			{
				if(memcmp(pReturn,UPDATE_APP_SEPARATE_STR,strlen(UPDATE_APP_SEPARATE_STR))==0)  //�Ƚ��Ƿ�ͷָ��ַ�����ͬ
					return (pReturn+strlen(UPDATE_APP_SEPARATE_STR));//����ƫ�Ʒָ��ַ�����ָ��
			}
			pReturn++;
		}
	}
	return NULL; //δ�ҵ��ָ��ַ���
}
eUpdateErrType SetSerialNumber(unsigned char*sn,unsigned char  len )
{ 
	memset((char *)gstUpdate.stFireInfo.SN,0,MODULE_SN_MAX);
	memcpy((char *)gstUpdate.stFireInfo.SN,sn,len);
	InterFlash_SetBankMode(SINGLE_BANK_MODE); //����Ϊsingle bank mode  
	InterFlash_Program(FACTORY_SET_VALUES_ADDR,(char *)gstUpdate.stFireInfo.SN,MODULE_SN_MAX,FLASH_TYPEPROGRAM_BYTE); 
	
	InterFlash_Read(FACTORY_SET_VALUES_ADDR,(char *)gstUpdate.stFireInfo.SN,sizeof(gstUpdate.stFireInfo.SN),FLASH_TYPEPROGRAM_BYTE); 
	return BOOT_NO_ERR;
}
eUpdateErrType FactoryOK(void)
{  
	InterFlash_SetBankMode(SINGLE_BANK_MODE); //����Ϊsingle bank mode  
	InterFlash_Program(FACTORY_SET_VALUES_ADDR+MODULE_SN_MAX,(char *) FACTORY_STRING,sizeof(FACTORY_STRING),FLASH_TYPEPROGRAM_BYTE); 
	if(CheckProgram((unsigned char *)FACTORY_STRING,sizeof(FACTORY_STRING),FACTORY_SET_VALUES_ADDR+MODULE_SN_MAX)==1)
			 while(1);
	  
	return BOOT_NO_ERR;
}

/**************************                       ���ú���                               ******************************/
#include "string.h"
//�����ַ���
#define RX_BUF_MAX 1024
typedef struct str_buf
{
  u8 Data[RX_BUF_MAX];
  u32 Len;
}sBufStrType;
/*
* �ڸ���buf�����ַ���
*/
int  findstr(u8 *rx_buf,unsigned int rx_counter,char *str)
{
	char * pTemp=str;
	u32  strLen=0;
	sBufStrType big,target;
	u32 counter1=0,counter2=0,temp,times=0;
	for(;*pTemp!='\0';)
	{ 
		pTemp++;
		strLen++;
	}
	memset(big.Data,0,RX_BUF_MAX);
	memset(target.Data,0,RX_BUF_MAX);
	memcpy(big.Data,(u8 *)rx_buf,rx_counter);
	memcpy(target.Data,str,strLen);
	big.Len=rx_counter;
	target.Len=strLen ;
	temp=counter1;
	if(big.Len<target.Len)  return 0;
	for(;counter1<big.Len;)    
	{
		if(big.Data[counter1]==target.Data[0])
		{
			while((big.Data[counter1]==target.Data[counter2])&&(counter2<target.Len))
			{
				counter1++;
				counter2++;
			}
		}
		if(counter2==target.Len)
		{
			times++; 
		}
		counter2=0; 
		temp=temp+1;
		counter1=temp;
	}
	if(times>0) 
		return 1;
	else       
	 return 0;
}
/*
* �ַ��������ӵ�ǰλ��ƫ�Ƶ�Ŀ��㷵�ؾ���
*
*/
unsigned int bytepos(char *str,char byte)
{
	unsigned int len=0;
	for(len=0;*str!=byte;len++)
	{
		str++;
	}
	return len;
}
/*********************************************************************************************************************/








