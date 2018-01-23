#ifndef __AT_BLUETOOTH_H__
#define __AT_BLUETOOTH_H__ 

//������Խ�����
/*
0a------���з��ţ�����������"\n"
0d------�س����ţ�����������"\r"
0D 0A 
*/
//#define BL_SEND_COMMAND_ECHO_1	'\r'
//#define BL_SEND_COMMAND_ECHO_2	'\n'

////������� equal
//#define	BL_EQUAL	"="
//#define	BL_COMMAND_END	"\0"
//ģʽ�л�����
#define	BL_MODE		"+++"			//ģʽ�л�ָ��
#define	BL_ADVON	"AT+ADVON"		//��ģ��㲥����
#define	BL_ADVOFF	"AT+ADVOFF"		//�ر�ģ��㲥����
#define	BL_DISCONN	"AT+DISCONN"	//�Ͽ������ӵ��豸
#define	BL_RESET	"AT+RESET"		//��λָ��
#define	BL_FACTORY	"AT+FACTORY"	//�ָ���������
//ֻ����ѯ������
#define	BL_VER		"AT+VER"		//��ѯ�汾��
#define	BL_MAC		"AT+MAC"		//��ѯģ�� MAC��ַ
#define	BL_SCAN		"AT+SCAN"		//ɨ�赱ǰ�ڹ㲥���豸
//��ѯ����������
#define	BL_NAME		"AT+NAME"		//��ѯ����ģ�� NAME
#define	BL_UART		"AT+UART"		//��ѯ���ô��ڲ�����
#define	BL_ROLE		"AT+ROLE"		//��ѯ����ģ���ɫ
#define	BL_TXPWR	"AT+TXPWR"		//��ѯ����ģ�鷢�书��
#define	BL_UUID		"AT+UUID"		//��ѯ�����豸 UUID
#define	BL_ADVDATA	"AT+ADVDATA"	//��ѯ���ù㲥����


#define	BL_ADVIN	"AT+ADVIN"		//��ѯ����ģ��㲥��϶
#define	BL_CONIN	"AT+CONIN"		//��ѯ����ģ�����Ӽ�϶

//����������
#define	BL_CONNECT	"AT+CONNECT"	//ָ�������豸
#define	BL_IBEACON	"AT+IBEACON"	//��ѯ���� IBEACON ����
#define	BL_RSSI		"AT+RSSI"		//��ȡ�������豸 RSSI
//����ַ�
#define BLE_CMD_EQUAL_STR          "="
#define BLE_CMD_END_ECHO           "\r\n"
#define BLE_UART_DEFAULT_SET_STR   ",8,1,0"
#define BLE_TXPWR_END_ECHO           "dbm"
//�Ƚ��ַ���
#define BLE_ACK_OK_ECHO         "OK"
#define BLE_ACK_SET_OK_ECHO     "PARA SET:"
#define BLE_CMD_IN              "CMD IN"
#define BLE_CMD_OUT             "CMD OUT"
#define BLE_ACK_DATA_STOP_ECHO_0             ','
//�������ݴ���ƫ��
#define BLE_RX_UART_BAUD_OFFSET       12

//����Ĭ�ϲ���
#define BLE_DEFAULT_BAUD           115200
#define BLE_PENETRATE_MODE_BAUD     19200
//ʱ�䶨��
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

