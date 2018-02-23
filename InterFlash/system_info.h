#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H




typedef struct sys_private_val
{
	unsigned int unPos;         //flash位置标记
	unsigned int unSysBackUpADDR;  //备份数据位置
	unsigned int unBtLoaderSysInfoSize;//升级数据信息
	unsigned int unAppSysInfoSize;  //APP部分系统信息长度
	unsigned int unBlockDevideCntAmount;//系统信息总共分为多少个
	unsigned int unInfoSingleSize;      //单个系统信息大小 =控制结构大小+升级部分系统信息大小+APP部分系统信息大小
	unsigned int  unPosMask;         //位置掩码保证数据不是乱的
	unsigned short usBtloaderCrc16;          //升级部分数据校验
	unsigned short usAppCrc16;          	//APP部分数据校验	
	unsigned char        *pucAppSysInfo ;     //APP部分系统信息指针
	unsigned char        *pucBtLoaderSysInfo ;//升级部分系统信息指针
}stSystemInformationType;



/**
  * @brief  更新备份系统信息到RAM并重新备份存储，在初始化后使用
  */
void RefreshSysInfoBackup(void);
/**
  * @brief  写系统信息到FLASH，在初始化后使用
  */
void SaveSysInfo(void);
/**
  * @brief  更新系统信息到RAM，在初始化后使用
  *	@retval 返回值 0：数据及校验正确，1：校验失败
  */
int RefreshSysInfo(void);
/**
  * @brief   bootloader中初始化 固定长度升级结构和控制结构，申请APP数据空间 
  */
void SystemInfoInit_BtLoader(void);
/**
  * @brief   App中初始化
  *          升级结构和控制结构为固定长度
  * @param   pAppSysInfo:APP需要存储的系统信息起始地址指针
  * @param   len: APP系统信息数据长度
  *	@retval  返回值 上次设置的长度
  */
unsigned int  SystemInfoInit_App(void * pAppSysInfo,unsigned int len);               //App 调用系统信息初始化
/**
  * @brief  擦拭系统信息block
  */
void EraseSysInfoBlock(void);   

 
#endif 
