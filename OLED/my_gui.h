#ifndef MY_GUI_H
#define MY_GUI_H
#include "sys.h"
 
void  GUI_DeviceBootInit(u8 x,u8 y);
void  GUI_DeviceUpdatePrepare(u8 x,u8 y);
void  GUI_DeviceUpdate(u8 x,u8 y); 
void  GUI_DeviceUpdateAppOk(u8 x,u8 y)   __attribute__((section("UpdateOk")));
void  GUI_DeviceWriteAppOk(u8 x,u8 y);
void  GUI_DeviceFactorySet(u8 x,u8 y);
void  GUI_DeviceFactorySetOK(u8 x,u8 y);

#endif
