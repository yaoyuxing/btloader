#ifndef  BOOT_LOADER_H
#define BOOT_LOADER_H
  
#define  MODULE_NAME_LEN_MAX       100
#define  MODULE_HARD_VS_LEN_MAX    10
#define  MODULE_FIRE_VS_LEN_MAX    10
#define  MODULE_TIME_LEN_MAX       16
#define  MODULE_SN_MAX             25
#define  BOOT_LOADER_INIT_FIRST_TIME_NUM   (0x00123456)
#define  FACTORY_STRING             "factory set2"
#define  APP_PATH                  "0:App.bin"
#define  APP_BUF_MAX               (1024*150)
//升级收发字符
//#define  UPDATE_SUCCEED_ACK_BYTE          0X03
//#define  UPDATE_FAILED_ACK_BYTE           0X04
//升级bin分隔字符串
#define  UPDATE_APP_SEPARATE_STR       "#APP SEPARATE LINE#"

#define  FIRE_INFO_BACKUP_ADDR       (gstUpdate.unCurrentAppADDR+APP_SIZE_MAX-1024)
#define  JUMP_VALUE    2
#define  APP_RUN_VALUE    3
typedef  enum 
{
	BOOT_NO_ERR,  
	RAM_ADDR_ERR, 
}eUpdateErrType;

typedef  enum 
{
	eNO_UPDATE=0,
	eAPP_JUMP_UPDATE,
  ePREPARE_UPDATE,
	eSTART_UPDATE,
	eWRITE_APP_OK,
	eUSR1_UPDATE_CHECK,
	eUSR2_UPDATE_CHECK,
	eUSR1_UPDATE_FINISH,
	eUSR2_UPDATE_FINISH, 
	eGET_FACTORY_DATA=0xed,
	eFACTORY_SET=0XEE,
	eFACTORY_SET_OK=0xef
}eUpdateStatusType;
typedef  enum 
{
	eUPDATE_MODE_NONE,
	eUPDATE_MODE_BLE,
	eUPDATE_MODE_4G,
	eUPDATE_MODE_UART,
}eUpdateModeType;
typedef  enum 
{
	eDEV_MODE_NONE,
	eDEV_MODE_BASE_STATION,
	eDEV_MODE_CANE,
}eDeviceModeType;

#define  BOOT_LOADER_SYS_INFO_SIZE           (sizeof (stUpdateInfoType))
typedef struct device_info
{
	unsigned char ucDeviceMode;
}sDeviceInfoType;

typedef struct fireinfo
{
	char  ModuleName[MODULE_NAME_LEN_MAX];
	char  HardVersion[MODULE_HARD_VS_LEN_MAX];
	char  FireVersion[MODULE_FIRE_VS_LEN_MAX];
	char  Time[MODULE_TIME_LEN_MAX];
	char  SN[MODULE_SN_MAX];
	unsigned int BackupAddr;
}stFireInfoType;

typedef struct updateinfo
{ 
  stFireInfoType     stFireInfo;
	unsigned  int      unCurrentAppADDR;
	unsigned  int      unCurrentAppSize;
volatile	eUpdateStatusType  eAppUpdateStatus; 
	eUpdateModeType    eUpdateMode;
	eDeviceModeType    eDeviceMode; 
	unsigned int      ucAppJumpFlag;
	unsigned int 		 FirstTimeNum;
}stUpdateInfoType;

/**************************                       升级部分变量                           ******************************/
typedef void( *pFunType)(void);
extern unsigned char gucAppBuf[APP_BUF_MAX]; 
extern  stUpdateInfoType  gstUpdate;
/*********************************************************************************************************************/

/**************************                       升级部分函数                           ******************************/
eUpdateErrType CheckAppBin(unsigned int *AppBuf);
void SaveAppBackup(char *pData,unsigned int len );
void RunAPP(void);
void UpdateInit(void);
void btUpdateInit_App(void);
void CheckFactory(void);
eUpdateErrType AppWrite2Flash(unsigned char *pData);
unsigned char * pGetAppBin(unsigned char *pBinData,unsigned int len,unsigned int Addr);
eUpdateErrType SetSerialNumber(unsigned char*sn,unsigned char  len );
eUpdateErrType FactoryOK(void);
eUpdateErrType RefreshCurrentAppFireInfo(void);
void BootLoaderStructInit(void);
void btCreateBtTask(void);
int  btBleRxByteFromIT_App(unsigned char byte);
void btUpdateCheckStatus(void);
void btIWDG_Init(void);
void btIWDG_Refresh(void);
void btResetSysForJump(void);
/*********************************************************************************************************************/






/**************************                       常用函数                               ******************************/
/*
* 在给定buf中找字符串
*/
int  findstr(unsigned char  *rx_buf,unsigned int rx_counter, char *str);
unsigned int bytepos(char *str,char byte);
#define  bt_crc16              Cal_CRC16








/*********************************************************************************************************************/
#endif
