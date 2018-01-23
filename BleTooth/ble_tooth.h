#ifndef __AT_BLUETOOTH_H__
#define __AT_BLUETOOTH_H__ 

//命令回显结束符
/*
0a------换行符号－－－－－－"\n"
0d------回车符号－－－－－－"\r"
0D 0A 
*/
//#define BL_SEND_COMMAND_ECHO_1	'\r'
//#define BL_SEND_COMMAND_ECHO_2	'\n'

////命令符号 equal
//#define	BL_EQUAL	"="
//#define	BL_COMMAND_END	"\0"
//模式切换命令
#define	BL_MODE		"+++"			//模式切换指令
#define	BL_ADVON	"AT+ADVON"		//打开模块广播功能
#define	BL_ADVOFF	"AT+ADVOFF"		//关闭模块广播功能
#define	BL_DISCONN	"AT+DISCONN"	//断开已连接的设备
#define	BL_RESET	"AT+RESET"		//复位指令
#define	BL_FACTORY	"AT+FACTORY"	//恢复出厂设置
//只做查询的命令
#define	BL_VER		"AT+VER"		//查询版本号
#define	BL_MAC		"AT+MAC"		//查询模块 MAC地址
#define	BL_SCAN		"AT+SCAN"		//扫描当前在广播的设备
//查询和设置命令
#define	BL_NAME		"AT+NAME"		//查询设置模块 NAME
#define	BL_UART		"AT+UART"		//查询设置串口波特率
#define	BL_ROLE		"AT+ROLE"		//查询设置模块角色
#define	BL_TXPWR	"AT+TXPWR"		//查询设置模块发射功率
#define	BL_UUID		"AT+UUID"		//查询设置设备 UUID
#define	BL_ADVDATA	"AT+ADVDATA"	//查询设置广播数据


#define	BL_ADVIN	"AT+ADVIN"		//查询设置模块广播间隙
#define	BL_CONIN	"AT+CONIN"		//查询设置模块连接间隙

//不常用命令
#define	BL_CONNECT	"AT+CONNECT"	//指定连接设备
#define	BL_IBEACON	"AT+IBEACON"	//查询设置 IBEACON 数据
#define	BL_RSSI		"AT+RSSI"		//获取已连接设备 RSSI
//组合字符
#define BLE_CMD_EQUAL_STR          "="
#define BLE_CMD_END_ECHO           "\r\n"
#define BLE_UART_DEFAULT_SET_STR   ",8,1,0"
#define BLE_TXPWR_END_ECHO           "dbm"
//比较字符串
#define BLE_ACK_OK_ECHO         "OK"
#define BLE_ACK_SET_OK_ECHO     "PARA SET:"
#define BLE_CMD_IN              "CMD IN"
#define BLE_CMD_OUT             "CMD OUT"
#define BLE_ACK_DATA_STOP_ECHO_0             ','
//接收数据处理偏移
#define BLE_RX_UART_BAUD_OFFSET       12

//蓝牙默认参数
#define BLE_DEFAULT_BAUD           115200
#define BLE_PENETRATE_MODE_BAUD     19200
//时间定义
#define BLE_CMD_TRY_TIMES          3
enum ble_mode
{
	BLE_PENETRATE_TRANS_MODE=0,
	BLE_CMD_MODE=2,
	BLE_CMD_MODE_NONE=0xff,
};
typedef enum ble_send_cmd_mod
{
	BLE_CMD_READ,
	BLE_CMD_SET
}eBleSendModeType; 
typedef enum ble_err
{
	BLE_NO_ERR,
	BLE_CMD_ERR,
	BLE_NACK,
	BLE_BAUD_ERR,
	BLE_TRY_ERR,
	
}eBleErrorType;
typedef enum ble_cmd
{
	cBL_MODE,
	cBL_ADVON,
	cBL_ADVOFF,
	cBL_DISCONN,
	cBL_RESET,
	cBL_FACTORY,
	cBL_VER,
	cBL_MAC,
	cBL_SCAN,
	cBL_NAME,
	cBL_UART,
	cBL_ROLE,
	cBL_TXPWR,
	cBL_UUID,
	cBL_ADVDATA,
	cBL_ADVIN,
	cBL_CONIN,
	cBL_CONNECT,
	cBL_IBEACON,
	cBL_RSSI,
	
}eBleCmdType;
typedef enum ble_delay_time
{
	tBL_MODE=1000,
	tBL_ADVON=1000,
	tBL_ADVOFF=1000,
	tBL_DISCONN=1000,
	tBL_RESET=1000,
	tBL_FACTORY=1000,
	tBL_VER=1000,
	tBL_MAC=1000,
	tBL_SCAN=1000,
	tBL_NAME=1000,
	tBL_UART=1000,
	tBL_ROLE=1000,
	tBL_TXPWR=1000,
	tBL_UUID=1000,
	tBL_ADVDATA=1000,
	tBL_ADVIN=1000,
	tBL_CONIN,
	tBL_CONNECT=1000,
	tBL_IBEACON=1000,
	tBL_RSSI=1000,
}eBleDelayTimeType;

eBleErrorType BleToothSendCmd(char *CmdHeadStr,eBleSendModeType eMode,char *Param); 
#endif

