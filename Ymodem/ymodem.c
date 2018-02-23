/**
  ******************************************************************************
  * @file    IAP/src/ymodem.c 
  * @author  MCD Application Team
  * @version V3.3.0
  * @date    10/15/2010
  * @brief   This file provides all the software functions related to the ymodem 
  *          protocol.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */

/** @addtogroup IAP
  * @{
  */ 
  
/* Includes ------------------------------------------------------------------*/
 
#include "ymodem.h"
#include "ble_update.h"
#include "stdlib.h"
#include "string.h"
#include "delay.h"
#include "uart.h"
#include "bootloader.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
 
uint8_t file_name[FILE_NAME_LENGTH];
uint32_t FlashDestination = 0x50000; /* Flash user program offset */
uint16_t PageSize = 128;
int32_t EraseCounter = 0x0,packet_length=0;
uint32_t NbrOfPage = 0;
int FLASHStatus = 0;
uint32_t RamSource; 

uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD]={0};
unsigned short Datalen=0;
unsigned char RecStartFlag=0;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size);
/**
  * @brief  Receive byte from sender
  * @param  c: Character
  * @param  timeout: Timeout
  * @retval 0: Byte received
  *         -1: Timeout
  */
static  int32_t Receive_Byte (uint8_t *c, unsigned short len ,uint32_t timeout)
{
	#if 1
	//char temp[2]={0};
  //if(BleReceviceByte((char *)c,timeout)==0) return 0;
	//temp[0]=*c;
	//BlePrintf(temp);..
	
	HAL_UART_Receive(&gstUart5.UartHandle, c, len, NAK_TIMEOUT);
	
  return 0;
	#else
	while(timeout--)
	{
		if(UART5_RX_GOT_BYTE_FLAG==SET)
		{ 
			//HAL_UART_Receive_IT(&gstUart5.UartHandle,(uint8_t *)&gstUart5.ucRecByte,1);  
			*c=gstUart5.ucRecByte;
			UART5_RX_GOT_BYTE_FLAG=RESET;
			return 0;
		}
	}
	
	return -1;
	#endif
}

/**
  * @brief  Send a byte
  * @param  c: Character
  * @retval 0: Byte sent
  */
static uint32_t Send_Byte (uint8_t c)
{ 
	BleSendByte(c); 
  return 0;
}

/**
  * @brief  Receive a packet from sender
  * @param  data
  * @param  length
  * @param  timeout
  *     0: end of transmission
  *    -1: abort by sender
  *    >0: packet length
  * @retval 0: normally return
  *        -1: timeout or packet error
  *         1: abort by user
  */
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
		while(timeout--)
		{
		 if(UART5_RX_GOT_FRAME_FLAG==SET)
		 {
			 
			
			 if (packet_data[PACKET_SEQNO_INDEX] != ((packet_data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
			 {
					 Datalen=0;
					 memset(packet_data,0,sizeof(packet_data));
					 UART5_RX_GOT_FRAME_FLAG=0;
					 RecStartFlag=0; 
					if(data)
					{
						return -9;
					}
					else return -1;
			 }
			 Datalen=0;
			 RecStartFlag=0; 
			 UART5_RX_GOT_FRAME_FLAG=RESET;
			 return 0;
		 }
		} 
		if(RecStartFlag)
		{ 
			if(data)
			{
				Datalen=0;
				memset(packet_data,0,sizeof(packet_data));
				UART5_RX_GOT_FRAME_FLAG=0;
				RecStartFlag=0;
				return -9;
			}
			else return -1;
		}
		else 
		{ 
		 
		} 
		Datalen=0;
		memset(packet_data,0,sizeof(packet_data));
		UART5_RX_GOT_FRAME_FLAG=0;
		RecStartFlag=0;
		return -1;
}

/**
  * @brief  Receive a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
int32_t Ymodem_Receive (uint8_t *buf)
{
  uint8_t   file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
  int32_t i,     session_done, file_done, packets_received, errors, session_begin, size = 0;
  int16_t crc16,datacrc16;
//	uint8_t nakflag=0;
	uint8_t begin=0;
  /* Initialize FlashDestination variable */ 
	Send_Byte(MODE_C);
  for (session_done = 0, errors = 0, session_begin = 0; ;)
  {
    for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)
    {
			btIWDG_Refresh();
			if(session_begin==1)  begin=1;
      switch (Receive_Packet( &begin, &packet_length, NAK_TIMEOUT))
      {
        case 0:
          errors = 0;
          switch (packet_length)
          {
            /* Abort by sender */
            case - 1:
              Send_Byte(ACK);
              return 0;
            /* End of transmission */
            case 0:
              Send_Byte(ACK); 
							Send_Byte(MODE_C);
              break;
            /* Normal packet */
            default:
              if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
              {
//								if(errors>20) //错误次数太多退出
//								{
//									Send_Byte(CA);
//									return 0;
//								}
								if(packet_data[PACKET_SEQNO_INDEX]==0)
								{
									Send_Byte(ACK);
									file_done=1;
									return size;
								}
                Send_Byte(NAK);
								errors++; 
              }
              else
              {
                if (packets_received == 0)
                { 
									crc16=Cal_CRC16((uint8_t *)(packet_data+PACKET_HEADER),packet_length);//对数据进行CRC计算
									datacrc16=( packet_data[packet_length+PACKET_HEADER]<<8)+packet_data[packet_length+PACKET_HEADER+1];//取crc值
									if(datacrc16!=crc16)                         //对比crc
									{  
										Send_Byte(NAK); 
										break;
									}  
                  /* Filename packet */
                  if (packet_data[PACKET_HEADER] != 0)
                  {
                    /* Filename packet has valid data */
                    for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
                    {
                      file_name[i++] = *file_ptr++;
                    }
                    file_name[i++] = '\0';
                    for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
                    {
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
                    size=atoi((char *)file_size); 
										memset(packet_data,0,sizeof(packet_data));
                    Send_Byte(ACK);
                    Send_Byte(MODE_C);
										UART5_RX_GOT_BYTE_FLAG=RESET; 
                  }
                  /* Filename packet is empty, end session */
                  else
                  { 
										Send_Byte(ACK); 
                    file_done = 1;
                    session_done = 1; 
										UART5_RX_GOT_BYTE_FLAG=RESET;
                  }
                }
                else
                {
                  memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);
                  buf_ptr+=packet_length; 
                  Send_Byte(ACK); 
									UART5_RX_GOT_BYTE_FLAG=RESET;									
                }
                packets_received ++;
                session_begin = 1;
              }
          }
          break;
        case 1:
          Send_Byte(CA);
          Send_Byte(CA);
          return -3;
				case -9:
					if (session_begin > 0)
          {
            errors ++;  
						Send_Byte(NAK);  
          }  
					else 
					{
						Send_Byte(MODE_C);
					}
          if (errors > MAX_ERRORS)
          {
            Send_Byte(CA);
            Send_Byte(CA);
            return 0;
          } 
					break;
        default:
          if (session_begin > 0)
          {
            errors ++; 
          }  
					else 
					{
						Send_Byte(MODE_C);
					} 
          if (errors > MAX_ERRORS)
          {
            Send_Byte(NAK); 
          }
          
          break;
      }
      if (file_done != 0)
      {
        break;
      }
    }
    if (session_done != 0)
    {
      break;
    }
  } 
  return (int32_t)size;
}

/**
  * @brief  check response using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
int32_t Ymodem_CheckResponse(uint8_t c)
{
  return 0;
}

/**
  * @brief  Prepare the first block
  * @param  timeout
  *     0: end of transmission
  */
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
  uint16_t i, j;
  uint8_t file_ptr[10];
  
  /* Make first three packet */
  data[0] = SOH;
  data[1] = 0x00;
  data[2] = 0xff;
  
  /* Filename packet has valid data */
  for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
  {
     data[i + PACKET_HEADER] = fileName[i];
  }

  data[i + PACKET_HEADER] = 0x00;
  sprintf((char *)file_ptr,"%d",*length);
  for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
  {
     data[i++] = file_ptr[j++];
  }
  
  for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
  {
    data[j] = 0;
  }
}

/**
  * @brief  Prepare the data packet
  * @param  timeout
  *     0: end of transmission
  */
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
  uint16_t i, size, packetSize;
  uint8_t* file_ptr;
  
  /* Make first three packet */
  packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
  size = sizeBlk < packetSize ? sizeBlk :packetSize;
  if (packetSize == PACKET_1K_SIZE)
  {
     data[0] = STX;
  }
  else
  {
     data[0] = SOH;
  }
  data[1] = pktNo;
  data[2] = (~pktNo);
  file_ptr = SourceBuf;
  
  /* Filename packet has valid data */
  for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
  {
     data[i] = *file_ptr++;
  }
  if ( size  <= packetSize)
  {
    for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
    {
      data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
    }
  }
}

/**
  * @brief  Update CRC16 for input byte
  * @param  CRC input value 
  * @param  input byte
   * @retval None
  */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
 uint32_t crc = crcIn;
 uint32_t in = byte|0x100;
 do
 {
 crc <<= 1;
 in <<= 1;
 if(in&0x100)
 ++crc;
 if(crc&0x10000)
 crc ^= 0x1021;
 }
 while(!(in&0x10000));
 return crc&0xffffu;
}


/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
   * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
 uint32_t crc = 0;
 const uint8_t* dataEnd = data+size;
 while(data<dataEnd)
  crc = UpdateCRC16(crc,*data++);
 
 crc = UpdateCRC16(crc,0);
 crc = UpdateCRC16(crc,0);
 return crc&0xffffu;
}

/**
  * @brief  Cal Check sum for YModem Packet
  * @param  data
  * @param  length
   * @retval None
  */
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
 uint32_t sum = 0;
 const uint8_t* dataEnd = data+size;
 while(data < dataEnd )
   sum += *data++;
 return sum&0xffu;
}

/**
  * @brief  Transmit a data packet using the ymodem protocol
  * @param  data
  * @param  length
   * @retval None
  */
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
  uint16_t i;
  i = 0;
  while (i < length)
  {
    Send_Byte(data[i]);
    i++;
  }
}

/**
  * @brief  Transmit a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{
  
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
  uint8_t FileName[FILE_NAME_LENGTH];
  uint8_t *buf_ptr, tempCheckSum ;
  uint16_t tempCRC, blkNumber;
  uint8_t receivedC[2], CRC16_F = 0, i;
  uint32_t errors, ackReceived, size = 0, pktSize;

  errors = 0;
  ackReceived = 0;
  for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
  {
    FileName[i] = sendFileName[i];
  }
  CRC16_F = 1;       
    
  /* Prepare first block */
  Ymodem_PrepareIntialPacket(&packet_data[0], FileName, &sizeFile);
  
  do 
  {
    /* Send Packet */
     Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
    /* Send CRC or Check Sum based on CRC16_F */
    if (CRC16_F)
    {
       tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCRC >> 8);
       Send_Byte(tempCRC & 0xFF);
    }
    else
    {
       tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCheckSum);
    } 
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0],1,10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* Packet transfered correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
  }while (!ackReceived);// && (errors < 0x0A));
  
  if (errors >=  0x0A)
  {
    return errors;
  }
  buf_ptr = buf;
  size = sizeFile;
  blkNumber = 0x01;
  /* Here 1024 bytes package is used to send the packets */
  
  
  /* Resend packet if NAK  for a count of 10 else end of commuincation */
  while (size)
  {
    /* Prepare next packet */
    Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
    ackReceived = 0;
    receivedC[0]= 0;
    errors = 0;
    do
    {
      /* Send next packet */
      if (size >= PACKET_1K_SIZE)
      {
        pktSize = PACKET_1K_SIZE;
       
      }
      else
      {
        pktSize = PACKET_SIZE;
      }
      Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
      /* Send CRC or Check Sum based on CRC16_F */
      /* Send CRC or Check Sum based on CRC16_F */
      if (CRC16_F)
      {
         tempCRC = Cal_CRC16(&packet_data[3], pktSize);
         Send_Byte(tempCRC >> 8);
         Send_Byte(tempCRC & 0xFF);
      }
      else
      {
        tempCheckSum = CalChecksum (&packet_data[3], pktSize);
        Send_Byte(tempCheckSum);
      }
      
      /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0],1, 100000) == 0)  && (receivedC[0] == ACK))
      {
        ackReceived = 1;  
        if (size > pktSize)
        {
           buf_ptr += pktSize;  
           size -= pktSize;
//           if (blkNumber == (FLASH_IMAGE_SIZE/1024))
//           {
//             return 0xFF; /*  error */
//           }
//           else
           {
              blkNumber++;
           }
        }
        else
        {
          buf_ptr += pktSize;
          size = 0;
        }
      }
      else
      {
        errors++;
      }
    }while(!ackReceived && (errors < 0x0A));
    /* Resend packet if NAK  for a count of 10 else end of commuincation */
    
    if (errors >=  0x0A)
    {
      return errors;
    }
    
  }
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;
  do 
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0],1 ,10000) == 0)  && receivedC[0] == ACK)
      {
        ackReceived = 1;  
      }
      else
      {
        errors++;
      }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
  
  /* Last packet preparation */
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;

  packet_data[0] = SOH;
  packet_data[1] = 0;
  packet_data [2] = 0xFF;

  for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
  {
     packet_data [i] = 0x00;
  }
  
  do 
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
    /* Send CRC or Check Sum based on CRC16_F */
    tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
    Send_Byte(tempCRC >> 8);
    Send_Byte(tempCRC & 0xFF);
  
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0],1 ,10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* Packet transfered correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
 
  }while (!ackReceived && (errors < 0x0A));
  /* Resend packet if NAK  for a count of 10  else end of commuincation */
  if (errors >=  0x0A)
  {
    return errors;
  }  
  
  do 
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0],1, 10000) == 0)  && receivedC[0] == ACK)
      {
        ackReceived = 1;  
      }
      else
      {
        errors++;
      }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
	
  return 0; /* file trasmitted successfully */
}

/**
  * @}
  */

/*******************(C)COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
