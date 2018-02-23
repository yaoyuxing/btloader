#include "sys.h"
#include "delay.h"  
#include "FreeRTOS.h"
#include "task.h"
 #include "string.h"
#include "ff.h"
#include "my_norflash.h"
#include "interflash.h" 
#include "uart.h" 
#include "bootloader.h"
#include "system_info.h"
#include "stdlib.h"
#include "ble_update.h"
#include "oled.h"



unsigned char gucAppBuf[APP_BUF_MAX];
TaskHandle_t  StartTask_Handler;
#define  START_TASK_PRIO   5
#define  START_STK_SIZE   1024
void start_task(void *param)
{
	 taskENTER_CRITICAL();           //进入临界区
	#if INTER_FLASH_TEST
	interflash_test();
	#endif
	#if NOR_FLASH_FS_TEST
	ceate_fs_test();
	#endif
	#if USART_TEST
	usart_test_fun();
	#endif 
  CreateOLEDTask();
	//CreateLocalUartUpdateTask();
	CreateBleTask();
	taskEXIT_CRITICAL();           //退出临界区 
	vTaskDelete(StartTask_Handler); //删除开始任务
} 


#define gDebugUartHandle gstUart1.UartHandle  
/**
  * @brief  打印串口初始化 
  * @param  Baud: 设置串口波特率  
  */
static void DebugUartInit(unsigned int Baud )
{  
	gDebugUartHandle.Instance= USART1;
	gDebugUartHandle.Init.BaudRate=Baud;
	gDebugUartHandle.Init.WordLength=UART_WORDLENGTH_8B;   //字长为8位数据格式
	gDebugUartHandle.Init.StopBits=UART_STOPBITS_1;	    //一个停止位
	gDebugUartHandle.Init.Parity=UART_PARITY_NONE;		    //无奇偶校验位 
	gDebugUartHandle.Init.Mode=UART_MODE_TX_RX;		    //
	HAL_UART_Init( (UART_HandleTypeDef*)&gDebugUartHandle); 
	//HAL_UART_Receive_IT(&gstUart1.UartHandle,(uint8_t *)&gstUart1.ucRecByte,1);  
	//BleRxByteITSet(gBleRxByte);
}
 
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
//	InterFlash_SetBankMode(SINGLE_BANK_MODE); //设置为single bank mode  	
////  InterFlash_EraseSector(SYSTEM_INIT_INFO_ADDR,NV_SIZE_MAX);  
//	SystemInfoInit_BtLoader(); 
	UpdateInit();		
	#if 0
	gstUpdate.unCurrentAppADDR=USER1_APP_ADDR;
	 gstUpdate.eAppUpdateStatus=eUSR1_UPDATE_CHECK;
	SaveSysInfo();
	RunAPP();
	#endif
	btIWDG_Init();
  DebugUartInit(115200);	 //调试串口初始化 
	

    //创建开始任务
	xTaskCreate((TaskFunction_t ) start_task,            //任务函数
							(const char*    ) "start_task",          //任务名称
							(uint16_t       )START_STK_SIZE,         //任务堆栈大小
							(void*          )NULL,                   //传递给任务函数的参数
							(UBaseType_t    )START_TASK_PRIO,        //任务优先级
							(TaskHandle_t*  )&StartTask_Handler);    //任务句柄                
	vTaskStartScheduler();          //开启任务调度 						
}






