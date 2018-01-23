#include "sys.h"
#include "delay.h"  
#include "FreeRTOS.h"
#include "task.h"
 #include "string.h"
#include "ff.h"
#include "my_norflash.h"
#include "interflash.h" 
#include "uart.h"
#include "LocalUart.h"
#include "bootloader.h"
#include "system_info.h"
#include "stdlib.h"
#include "ble_update.h"
 
 
unsigned char gucAppBuf[APP_BUF_MAX];
TaskHandle_t  StartTask_Handler;
#define  START_TASK_PRIO   5
#define  START_STK_SIZE   1024
void start_task(void *param)
{
	 taskENTER_CRITICAL();           //进入临界区
	#ifdef INTER_FLASH_TEST
	interflash_test();
	#endif
	#ifdef NOR_FLASH_FS_TEST
	ceate_fs_test();
	#endif
	#ifdef USART_TEST
	usart_test_fun();
	#endif
	 
	//CreateLocalUartUpdateTask();
	CreateBleTask();
	taskEXIT_CRITICAL();           //进入临界区
	
	
	
	vTaskDelete(StartTask_Handler); //删除开始任务
}
unsigned char buf[1024]="abcd123456";
int main(void)
{  
	
	SCB->VTOR = FLASH_BASE ; 
	__set_FAULTMASK(0);// 
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);                 //中断分组4 0~15抢占方式
	MPU_ConfigNOR();
	Cache_Enable();                 //打开L1-Cache
	HAL_Init();                     //初始化HAL库   
	Stm32_Clock_Init(432,25,2,9);   //设置时钟,216Mhz 
	delay_init(216);                //延时初始化
	BSP_NOR_Init();  
	InterFlash_EraseSector(SYSTEM_INIT_INFO_ADDR,1);          //擦拭  
	SystemInfoInit_BtLoader();
	RefreshSysInfo();
	
	memcpy(gstUpdate.stFireInfo.FireVersion,"abcde",5);
	SaveSysInfo();
//	 delay_ms(1000);
//	memcpy(gstUpdate.stFireInfo.FireVersion,"abcde",5);
//	SaveSysInfo();
//	RefreshSysInfo();
////SetBtLoaderSystemInfoSize(sizeof(gstUpdate)); 
//	
//	memcpy(gstUpdate.stFireInfo.FireVersion,"123456",5);
//	SaveSysInfo();
//	
//	RefreshSysInfo();
//	while(1)
//	{
//	SaveSysInfo();
//	RefreshSysInfo();
//	}
	CheckUpdateStatus();	 
	
    //创建开始任务
	xTaskCreate((TaskFunction_t ) start_task,            //任务函数
							(const char*    ) "start_task",          //任务名称
							(uint16_t       )START_STK_SIZE,        //任务堆栈大小
							(void*          )NULL,                  //传递给任务函数的参数
							(UBaseType_t    )START_TASK_PRIO,       //任务优先级
							(TaskHandle_t*  )&StartTask_Handler);   //任务句柄                
    vTaskStartScheduler();          //开启任务调度 						
}







