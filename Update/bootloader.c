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
  * @brief  蓝牙升级任务函数
  * @note   用于APP中接收命令跳转
  * @param  param: 不处理
  * @retval 无
  */
#define TEMP_BUF_MAX   1024
unsigned char JumpCmd[]={'#',0x00,0x01,'J',0xff,0xff,'@'};//跳转命令帧
unsigned char GetFireInfo[]={'#',0x00,0x01,'G',0xff,0xff,'@'};//获取固件信息命令帧
char   pdata[TEMP_BUF_MAX]={0}; 
void btBleTask_App(void* param)
{

	BleSend2PhoneType  BleSend2PhoneFun =( BleSend2PhoneType)((BLE_SEND_2_PHONE_FUN_ADDR+1 ));
	RefreshSysInfo();//读取信息
  btUpdateCheckStatus();
	btIWDG_Init();
	while(1)
	{
		vTaskDelay(1); 
		btIWDG_Refresh();
		if(btBLE_RX_FRAME_FLAG==SET)      //接收到蓝牙串口数据帧
		{  
			if(memcmp(btRxBuffer_App,JumpCmd,sizeof JumpCmd)==0)//比较帧
			{ 
				gstUpdate.eAppUpdateStatus=eAPP_JUMP_UPDATE;//设置升级状态为跳转
				SaveSysInfo();
				NVIC_SystemReset();//保存后重启
			}
			if(memcmp(btRxBuffer_App,GetFireInfo,sizeof GetFireInfo)==0)//比较帧
			{   
				memset(pdata,0,TEMP_BUF_MAX);  
				sprintf(pdata,"G{\"Version\":{\"ModuleName\":\"%s\",\"HardVersion\":\"%s\",\"FireVersion\":\"%s\",\"DataTime\":\"%s\"}}"\
				,gstUpdate.stFireInfo.ModuleName\
				,gstUpdate.stFireInfo.HardVersion\
				,gstUpdate.stFireInfo.FireVersion\
				,gstUpdate.stFireInfo.Time);  
				BleSend2PhoneFun(pdata,strlen(pdata));  
			}
			memset(btRxBuffer_App,0,bt_RX_BUFFER_APP_MAX);//清空接收
			btBLE_RX_FRAME_FLAG=RESET;//复位标志
		}
  	    
	}
}

/**
  * @brief  创建蓝牙任务
  * @note   用于APP中接收命令跳转
  * @param  param: 不处理
  * @retval 无
  */
void btCreateBtTask()
{
	
	//创建开始任务
  xTaskCreate((TaskFunction_t ) btBleTask_App,            //任务函数
						(const char*    ) "btBle_task",          //任务名称
						(uint16_t       )BT_STK_SIZE,         //任务堆栈大小
						(void*          )NULL,                   //传递给任务函数的参数
						(UBaseType_t    )BT_TASK_PRIO,        //任务优先级
						(TaskHandle_t*  )&btTask_Handler_App);    //任务句柄 

}
/**
  * @brief  引导蓝牙中断接收回调函数
  * @note   用于App蓝牙接收中断中，在hal库中断回调函数最前保证byte能接收到每个中断过来的串口数据
  * @param  byte: 串口接收数据
  * @retval 无意义仅用来控制函数结束位置
  */
int btBleRxByteFromIT_App(unsigned char byte)
{
	static unsigned char ucRecByte=0,EscapeFlag=0,FrameStart=0;
	const static unsigned char ESCAPE_CH=0x7d,XOR_CH=0x20;
#define    HEAD_CH  '#'
#define    TAIL_CH  '@'
	ucRecByte=byte; 
	if((ucRecByte==ESCAPE_CH)&&(EscapeFlag==RESET))//接到转义字符
	{
		EscapeFlag=SET;  //设置标记
		return 0;
	}
	else if(EscapeFlag==SET) 
	{ 
		ucRecByte=ucRecByte^XOR_CH; //  取转义后字符
		EscapeFlag=RESET; 
	}   
	if(ucRecByte==HEAD_CH)//找到帧头
	{ 
		btRxBufferLen=0;
		btRxBuffer_App[btRxBufferLen]=HEAD_CH;  //取帧头
		btRxBufferLen+=1;       //长度加1
		FrameStart=SET; //帧开始
	}
	else if((ucRecByte!=TAIL_CH)&&(FrameStart==SET)) //帧开始接收数据
	{
		btRxBuffer_App[btRxBufferLen++]=ucRecByte; 
	}
	else if((FrameStart==SET)&&(ucRecByte==TAIL_CH)) //接收到帧尾
	{
		btRxBuffer_App[btRxBufferLen++]=ucRecByte;//取帧尾 
		btBLE_RX_FRAME_FLAG=SET;  
	}
	return 0;
}
IWDG_HandleTypeDef IwdgHandle;
void btIWDG_Init(void)
{  
	IwdgHandle.Init.Prescaler=IWDG_PRESCALER_256;	/* 狗狗时钟分频,32K/256=125HZ(8ms)*/
	IwdgHandle.Init.Reload=2500;	/* 喂狗时间 20s/8MS=625 .注意不能大于0xfff*/
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
	CheckFactory();			//出厂设置
	InterFlash_SetBankMode(SINGLE_BANK_MODE); //设置为single bank mode  	 
	SystemInfoInit_BtLoader(); 
	if(gstUpdate.ucAppJumpFlag==JUMP_VALUE)   //跳转标志置位
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
		strcpy((void*)gstUpdate.stFireInfo.FireVersion,"1.0.0");  //重设固件信息
		strcpy((void*)gstUpdate.stFireInfo.HardVersion,"1.0");
		strcpy((void*)gstUpdate.stFireInfo.ModuleName,"GNSS.BR");
		strcpy((void*)gstUpdate.stFireInfo.Time,"1000000");
		gstUpdate.eAppUpdateStatus=eFACTORY_SET;
		gstUpdate.eDeviceMode=eDEV_MODE_BASE_STATION; //设置为基站模式
		gstUpdate.unCurrentAppADDR=USER1_APP_ADDR;
		gstUpdate.unCurrentAppSize=0;  
		HAL_FLASH_Unlock();
		for(cnt=0,addr=FIRE_INFO_BACKUP_ADDR;cnt<sizeof (gstUpdate.stFireInfo);cnt++)//写入fire信息
		{
				 HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,addr,pFireData[cnt]);   //写入byte 
				 addr++;
		}
//		if(CheckProgram(pFireData,sizeof(gstUpdate.stFireInfo),FIRE_INFO_BACKUP_ADDR)==1)
//			 while(1);
		HAL_FLASH_Lock();
	}
}
/*
结构体初始化函数 
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
	if(xTaskGetSchedulerState()!=taskSCHEDULER_NOT_STARTED)//系统已经运行
	{
		taskENTER_CRITICAL(); 
		__set_FAULTMASK(1);//  跳转前关中断
		JumpFun=(pFunType)(*(__IO u32 *)(jumpAdr+4)); //跳至APP
		JumpFun(); 
		taskEXIT_CRITICAL();
	}
	else
  {  
		__set_FAULTMASK(1); //  跳转前关中断  
		JumpFun=(pFunType)(*(__IO u32 *)(jumpAdr+4)); //跳至APP
		JumpFun(); 
  }
	
	
}

void UpadateError(void)
{
	
}
/**
  * @brief  升级程序写入flash函数
  * @note   擦拭对应flash扇区,写入bin文件并且写入对应固件信息
  * @retval 错误代码 eBleErrorType
  */
eUpdateErrType AppWrite2Flash(unsigned char *pData)
{
	unsigned int cnt=0,addr=gstUpdate.unCurrentAppADDR;
	unsigned char *pFireData=(unsigned char *)&gstUpdate.stFireInfo;
	gstUpdate.stFireInfo.BackupAddr=gstUpdate.unCurrentAppADDR;
	InterFlash_EraseSector(addr,APP_SIZE_MAX); 
//	InterFlash_Program(gstUpdate.unCurrentAppADDR,pData,gstUpdate.unCurrentAppSize, FLASH_TYPEPROGRAM_BYTE);//写入APP
	HAL_FLASH_Unlock();
	for(cnt=0;cnt<gstUpdate.unCurrentAppSize;cnt++)  //写入APP
	{
			 HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,addr,pData[cnt]);   //写入byte 
			 addr++;
	}
	if(CheckProgram(pData,gstUpdate.unCurrentAppSize,gstUpdate.unCurrentAppADDR)==1)
			 while(1);

	for(cnt=0,addr=FIRE_INFO_BACKUP_ADDR;cnt<sizeof (gstUpdate.stFireInfo);cnt++)//写入fire信息
	{
			 HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE,addr,pFireData[cnt]);   //写入byte 
			 addr++;
	}
	if(CheckProgram(pFireData,sizeof(gstUpdate.stFireInfo),FIRE_INFO_BACKUP_ADDR)==1)
			 while(1);
	HAL_FLASH_Lock();
	return BOOT_NO_ERR;
}
/**
  * @brief  提取当前运行的APP的固件信息
  * @note   主要是用于升级未成功后取以前的固件信息
  * @retval 错误代码 eBleErrorType
  */
eUpdateErrType RefreshCurrentAppFireInfo(void)
{
	unsigned char *pFireData=(unsigned char *)&gstUpdate.stFireInfo;
	unsigned int cnt=0,addr=FIRE_INFO_BACKUP_ADDR;
	for(cnt=0,addr=FIRE_INFO_BACKUP_ADDR;cnt<sizeof (gstUpdate.stFireInfo);cnt++)//写入fire信息
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
	if(gstUpdate.eAppUpdateStatus==eUSR1_UPDATE_CHECK)      //usr1升级
	{
		if(gstUpdate.unCurrentAppADDR==USER1_APP_ADDR)        //判断地址
		{
			gstUpdate.eAppUpdateStatus=eUSR1_UPDATE_FINISH;      //升级成功
			SaveSysInfo();    
			OledUpdateAppOkFun(18,24);
			BleSend2PhoneFun(UpdateOKChars,2);
			
		}
	} 
	else if(gstUpdate.eAppUpdateStatus==eUSR2_UPDATE_CHECK)// usr2升级
	{
		if(gstUpdate.unCurrentAppADDR==USER2_APP_ADDR)   
		{
			gstUpdate.eAppUpdateStatus=eUSR2_UPDATE_FINISH;  //升级成功
			SaveSysInfo(); 
			OledUpdateAppOkFun(18,24);
			BleSend2PhoneFun(UpdateOKChars,2);
			
		}
	}
}

/**
  * @brief  APP初始化升级信息，调用在SystemInfoInit_App后 ，中断前
  */
void btUpdateInit_App(void)
{

	//__set_FAULTMASK(0);  
	RefreshSysInfo();              //获取系统信息
	if((gstUpdate.unCurrentAppADDR==USER1_APP_ADDR)||(gstUpdate.unCurrentAppADDR==USER2_APP_ADDR)) //正确的升级位置就设置中断向量偏移
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
			if(*pReturn==((char *)UPDATE_APP_SEPARATE_STR)[0] )//比较是否和分隔字符串头相同
			{
				if(memcmp(pReturn,UPDATE_APP_SEPARATE_STR,strlen(UPDATE_APP_SEPARATE_STR))==0)  //比较是否和分隔字符串相同
					return (pReturn+strlen(UPDATE_APP_SEPARATE_STR));//返回偏移分隔字符串的指针
			}
			pReturn++;
		}
	}
	return NULL; //未找到分隔字符串
}
eUpdateErrType SetSerialNumber(unsigned char*sn,unsigned char  len )
{ 
	memset((char *)gstUpdate.stFireInfo.SN,0,MODULE_SN_MAX);
	memcpy((char *)gstUpdate.stFireInfo.SN,sn,len);
	InterFlash_SetBankMode(SINGLE_BANK_MODE); //设置为single bank mode  
	InterFlash_Program(FACTORY_SET_VALUES_ADDR,(char *)gstUpdate.stFireInfo.SN,MODULE_SN_MAX,FLASH_TYPEPROGRAM_BYTE); 
	
	InterFlash_Read(FACTORY_SET_VALUES_ADDR,(char *)gstUpdate.stFireInfo.SN,sizeof(gstUpdate.stFireInfo.SN),FLASH_TYPEPROGRAM_BYTE); 
	return BOOT_NO_ERR;
}
eUpdateErrType FactoryOK(void)
{  
	InterFlash_SetBankMode(SINGLE_BANK_MODE); //设置为single bank mode  
	InterFlash_Program(FACTORY_SET_VALUES_ADDR+MODULE_SN_MAX,(char *) FACTORY_STRING,sizeof(FACTORY_STRING),FLASH_TYPEPROGRAM_BYTE); 
	if(CheckProgram((unsigned char *)FACTORY_STRING,sizeof(FACTORY_STRING),FACTORY_SET_VALUES_ADDR+MODULE_SN_MAX)==1)
			 while(1);
	  
	return BOOT_NO_ERR;
}

/**************************                       常用函数                               ******************************/
#include "string.h"
//定义字符串
#define RX_BUF_MAX 1024
typedef struct str_buf
{
  u8 Data[RX_BUF_MAX];
  u32 Len;
}sBufStrType;
/*
* 在给定buf中找字符串
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
* 字符串操作从当前位置偏移到目标点返回距离
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








