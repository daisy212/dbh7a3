/*
#include "h7a3_rtc.h"
#include "h7a3_usb.h"

#include "h7a3_low_power.h"


#include "FS.h"
*/

#include "BSP.h"
#include "DBxxxx.h"

/*

#ifdef __cplusplus
extern "C" {     // Make sure we have C-declarations in C++ programs 
#endif
void DBx_Task_usb(void);
#ifdef __cplusplus
}
#endif
*/
void DBx_Task_Usb(void) 
{
   usb_connected = false;
   OS_TASKEVENT MyEvents;
   Init_Usb_Detect();
   HAL_NVIC_EnableIRQ(USB_DETECT_EXTI_IRQn);
   OS_TASK_Delay_ms(50);

//   OS_EVENT_GetBlocked(&USB_Start);
   RTT_vprintf_cr_time(  "Usb : waiting");

   while(1){
      OS_EVENT_GetBlocked(&EV_USB_Vbus);
// same interrupt than keyboard...
//     HAL_NVIC_DisableIRQ(USB_DETECT_EXTI_IRQn);
      RTT_vprintf_cr_time( "USB : connexion");
      HAL_NVIC_DisableIRQ(USB_DETECT_EXTI_IRQn);

      SystemClock_Config_P64();
      SystemClock_Config_P280();

      usb_connected = true;
      USBD_Init();

      _FSTest();

      _AddMSD();
      USBD_Start();

      OS_TASKEVENT_Set( &TDB48X, EV_DBx_USB_CON);
      MyEvents = OS_TASKEVENT_GetSingleTimed(EV_USB_ACQ, 50);

      while (Usb_Detect()) {
       //
       // Wait for configuration
       //
         while (((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) != USB_STAT_CONFIGURED)&&(Usb_Detect())) {
            BSP_ToggleLED(0);
            OS_TASK_Delay_ms(50);
         }
         BSP_SetLED(0);
         USBD_MSD_Task();
         OS_TASK_Delay_ms(50);
      }
      // deconnexion

      FS_Sync("nor:0:");
      usb_connected = false;
      RTT_vprintf_cr_time( "USB : disconnect");
      USBD_DeInit();
      HAL_NVIC_EnableIRQ(USB_DETECT_EXTI_IRQn);
      OS_TASK_Delay_ms(1);
      OS_EVENT_Reset(&EV_USB_Vbus);

// 135µA, 105µA higher than before usb connection 
// sd detect !!!
// 60µA instead of 30µA
      HAL_PWREx_DisableUSBVoltageDetector();       
// back to 30µA

      __HAL_RCC_USB_OTG_HS_CLK_DISABLE();    // needed ????

// can't modify pll when running on pll, pll => HSI, pll mod => pll
      SystemClock_Config_P64();        // cpu using HSI 64Mhz
      SystemClock_Config_P120();        // new pll setting
      SystemCoreClockUpdate();

      OS_TASKEVENT_Set( &TDB48X, EV_DBx_USB_DIS);
// this task if higher priority than DB48X, giving some time to work after sending USB_DIS
      MyEvents = OS_TASKEVENT_GetSingleTimed(EV_USB_ACQ, 50);

// a déplacer ???
      usb_connected = false;        // ==> power stop enabled
   }
}
