/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2022  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : USB_MSD_FS_Start.c
Purpose : This sample demonstrates the use of the MSD component together
          with emFile.

Additional information:
  Preparations:
    The correct emFile configuration file has
    to be included in the project. Depending on the hardware
    it can be one of the following:
    * FS_ConfigRAMDisk_23k.c
    * FS_ConfigNAND_*.c
    * FS_ConfigMMC_CardMode_*.c
    * FS_ConfigNAND_*.c

  Expected behavior:
    This sample will format the storage medium if necessary and
    create a "Readme.txt" file in the root of the storage
    medium. After the formatting is done and the USB cable has
    been connected to a PC a new MSD volume will show up with
    a "Readme.txt" file in the root directory.

  Sample output:
    The target side does not produce terminal output.
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/
#include "RTOS.h"
#include <string.h>
#include "FS.h"
#include "BSP.h"
#include "stdbool.h"
#include "h7a3_usb.h"

#include "h7a3_rtc.h"

extern OS_EVENT       RTC_Event, PB_Event, KBD_Event, DBu_Start, USB_Start, LED_Start, EV_db_ready, FOR_EVER;

OS_EVENT    OS_RAM   EV_USB_Vbus, USB_Event;

bool usb_connected = false;

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define BUFFER_SIZE       8192

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/
//
//  Information that is used during enumeration.
//
static const USB_DEVICE_INFO _DeviceInfo = {
   0x8765,         // VendorId
   0x1000,         // ProductId
   "Vendor",       // VendorName
   "MSD device",   // ProductName
   "000013245678"  // SerialNumber. Should be 12 character or more for compliance with Mass Storage Device For Bootability spec.
};
//
// String information used when inquiring the volume 0.
//
static const USB_MSD_LUN_INFO _Lun0Info = {
   "DB",     // MSD VendorName
   "h7a3_SD", // MSD ProductName
   "1.00",       // MSD ProductVer
   "134657890"   // MSD SerialNo
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_RAM U32  _aSectorBuffer[BUFFER_SIZE / 4];     // Used as sector buffer in order to do read/write sector bursts (~8 sectors at once)

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _FSTest
*/
void _FSTest(void) {
   FS_FILE    * pFile;
   unsigned     Len;
   const char * sInfo = "This sample is based on the SEGGER emUSB-Device software with an MSD component.\r\nFor further information please visit: www.segger.com\r\n";
   unsigned     NumVolumes;
   unsigned     i;
   char         acVolumeName[20];

   Len        = strlen(sInfo);
   NumVolumes = FS_GetNumVolumes();
   for (i = 0; i < NumVolumes; i++) {
      FS_GetVolumeName(i, &acVolumeName[0], sizeof(acVolumeName));
      if (FS_IsLLFormatted(acVolumeName) == 0) {
         FS_X_Log("Low level formatting");
         FS_FormatLow(acVolumeName);          /* Erase & Low-level  format the volume */
      }
      if (FS_IsHLFormatted(acVolumeName) == 0) {
         FS_X_Log("High level formatting\n");
         FS_Format(acVolumeName, NULL);       /* High-level format the volume */
      }
      strcat(acVolumeName, "\\Readme.txt");
      pFile = FS_FOpen(acVolumeName, "w");
      FS_Write(pFile, sInfo, Len);
      FS_FClose(pFile);
      FS_SetVolumeLabel(acVolumeName, "Db48x_h7a3");
      FS_Unmount(acVolumeName);
   }
}

/*********************************************************************
*
*       _AddMSD
*
*  Function description
*    Add mass storage device to USB stack
*
*  Notes:
*   (1)  -   This examples uses the internal driver of the file system.
*            The module initializes the low-level part of the file system if necessary.
*            If FS_Init() was not previously called, none of the high level functions
*            such as FS_FOpen, FS_Write etc will work.
*            Only functions that are driver related will be called.
*            Initialization, sector read/write, retrieve device information.
*            The members of the DriverData are used as follows:
*              DriverData.pStart       = VOLUME_NAME such as "nand:", "mmc:1:".
*              DriverData.NumSectors   = Number of sectors to be used - 0 means auto-detect.
*              DriverData.StartSector  = The first sector that shall be used.
*              DriverData.SectorSize will not be used.
*/
 void _AddMSD(void) {
   static U8         _abOutBuffer[USB_HS_BULK_MAX_PACKET_SIZE];
   USB_MSD_INIT_DATA InitData;
   USB_MSD_INST_DATA InstData;
   USB_ADD_EP_INFO       EPIn;
   USB_ADD_EP_INFO       EPOut;

   memset(&InitData, 0, sizeof(InitData));
   EPIn.Flags          = 0;                             // Flags not used.
   EPIn.InDir          = USB_DIR_IN;                    // IN direction (Device to Host)
   EPIn.Interval       = 0;                             // Interval not used for Bulk endpoints.
   EPIn.MaxPacketSize  = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
   EPIn.TransferType   = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
   InitData.EPIn  = USBD_AddEPEx(&EPIn, NULL, 0);

   EPOut.Flags         = 0;                             // Flags not used.
   EPOut.InDir         = USB_DIR_OUT;                   // OUT direction (Host to Device)
   EPOut.Interval      = 0;                             // Interval not used for Bulk endpoints.
   EPOut.MaxPacketSize = USB_HS_BULK_MAX_PACKET_SIZE;   // Maximum packet size (512 for Bulk in high-speed).
   EPOut.TransferType  = USB_TRANSFER_TYPE_BULK;        // Endpoint type - Bulk.
   InitData.EPOut = USBD_AddEPEx(&EPOut, _abOutBuffer, sizeof(_abOutBuffer));

   #if !USE_RNDIS
   USBD_SetDeviceInfo(&_DeviceInfo);
   #endif
   USBD_MSD_Add(&InitData);
   //
   // Add logical unit 0:
   //
   memset(&InstData, 0,  sizeof(InstData));
   InstData.pAPI                    = &USB_MSD_StorageByName;    // s. Note (1)
   InstData.DriverData.pStart       = (void *)"";
   InstData.DriverData.pSectorBuffer   = _aSectorBuffer;
   InstData.DriverData.NumBytes4Buffer = sizeof(_aSectorBuffer);
   InstData.pLunInfo = &_Lun0Info;
   USBD_MSD_AddUnit(&InstData);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

EXTI_HandleTypeDef OS_RAM h_usb_exti_9;

extern uint8_t  wakeup_from;

void USB_EXTI_Callback(void)
{
   OS_INT_Enter();
   RTT_vprintf_cr_time( "USB detect");
   OS_EVENT_Set(&KBD_Event);
   OS_EVENT_Set(&EV_USB_Vbus);

   OS_INT_Leave();
   wakeup_from = 3;
}


void Init_Usb_Detect(void)
// init usb detect pin
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};

   __HAL_RCC_GPIOA_CLK_ENABLE();

// init PA9, EXTI, rising edge, pull-down
   GPIO_InitStruct.Pin = USB_DETECT_Pin;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
   HAL_GPIO_Init(USB_DETECT_GPIO_Port, &GPIO_InitStruct);

   (void)HAL_EXTI_GetHandle(&h_usb_exti_9, USB_Vbus_EXTI_LINE);
   (void)HAL_EXTI_RegisterCallback(&h_usb_exti_9,  HAL_EXTI_COMMON_CB_ID, USB_EXTI_Callback);

   HAL_NVIC_SetPriority(USB_DETECT_EXTI_IRQn, 12, 0);
   HAL_NVIC_DisableIRQ(USB_DETECT_EXTI_IRQn);

}

bool Usb_Detect(void)
// return 1 if is usb connected
{
   return  (HAL_GPIO_ReadPin( USB_DETECT_GPIO_Port, USB_DETECT_Pin) == GPIO_PIN_SET) ? 1 : 0;
}



/**************************** end of file ***************************/
