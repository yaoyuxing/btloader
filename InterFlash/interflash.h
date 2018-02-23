#ifndef INTER_FLASH_H
#define INTER_FLASH_H 
 
//#define INTER_FLASH_TEST

#define  APP_SIZE_MAX              (768*1024)
#define  BOOT_LOADER_SIZE_MAX      (128*1024) 
#define  NV_SIZE_MAX               (128*1024)
#define  INTER_FLASH_BASE          (0X08000000)
//引导地址
#define  BOOT_LOADER_ADDR          (INTER_FLASH_BASE)

//系统信息地址
#define  SYSTEM_INIT_INFO_ADDR     (BOOT_LOADER_ADDR+BOOT_LOADER_SIZE_MAX)  
//程序地址
#define  USER1_APP_ADDR            (SYSTEM_INIT_INFO_ADDR+NV_SIZE_MAX)
#define  USER2_APP_ADDR            (USER1_APP_ADDR+APP_SIZE_MAX)

//引导函数地址
#define FUN_CODE_MAX_SIZE								(16*1024)

#define  BOOT_LOADER_CODE_SIZE          (64*1024)
#define  BLE_SEND_2_PHONE_FUN_ADDR  		(INTER_FLASH_BASE+BOOT_LOADER_CODE_SIZE)
#define  OLED_UPDATE_OK_FUN_ADDR        (BLE_SEND_2_PHONE_FUN_ADDR+FUN_CODE_MAX_SIZE)
//出厂设置地址
#define  FACTORY_SET_VALUES_ADDR		(OLED_UPDATE_OK_FUN_ADDR+FUN_CODE_MAX_SIZE)
   
//

 #ifdef INTER_FLASH_TEST
 void interflash_test(void); //start task调用此函数创建测试任务
 #endif
 
 
 
 
 
typedef enum  blockmode
{
	DUAL_BANK_MODE=0,
	SINGLE_BANK_MODE=1
}eBlockModeType;







int InterFlash_EraseSector(unsigned int  Addr,unsigned int offset);
int InterFlash_Program(unsigned int Addr,void* pBuf,unsigned int DataCnt ,unsigned int  DataType);
void InterFlash_Read(unsigned int Addr,void* pBuf,unsigned int DataCnt ,unsigned int  DataType);
int InterFlash_SetBankMode(eBlockModeType bankmode);
eBlockModeType InterFlash_GetBankMode(void);
int  CheckProgram(unsigned char *buf ,unsigned int len,unsigned int addr);


#endif
