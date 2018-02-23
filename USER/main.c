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
	 taskENTER_CRITICAL();           //�����ٽ���
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
	taskEXIT_CRITICAL();           //�˳��ٽ��� 
	vTaskDelete(StartTask_Handler); //ɾ����ʼ����
} 


#define gDebugUartHandle gstUart1.UartHandle  
/**
  * @brief  ��ӡ���ڳ�ʼ�� 
  * @param  Baud: ���ô��ڲ�����  
  */
static void DebugUartInit(unsigned int Baud )
{  
	gDebugUartHandle.Instance= USART1;
	gDebugUartHandle.Init.BaudRate=Baud;
	gDebugUartHandle.Init.WordLength=UART_WORDLENGTH_8B;   //�ֳ�Ϊ8λ���ݸ�ʽ
	gDebugUartHandle.Init.StopBits=UART_STOPBITS_1;	    //һ��ֹͣλ
	gDebugUartHandle.Init.Parity=UART_PARITY_NONE;		    //����żУ��λ 
	gDebugUartHandle.Init.Mode=UART_MODE_TX_RX;		    //
	HAL_UART_Init( (UART_HandleTypeDef*)&gDebugUartHandle); 
	//HAL_UART_Receive_IT(&gstUart1.UartHandle,(uint8_t *)&gstUart1.ucRecByte,1);  
	//BleRxByteITSet(gBleRxByte);
}
 
int main(void)
{    
	SCB->VTOR = FLASH_BASE ; 
	__set_FAULTMASK(0);//  
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);                 //�жϷ���4 0~15��ռ��ʽ
	MPU_ConfigNOR();
	Cache_Enable();                 //��L1-Cache
	HAL_Init();                     //��ʼ��HAL��    
	Stm32_Clock_Init(432,25,2,9);   //����ʱ��,216Mhz 
	delay_init(216);                //��ʱ��ʼ��
	BSP_NOR_Init(); 
//	InterFlash_SetBankMode(SINGLE_BANK_MODE); //����Ϊsingle bank mode  	
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
  DebugUartInit(115200);	 //���Դ��ڳ�ʼ�� 
	

    //������ʼ����
	xTaskCreate((TaskFunction_t ) start_task,            //������
							(const char*    ) "start_task",          //��������
							(uint16_t       )START_STK_SIZE,         //�����ջ��С
							(void*          )NULL,                   //���ݸ��������Ĳ���
							(UBaseType_t    )START_TASK_PRIO,        //�������ȼ�
							(TaskHandle_t*  )&StartTask_Handler);    //������                
	vTaskStartScheduler();          //����������� 						
}






