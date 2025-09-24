/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2023  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
Purpose : Config file for STM32H735G-DK (MB1520)
--------  END-OF-HEADER  ---------------------------------------------
*/


#include "FS.h"
#include "USB.h"
//#include "BSP_USB.h"
#include "stm32h7xx.h"
#include "RTOS.h"



typedef void USB_ISR_HANDLER  (void);
static USB_ISR_HANDLER * _pfOTG_HSHandler;
/****** Declare ISR handler here to avoid "no prototype" warning. They are not declared in any CMSIS header */

#ifdef __cplusplus
extern "C" {
#endif
void OTG_HS_IRQHandler(void);
#ifdef __cplusplus
}
#endif


/*********************************************************************
*
*       OTG_HS_IRQHandler
*/
void OTG_HS_IRQHandler(void) {
  OS_INT_Enter(); // Inform embOS that interrupt code is running
  if (_pfOTG_HSHandler) {
    (_pfOTG_HSHandler)();
  }
  OS_INT_Leave(); // Inform embOS that interrupt code is left
}

/*********************************************************************
*
*       BSP_USB_InstallISR_Ex()
*/
void BSP_USB_InstallISR_Ex(int ISRIndex, void (*pfISR)(void), int Prio){
  (void)Prio;
  if (ISRIndex == OTG_HS_IRQn) {
    _pfOTG_HSHandler = pfISR;
  }
  NVIC_SetPriority((IRQn_Type)ISRIndex, (1u << __NVIC_PRIO_BITS) - 2u);
  NVIC_EnableIRQ((IRQn_Type)ISRIndex);
}

/*********************************************************************
*
*       BSP_USBH_InstallISR_Ex()
*/
void BSP_USBH_InstallISR_Ex(int ISRIndex, void (*pfISR)(void), int Prio){
  (void)Prio;
  if (ISRIndex == OTG_HS_IRQn) {
    _pfOTG_HSHandler = pfISR;
  }
  NVIC_SetPriority((IRQn_Type)ISRIndex, (1u << __NVIC_PRIO_BITS) - 2u);
  NVIC_EnableIRQ((IRQn_Type)ISRIndex);
}






/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#define USB_ISR_PRIO  254

/*********************************************************************
*
*       Defines, sfrs
*
**********************************************************************
*/
//
// AXI
//
#define AXI_BASE_ADDR             0x51000000u
#define AXI_TARG7_FN_MOD_ISS_BM   (*(volatile U32 *)(AXI_BASE_ADDR + 0x1008 + 0x7000))

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_RAM U32 _EPBufferPool[1280 / 4];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
/*********************************************************************
*
*       _EnableISR
*/
static void _EnableISR(USB_ISR_HANDLER * pfISRHandler) {
  BSP_USB_InstallISR_Ex((int)OTG_HS_IRQn, pfISRHandler, USB_ISR_PRIO);
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       Setup which target USB driver shall be used
*/

/*********************************************************************
*
*       USBD_X_Config
*/
void USBD_X_Config(void) {
  //
  // Configure IO's
  //
  RCC->AHB4ENR |= 0
               | RCC_AHB4ENR_GPIOAEN  // GPIOAEN: IO port A clock enable
               ;
  //
  //  PA10: USB_ID
  //
  GPIOA->MODER    =   (GPIOA->MODER  & ~(3UL  <<  20)) | (2UL  <<  20);
  GPIOA->OTYPER  |=   (1UL  <<  10);
  GPIOA->OSPEEDR |=   (3UL  <<  20);
  GPIOA->PUPDR    =   (GPIOA->PUPDR & ~(3UL  <<  20)) | (1UL << 20);
  GPIOA->AFR[1]   =   (GPIOA->AFR[1]  & ~(15UL << 8)) | (10UL << 8);
  //
  //  PA11: USB_DM
  //
  GPIOA->MODER    =   (GPIOA->MODER  & ~(3UL  <<  22)) | (2UL  <<  22);
  GPIOA->OTYPER  &=  ~(1UL  <<  11);
  GPIOA->OSPEEDR |=   (3UL  <<  22);
  GPIOA->PUPDR   &=  ~(3UL  <<  22);
  GPIOA->AFR[1]   =   (GPIOA->AFR[1]  & ~(15UL << 12)) | (10UL << 12);
  //
  //  PA12: USB_DP
  //
  GPIOA->MODER    =   (GPIOA->MODER  & ~(3UL  <<  24)) | (2UL  <<  24);
  GPIOA->OTYPER  &=  ~(1UL  <<  12);
  GPIOA->OSPEEDR |=   (3UL  <<  24);
  GPIOA->PUPDR   &=  ~(3UL  <<  24);
  GPIOA->AFR[1]   =   (GPIOA->AFR[1]  & ~(15UL << 16)) | (10UL << 16);
  //
  // Enable HSI48
  //
  RCC->CR |= RCC_CR_HSI48ON;
  while ((RCC->CR & RCC_CR_HSI48RDY) == 0) {
  }
  //
  // Set USB clock selector to HSI48
  //
/*     stm32h7a3
#define RCC_CDCCIP2R_USBSEL_Msk           (0x3UL << RCC_CDCCIP2R_USBSEL_Pos)           // !< 0x00300000 
#define RCC_CDCCIP2R_USBSEL               RCC_CDCCIP2R_USBSEL_Msk
#define RCC_USBCLKSOURCE_HSI48            RCC_CDCCIP2R_USBSEL
stm32h735 : RCC->D2CCIP2R |= RCC_D2CCIP2R_USBSEL;
*/
  RCC->CDCCIP2R |= RCC_USBCLKSOURCE_HSI48;
  //
  //  Enable clock for OTG_HS1
  //
  RCC->AHB1ENR    |=  RCC_AHB1ENR_USB1OTGHSEN;
  USB_OS_Delay(10);
  //
  // Reset USB clock
  //
  RCC->AHB1RSTR   |=  RCC_AHB1RSTR_USB1OTGHSRST;
  USB_OS_Delay(10);
  RCC->AHB1RSTR   &= ~(RCC_AHB1RSTR_USB1OTGHSRST);
  USB_OS_Delay(40);
  //
  // Enable voltage level detector for transceiver
  //
  PWR->CR3 |= PWR_CR3_USB33DEN;
  //
  // Workaround to avoid AXI SRAM corruption (see STM32H753xI Errata sheet Rev. 2, November 2017)
  // According to ST this errata has been fixed with chip revisions X and V.
  // If you are using one of the newer chips you can remove the following line.
  //
  AXI_TARG7_FN_MOD_ISS_BM |= 1;
  //
  // Override the B-Session valid operation from the USB PHY
  //
  USB1_OTG_HS->GOTGCTL |= 0
                 | USB_OTG_GOTGCTL_BVALOVAL
                 | USB_OTG_GOTGCTL_BVALOEN
                 ;
  USBD_AddDriver(&USB_Driver_ST_STM32H7xxHS_inFS_DynMem);

  USBD_AssignMemory(_EPBufferPool, sizeof(_EPBufferPool));
  USBD_SetISRMgmFuncs(_EnableISR, USB_OS_IncDI, USB_OS_DecRI);
}

/*************************** End of file ****************************/
