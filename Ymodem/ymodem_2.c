#include "ymodem.h"
#include "ble_update.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"
#include "uart.h"



/**
  * @brief  Send a byte
  * @param  c: Character
  * @retval 0: Byte sent
  */
void SendByte (uint8_t c)
{ 
	BleSendByte(c);  
}

int YmodemRecISR(char Data)
{    
	static unsigned char ucRecByte=0,EscapeFlag=0 ;
	const unsigned char ESCAPE_CH=0x7d,XOR_CH=0x20;
	#define    HEAD_CH  '#'
	#define    TAIL_CH  '@'
		ucRecByte=Data; 
		if((ucRecByte==ESCAPE_CH)&&(EscapeFlag==RESET))//接到转义字符
		{
			EscapeFlag=SET;  //设置标记
			return -1;
		}
		else if(EscapeFlag==SET) 
		{ 
			ucRecByte=ucRecByte^XOR_CH; //  取转义后字符
			EscapeFlag=RESET; 
		} 
		if(RecStartFlag==0)
		{ 
			switch (ucRecByte)
			{
				case SOH:
//					if(sta==1) return -1;
//					sta=1;
					packet_length = PACKET_SIZE;
					packet_data[0] = ucRecByte;
					RecStartFlag=1;
					//UART5_RX_GOT_FRAME_FLAG=0;
					Datalen=1;
					return -1;
				case STX:
					packet_length = PACKET_1K_SIZE;
					packet_data[0] = ucRecByte;
					RecStartFlag=1;
					Datalen=1;
					return -1;
				case EOT:
					packet_length=0; 
					UART5_RX_GOT_FRAME_FLAG=1; 
					return 0;
				case CA:
					 return -1;
				case ABORT1:
					return -1;
				case ABORT2:
					return 1;
				default:
					return -1;
			}
		} 
		if(Datalen<(packet_length +5)-1)
		{
			packet_data[Datalen++]=ucRecByte;
		}
		else 
		{ 
			packet_data[Datalen++]=ucRecByte;
			UART5_RX_GOT_FRAME_FLAG=1;
		}
		return 0; 
}



