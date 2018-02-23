#include "stdint.h"
#include "ble_tooth.h"
#include "string.h"
#include "ble_update.h"
#include "delay.h"
#include "ble_update.h"
#include "delay.h"
#include "bootloader.h"
#include "stdlib.h"

#define  BlSendCommand          BleSendStr
static eBleErrorType eCombinCommand(char *CmdHeadStr,eBleSendModeType eMode,char *Param,char *sendstr,eBleCmdType *eCmd,eBleDelayTimeType *eDelayTime);
static eBleErrorType eAnalyzeBleAck(eBleCmdType eCmd);
//命令发送应答数据处理函数
static eBleErrorType eBL_MODE_Analyze(void);
static eBleErrorType eBL_NAME_Analyze(void);
static eBleErrorType eBL_UART_Analyze(void);


#define CMD_BUF_LEN      50
/**
  * @brief  蓝牙发送命令函数
  * @note   向蓝牙模块发送命令，处理蓝牙应答数据
  * @param  CmdHeadStr: 发送命令头字符串指针 如："AT+NAME"
  * @param  eMode: 发送命令模式
  * @param  Param: 发送命令附带的参数
  * @retval 蓝牙错误码
  */
eBleErrorType BleToothSendCmd(char *CmdHeadStr,eBleSendModeType eMode,char *Param)
{
	eBleErrorType res;
	eBleCmdType eCmd;//命令
	eBleDelayTimeType eDelayTime;//取数据延时时间
	char cmdbuf[CMD_BUF_LEN]={0}; 
	res=eCombinCommand(CmdHeadStr,eMode,Param,cmdbuf,&eCmd,&eDelayTime); //分析组合命令 获取延时时间及命令枚举
	BleCmdNormalRxMode_Enter();	 //串口接收进入蓝牙发送命令模式
	BleRxBufferClear();      //清空数据
	BlSendCommand(cmdbuf);      //发送命令
	BleDelay_ms(eDelayTime); //接收数据到延时结束
	res=eAnalyzeBleAck(eCmd);//处理蓝牙应答数据
	BleCmdNormalRxMode_Exit();     //串口接收退出蓝牙发送命令模式
	BleRxBufferClear();      //清空数据
	return res;
}

/**
  * @brief  蓝牙模块强度设置应答处理函数 CC78ABA12893
  * @retval 蓝牙错误码
  */
static eBleErrorType eBL_TXPWR_Analyze(void)
{ 
	if(BleRxBufferFindStr(BLE_ACK_OK_ECHO))
	{
		return BLE_NO_ERR;
	}
	else if(BleRxBufferFindStr(BLE_ACK_SET_OK_ECHO))
	{
		return BLE_NO_ERR;
	}
	return BLE_NACK;
}
/**
  * @brief  蓝牙模块角色设置应答处理函数 
  * @retval 蓝牙错误码
  */
static eBleErrorType eBL_ROLE_Analyze(void)
{ 
	if(BleRxBufferFindStr(BLE_ACK_OK_ECHO))
	{
		return BLE_NO_ERR;
	}
	else if(BleRxBufferFindStr(BLE_ACK_SET_OK_ECHO))
	{
		return BLE_NO_ERR;
	}
	return BLE_NACK;
}
/**
  * @brief  蓝牙模块波特率设置应答处理函数 
  * @retval 蓝牙错误码
  */
static eBleErrorType eBL_UART_Analyze(void)
{
	unsigned int ucPos=0;
	if(BleRxBufferFindStr(BLE_ACK_OK_ECHO))
	{
		ucPos=bytepos((char*)&gBleRxBuffer[BLE_RX_UART_BAUD_OFFSET],BLE_ACK_DATA_STOP_ECHO_0);
		gBleRxBuffer[BLE_RX_UART_BAUD_OFFSET+ucPos]=0;
		gBleBaud=atoi((void*)&gBleRxBuffer[BLE_RX_UART_BAUD_OFFSET]);
		return BLE_NO_ERR;
	}
	else if(BleRxBufferFindStr(BLE_ACK_SET_OK_ECHO))
	{ 
		gBleBaud=atoi((void*)&gBleRxBuffer[BLE_RX_UART_BAUD_OFFSET]);
		return BLE_NO_ERR;
	}
	return BLE_NACK;
}

/**
  * @brief  蓝牙模块名字设置应答处理函数 
  * @retval 蓝牙错误码
  */
static eBleErrorType eBL_NAME_Analyze(void)
{
	if(BleRxBufferFindStr(BLE_ACK_OK_ECHO))
	{
		
		return BLE_NO_ERR;
	}
	else if(BleRxBufferFindStr(BLE_ACK_SET_OK_ECHO))
	{ 
		return BLE_NO_ERR;
	}
	return BLE_NACK;
}
/**
  * @brief  蓝牙模式切换应答处理函数 
  * @retval 蓝牙错误码
  */
static eBleErrorType eBL_MODE_Analyze(void)
{
	if(BleRxBufferFindStr(BLE_CMD_IN))
	{
		gBleCurrentMode=BLE_CMD_MODE;
		return BLE_NO_ERR;
	}
	else if(BleRxBufferFindStr(BLE_CMD_OUT))
	{
		gBleCurrentMode=BLE_PENETRATE_TRANS_MODE;
		return BLE_NO_ERR;
	}
	return BLE_NACK;
}
/**
  * @brief  蓝牙处理命令应答数据函数
  * @param  eCmd: 命令枚举指针 
  * @retval 蓝牙错误码
  */
static eBleErrorType eAnalyzeBleAck(eBleCmdType eCmd)
{
	eBleErrorType res=BLE_NO_ERR;
	switch(eCmd)
	{
		case cBL_MODE:
			res=eBL_MODE_Analyze();
			break;
		case cBL_NAME:
			res=eBL_NAME_Analyze();
			break;
		case cBL_UART:
			res=eBL_UART_Analyze();
			break;
		case cBL_ROLE:
			res=eBL_ROLE_Analyze();
		case cBL_TXPWR:
			res=eBL_TXPWR_Analyze();
		default:
			break;
	}
	return res;
}
/**
  * @brief  蓝牙命令组合分析函数
  * @note   根据字符串获取蓝牙命令枚举，组合蓝牙命令，获取接收时间
  * @param  CmdHeadStr: 发送命令头字符串指针 如："AT+NAME"
  * @param  eMode: 发送命令模式
  * @param  Param: 发送命令附带的参数
  * @param  sendstr: 组合后的命令
  * @param  eCmd: 命令枚举指针
  * @param  eDelayTime: 命令延时接收时间指针
  * @retval 蓝牙错误码
  */
eBleErrorType eCombinCommand(char *CmdHeadStr,eBleSendModeType eMode,char *Param,char *sendstr,eBleCmdType *eCmd,eBleDelayTimeType *eDelayTime)
{
	eBleErrorType res=BLE_CMD_ERR;
	char cmdbuf[CMD_BUF_LEN]={0};
	if(strcmp(CmdHeadStr,BL_MODE )==0)          //设置透传或者命令模式
	{
		strcat(sendstr,BL_MODE); 
		*eCmd=cBL_MODE;
		*eDelayTime=tBL_MODE; 
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_ADVON )==0)
	{
		strcat(sendstr,BL_ADVON); 
		*eCmd=cBL_ADVON;
		*eDelayTime=tBL_ADVON;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_ADVOFF )==0)
	{
		strcat(sendstr,BL_ADVOFF);
		*eCmd=cBL_ADVOFF;
		*eDelayTime=tBL_ADVOFF;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_DISCONN )==0)
	{
		strcat(sendstr,BL_DISCONN);
		*eCmd=cBL_DISCONN;
		*eDelayTime=tBL_DISCONN;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_RESET )==0)       //复位指令
	{
		strcat(sendstr,BL_RESET);
		*eCmd=cBL_RESET;
		*eDelayTime=tBL_RESET;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_FACTORY )==0)   
	{
		strcat(sendstr,BL_FACTORY);
		*eCmd=cBL_FACTORY;
		*eDelayTime=tBL_FACTORY;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_VER )==0)
	{
		strcat(sendstr,BL_VER);
		*eCmd=cBL_VER;
		*eDelayTime=tBL_VER;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_MAC )==0)
	{
		strcat(sendstr,BL_MAC);
		*eCmd=cBL_MAC;
		*eDelayTime=tBL_MAC;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_SCAN )==0)
	{
		strcat(sendstr,BL_SCAN);
		*eCmd=cBL_SCAN;
		*eDelayTime=tBL_SCAN;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_NAME )==0)    //设置模块名称
	{
		strcat(sendstr,BL_NAME);
		if(eMode==BLE_CMD_SET)
		{ 
			strcat(sendstr,BLE_CMD_EQUAL_STR); 
			strcat(sendstr,Param);
		}
		*eCmd=cBL_NAME;
		*eDelayTime=tBL_NAME;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_UART )==0)   //设置模块波特率
	{
		strcat(sendstr,BL_UART);
		if(eMode==BLE_CMD_SET)
		{
			strcat(sendstr,BLE_CMD_EQUAL_STR); 
			sprintf(cmdbuf,"%s"BLE_UART_DEFAULT_SET_STR,Param);
			strcat(sendstr,cmdbuf);
		}
		*eCmd=cBL_UART;
		*eDelayTime=tBL_UART;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_ROLE )==0)     //查询设置模块角色
	{
		strcat(sendstr,BL_ROLE);
		if(eMode==BLE_CMD_SET)
		{
			strcat(sendstr,BLE_CMD_EQUAL_STR); 
			strcat(sendstr,Param);
		}
		*eCmd=cBL_ROLE;
		*eDelayTime=tBL_ROLE;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_TXPWR )==0)     //查询设置模块发射功率
	{
		strcat(sendstr,BL_TXPWR);
		if(eMode==BLE_CMD_SET)
		{
			strcat(sendstr,BLE_CMD_EQUAL_STR); 
			strcat(sendstr,Param);
			strcat(sendstr,BLE_TXPWR_END_ECHO);
		}
		*eCmd=cBL_TXPWR;
		*eDelayTime=tBL_TXPWR;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_UUID )==0) //查询设置设备 UUID
	{
		strcat(sendstr,BL_UUID);
		if(eMode==BLE_CMD_SET)
		{
			strcat(sendstr,BLE_CMD_EQUAL_STR); 
			strcat(sendstr,Param);
		}
		*eCmd=cBL_UUID;
		*eDelayTime=tBL_UUID;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_ADVDATA )==0)
	{
		strcat(sendstr,BL_ADVDATA);
		*eCmd=cBL_ADVDATA;
		*eDelayTime=tBL_ADVDATA;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_ADVIN )==0)
	{
		strcat(sendstr,BL_ADVIN);
		*eCmd=cBL_ADVIN;
		*eDelayTime=tBL_ADVIN;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_CONIN )==0)
	{
		strcat(sendstr,BL_CONIN);
		*eCmd=cBL_CONIN;
		*eDelayTime=tBL_CONIN;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_CONNECT )==0)
	{
		strcat(sendstr,BL_CONNECT);
		*eCmd=cBL_CONNECT;
		*eDelayTime=tBL_CONNECT;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_IBEACON )==0)
	{
		strcat(sendstr,BL_IBEACON);
		*eCmd=cBL_IBEACON;
		*eDelayTime=tBL_IBEACON;
		res=BLE_NO_ERR;
	}
	else if(strcmp(CmdHeadStr,BL_RSSI )==0)
	{
		strcat(sendstr,BL_RSSI);
		*eCmd=cBL_RSSI;
		*eDelayTime=tBL_RSSI;
		res=BLE_NO_ERR;
	}  
	return res;
}
