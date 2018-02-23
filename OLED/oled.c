#include "oled.h"
#include "stdlib.h"
#include "delay.h" 
#include "oledfont.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bootloader.h" 
#include "system_info.h"
#include "my_gui.h"
#include "ymodem.h"
#include "interflash.h"

u8 OLED_GRAM[128][8]={0};	 


TaskHandle_t  OLEDTask_Handler;
#define  OLED_TASK_PRIO   8
#define  OLED_STK_SIZE   1024 
static void OLEDTask(void* param);

void CreateOLEDTask(void)
{
	xTaskCreate((TaskFunction_t )OLEDTask,            //任务函数
							(const char*    )"OLED_task",         //任务名称
							(uint16_t       )OLED_STK_SIZE,        //任务堆栈大小
							(void*          )NULL,                //传递给任务函数的参数
							(UBaseType_t    )OLED_TASK_PRIO,       //任务优先级
							(TaskHandle_t*  )&OLEDTask_Handler);   //任务句柄  	 	
}
static void OLEDTask(void* param)
{ 
	unsigned int BootTimeOut=0x10;  //boot等待命令超时时间
	unsigned int UpdateTimeOut=0x600;  //升级超时时间
	OLED_Init(); //oled初始化 
	OLED_Clear(); 
	
	#if 1
		while(--BootTimeOut)
		{   
			GUI_DeviceBootInit(14,24);  //显示boot初始化中  
			if(gOledUpdateStatus==ePREPARE_UPDATE||gOledUpdateStatus==eFACTORY_SET)   //有升级跳出
			{ 
				break;
			}			
		} 
		if(gOledUpdateStatus==eFACTORY_SET)   //如果出厂设置
		{ 
			while(1)
			{
				GUI_DeviceFactorySet(10,24); //显示出厂设置
				if(gOledUpdateStatus==eFACTORY_SET_OK)//出厂设置成功
				{
					gOledUpdateStatus=eNO_UPDATE;
					SaveSysInfo();
					GUI_DeviceFactorySetOK(16,24);//显示设置成功
					BootTimeOut=0;
					break;
				}
			}
		} 
		if(BootTimeOut) //未超时就检测到准备升级
		{ 
			while(UpdateTimeOut--)    //升级过程超时时间内处理
			{
				if(gOledUpdateStatus==ePREPARE_UPDATE)
				{
						GUI_DeviceUpdatePrepare(14,24);	//显示升级准备
				}
				if(gOledUpdateStatus==eSTART_UPDATE)
				{
						GUI_DeviceUpdate(28,24);				//显示升级中
				}
				if(gOledUpdateStatus==eWRITE_APP_OK)
				{
						GUI_DeviceWriteAppOk(18,24);		//显示写入成功 校验 
						while(1);
				} 
			} 
		}
		gOledUpdateStatus=eNO_UPDATE;       //超时修改升级状态为无
		SaveSysInfo();
		#if 0
		OLED_Clear();
		while(1);
		#else
			#if 0
			gstUpdate.unCurrentAppADDR=USER1_APP_ADDR;
			gstUpdate.eAppUpdateStatus=eUSR1_UPDATE_CHECK;
			gstUpdate.ucAppJumpFlag=SET;
			SaveSysInfo();
			NVIC_SystemReset();
			RunAPP();
			#else
			btResetSysForJump(); 
			#endif
		#endif
	
	#else 
	while(1)
	GUI_DeviceUpdateAppOk(28,24); 
	#endif
} 


//更新显存到LCD		 
void OLED_Refresh_Gram(void)
{
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		for(n=0;n<128;n++)OLED_WR_Byte(OLED_GRAM[n][i],OLED_DATA); 
	}   
}
#if OLED_MODE==1	//8080并口
//通过拼凑的方法向OLED输出一个8位数据
//data:要输出的数据
void OLED_Data_Out(u8 data)
{
	u16 dat=data&0X0F;
	GPIOC->ODR&=~(0XF<<6);		//清空6~9
	GPIOC->ODR|=dat<<6;			//D[3:0]-->PC[9:6]
    
    GPIOC->ODR&=~(0X1<<11);		//清空11
    GPIOC->ODR|=((data>>4)&0x01)<<11;
    
    GPIOD->ODR&=~(0X1<<3);		//清空3
    GPIOD->ODR|=((data>>5)&0x01)<<3;
    
    GPIOB->ODR&=~(0X3<<8);		//清空8,9
    GPIOB->ODR|=((data>>6)&0x01)<<8;
    GPIOB->ODR|=((data>>7)&0x01)<<9;
} 
//向SSD1306写入一个字节。
//dat:要写入的数据/命令
//cmd:数据/命令标志 0,表示命令;1,表示数据;
void OLED_WR_Byte(u8 dat,u8 cmd)
{
	OLED_Data_Out(dat);	
 	OLED_RS=cmd;
	OLED_CS(0);	
	OLED_WR(0);	  
	OLED_WR(1);   
	OLED_CS(1);   
	OLED_RS(1);   
} 	    	    
#else
//向SSD1306写入一个字节。
//dat:要写入的数据/命令
//cmd:数据/命令标志 0,表示命令;1,表示数据;
void OLED_WR_Byte(u8 dat,u8 cmd)
{	
	u8 i;			  
	OLED_RS(cmd); //写命令 
	OLED_CS(0);	
	for(i=0;i<8;i++)
	{			  
		OLED_SCLK(0);
		if(dat&0x80)OLED_SDIN(1);
		else OLED_SDIN(0);
		OLED_SCLK(1);
		dat<<=1;   
	}		
	OLED_CS(1);		  
	OLED_RS(1);   	  
} 
#endif
	  	  
//开启OLED显示    
void OLED_Display_On(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X14,OLED_CMD);  //DCDC ON
	OLED_WR_Byte(0XAF,OLED_CMD);  //DISPLAY ON
}
//关闭OLED显示     
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X10,OLED_CMD);  //DCDC OFF
	OLED_WR_Byte(0XAE,OLED_CMD);  //DISPLAY OFF
}		   			 
//清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!	  
void OLED_Clear(void)  
{  
	u8 i,n;  
	for(i=0;i<8;i++)for(n=0;n<128;n++)OLED_GRAM[n][i]=0X00;  
	OLED_Refresh_Gram();//更新显示
}
//画点 
//x:0~127
//y:0~63
//t:1 填充 0,清空				   
void OLED_DrawPoint(u8 x,u8 y,u8 t)
{
	u8 pos,bx,temp=0;
	if(x>127||y>63)return;//超出范围了.
	pos=7-y/8;
	bx=y%8;
	temp=1<<(7-bx);
	if(t)OLED_GRAM[x][pos]|=temp;
	else OLED_GRAM[x][pos]&=~temp;	    
}
//x1,y1,x2,y2 填充区域的对角坐标
//确保x1<=x2;y1<=y2 0<=x1<=127 0<=y1<=63	 	 
//dot:0,清空;1,填充	  
void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot)  
{  
	u8 x,y;  
	for(x=x1;x<=x2;x++)
	{
		for(y=y1;y<=y2;y++)OLED_DrawPoint(x,y,dot);
	}													    
	OLED_Refresh_Gram();//更新显示
}
//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示				 
//size:选择字体 12/16/24
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size,u8 mode)
{      			    
	u8 temp,t,t1;
	u8 y0=y;
	u8 csize=(size/8+((size%8)?1:0))*(size/2);		//得到字体一个字符对应点阵集所占的字节数
	chr=chr-' ';//得到偏移后的值		 
    for(t=0;t<csize;t++)
    {   
		if(size==12)temp=asc2_1206[chr][t]; 	 	//调用1206字体
		else if(size==16)temp=asc2_1608[chr][t];	//调用1608字体
		else if(size==24)temp=asc2_2412[chr][t];	//调用2412字体
		else return;								//没有的字库
        for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp<<=1;
			y++;
			if((y-y0)==size)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
    }          
}
//m^n函数
u32 mypow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}				  
//显示2个数字
//x,y :起点坐标	 
//len :数字的位数
//size:字体大小
//mode:模式	0,填充模式;1,叠加模式
//num:数值(0~4294967295);	 		  
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size)
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size/2)*t,y,' ',size,1);
				continue;
			}else enshow=1; 
		 	 
		}
	 	OLED_ShowChar(x+(size/2)*t,y,temp+'0',size,1); 
	}
} 
//显示字符串
//x,y:起点坐标  
//size:字体大小 
//*p:字符串起始地址 
void OLED_ShowString(u8 x,u8 y,const u8 *p,u8 size)
{	
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {       
        if(x>(128-(size/2))){x=0;y+=size;}
        if(y>(64-size)){y=x=0;OLED_Clear();}
        OLED_ShowChar(x,y,*p,size,1);	 
        x+=size/2;
        p++;
    }  
	
}	

//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示				 
//size:选择字体 12/16/24
//buffer :调用的那个字库

void OLED_ShowChineseChar(u8 x,u8 y,u8 chr,u8 size,u8 mode ,int P_buffer)
{      			    
	u8 temp,t,t1;
	u8 y0=y;
	u8 csize=size*2;		//得到字体一个字符对应点阵集所占的字节数
	u8 (*p_buffer)[csize];
	
	p_buffer=(u8(*)[size*2])P_buffer;
    for(t=0;t<csize;t++)
    {   
		temp=p_buffer[chr][t];
        for(t1=0;t1<8;t1++)
		{
			if(temp&0x80)OLED_DrawPoint(x,y,mode);
			else OLED_DrawPoint(x,y,!mode);
			temp<<=1;
			y++;
			if((y-y0)==size)
			{
				y=y0;
				x++;
				break;
			}
		}  	 
    }          
}
/*
显示汉字字符串
//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
len：要显示的字符串长度
//mode:0,反白显示;1,正常显示				 
//size:选择字体 12/16/24
//buffer :调用的那个字库

*/
void  OLED_ShowChineseString(u8 x,u8 y,u8 len,u8 size,u8 mode ,int P_buffer)
{
	u8 num=0;
	for(num =0;num<len;num++)
	{
		OLED_ShowChineseChar(x+(size*num),y,num,size,mode ,P_buffer);
	}

} 
//初始化SSD1306					    
void OLED_Init(void)
{ 	 		 
    GPIO_InitTypeDef  GPIO_Initure;
	
    __HAL_RCC_GPIOA_CLK_ENABLE();   //使能GPIOA时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();   //使能GPIOB时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();   //使能GPIOC时钟
    __HAL_RCC_GPIOD_CLK_ENABLE();   //使能GPIOD时钟
    __HAL_RCC_GPIOF_CLK_ENABLE();   //使能GPIOF时钟
    __HAL_RCC_GPIOH_CLK_ENABLE();   //使能GPIOH时钟
#if OLED_MODE==1		//使用8080并口模式		
{
	
	//GPIO初始化设置      
    GPIO_Initure.Pin=GPIO_PIN_15;         //PA15
    GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;//推挽输出
    GPIO_Initure.Pull=GPIO_PULLUP;        //上拉
    GPIO_Initure.Speed=GPIO_SPEED_HIGH;   //高速
    HAL_GPIO_Init(GPIOA,&GPIO_Initure);   //初始化
	
    //PB3,4,7,8,9
    GPIO_Initure.Pin=GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;	
	HAL_GPIO_Init(GPIOB,&GPIO_Initure);//初始化

    //PC6,7,8,9,11
    GPIO_Initure.Pin=GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_11;	
	HAL_GPIO_Init(GPIOC,&GPIO_Initure);//初始化	
  
    //PD3
	GPIO_Initure.Pin=GPIO_PIN_3;	
	HAL_GPIO_Init(GPIOD,&GPIO_Initure);//初始化	
	
    //PH8
	GPIO_Initure.Pin=GPIO_PIN_8;	
	HAL_GPIO_Init(GPIOH,&GPIO_Initure);//初始化	
	
 	OLED_WR(1);
	OLED_RD(1); 
}
#else					//使用4线SPI 串口模式
	#if OLED_DEVELOPMENT_BOARD
	{
		//GPIO初始化设置      
		GPIO_Initure.Pin=GPIO_PIN_15;         //PA15
		GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;//推挽输出
		GPIO_Initure.Pull=GPIO_PULLUP;        //上拉
		GPIO_Initure.Speed=GPIO_SPEED_FAST;   //高速
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);   //初始化
		
		//PB4,7
		GPIO_Initure.Pin=GPIO_PIN_4|GPIO_PIN_7;	
		HAL_GPIO_Init(GPIOB,&GPIO_Initure);//初始化

		//PC6,7
		GPIO_Initure.Pin=GPIO_PIN_6|GPIO_PIN_7;	
		HAL_GPIO_Init(GPIOC,&GPIO_Initure);//初始化	
	}
	#else  
	{
			//GPIO初始化设置      
		GPIO_Initure.Pin=GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_7|GPIO_PIN_6;    
		GPIO_Initure.Mode=GPIO_MODE_OUTPUT_PP;//推挽输出
		GPIO_Initure.Pull=GPIO_PULLUP;        //上拉
		GPIO_Initure.Speed=GPIO_SPEED_FAST;   //高速
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);   //初始化
		
		GPIO_Initure.Pin=GPIO_PIN_14|GPIO_PIN_7;	  
		HAL_GPIO_Init(GPIOB,&GPIO_Initure);//初始化
		//OLED_PH_EN
		GPIO_Initure.Pin=GPIO_PIN_8;	
		HAL_GPIO_Init(GPIOA,&GPIO_Initure);//初始化
		OLED_PH_EN(1);
	}
#endif
	
	OLED_SDIN(1);
	OLED_SCLK(1);
#endif
	OLED_CS(1);
	OLED_RS(1);
	
	OLED_RST(1); 
	OLED_RST(0);
	delay_ms(200);
	OLED_RST(1); 
	delay_ms(200);
#if OLED_DEVELOPMENT_BOARD				  
	OLED_WR_Byte(0xAE,OLED_CMD);
	OLED_WR_Byte(0xD5,OLED_CMD);
	OLED_WR_Byte(0x80,OLED_CMD);
	OLED_WR_Byte(0xA8,OLED_CMD);
	OLED_WR_Byte(0X3F,OLED_CMD);
	OLED_WR_Byte(0xD3,OLED_CMD);
	OLED_WR_Byte(0X00,OLED_CMD);
	OLED_WR_Byte(0x40,OLED_CMD);
	OLED_WR_Byte(0x8D,OLED_CMD);
	OLED_WR_Byte(0x14,OLED_CMD);
	OLED_WR_Byte(0x20,OLED_CMD);
	OLED_WR_Byte(0x02,OLED_CMD);
	OLED_WR_Byte(0xA1,OLED_CMD);
	OLED_WR_Byte(0xC0,OLED_CMD);
	OLED_WR_Byte(0xDA,OLED_CMD);
	OLED_WR_Byte(0x12,OLED_CMD);
	OLED_WR_Byte(0x81,OLED_CMD);
	OLED_WR_Byte(0xEF,OLED_CMD);
	OLED_WR_Byte(0xD9,OLED_CMD);
	OLED_WR_Byte(0xf1,OLED_CMD);
	OLED_WR_Byte(0xDB,OLED_CMD);
	OLED_WR_Byte(0x30,OLED_CMD);
	OLED_WR_Byte(0xA4,OLED_CMD);
	OLED_WR_Byte(0xA6,OLED_CMD);
	OLED_WR_Byte(0xAF,OLED_CMD);
#else
	OLED_WR_Byte(0xAE,OLED_CMD);
	OLED_WR_Byte(0xD5,OLED_CMD);
	OLED_WR_Byte(0x80,OLED_CMD);
	OLED_WR_Byte(0xA8,OLED_CMD);
	OLED_WR_Byte(0X3F,OLED_CMD);
	OLED_WR_Byte(0xD3,OLED_CMD);
	OLED_WR_Byte(0X00,OLED_CMD);
	OLED_WR_Byte(0x40,OLED_CMD);
	OLED_WR_Byte(0x8D,OLED_CMD);
	OLED_WR_Byte(0x14,OLED_CMD);
	OLED_WR_Byte(0x20,OLED_CMD);
	OLED_WR_Byte(0x02,OLED_CMD);
	OLED_WR_Byte(0xA1,OLED_CMD);
	OLED_WR_Byte(0xC0,OLED_CMD);
	OLED_WR_Byte(0xDA,OLED_CMD);
	OLED_WR_Byte(0x12,OLED_CMD);
	OLED_WR_Byte(0x81,OLED_CMD);
	OLED_WR_Byte(0xEF,OLED_CMD);
	OLED_WR_Byte(0xD9,OLED_CMD);
	OLED_WR_Byte(0xf1,OLED_CMD);
	OLED_WR_Byte(0xDB,OLED_CMD);
	OLED_WR_Byte(0x30,OLED_CMD);
	OLED_WR_Byte(0xA4,OLED_CMD);
	OLED_WR_Byte(0xA6,OLED_CMD);
	OLED_WR_Byte(0xAF,OLED_CMD);
#endif 
	OLED_Clear(); 
}  
