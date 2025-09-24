#ifndef H7A3_USB_H
#define H7A3_USB_H


#include "stdbool.h"
#include "stdint.h"

#include "RTOS.h"
#include "USB.h"
#include "USB_MSD.h"


#define USB_DETECT_Pin           GPIO_PIN_9
#define USB_DETECT_GPIO_Port     GPIOA
#define USB_DETECT_EXTI_IRQn     EXTI9_5_IRQn   // shared with button & keyboard
#define USB_Vbus_EXTI_LINE       EXTI_LINE_9


extern   OS_EVENT     EV_USB_Vbus, USB_Event;
extern bool usb_connected;


void _FSTest(void);
 void _AddMSD(void);

void Init_Usb_Detect(void);

bool Usb_Detect(void);


#endif   // H7A3_USB_H


