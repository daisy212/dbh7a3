#ifndef H7A3_LOW_POWER_H
#define H7A3_LOW_POWER_H

#include "stm32h7xx.h"
#include "stdbool.h"
#include "stdint.h"
#include "RTOS.h"
#include <time.h>
#include <stdio.h>



typedef enum {
/* etats possible :
 * 0 : emerging from deep sleep
 * 1 : running, scrutation active
 * 2 : sleep, keyboard with interrupt
 * 3 : entering deep sleep, 
 * 4 : deep sleep
 * 5 : power_on
 */
   PW_wake_up = 0,
   PW_running = 1,
   PW_sleeping = 2,
   PW_near_deep_sleep = 3,
   PW_deep_sleep = 4,
   PW_power_on = 5,
   PW_power_on_waiting = 6,
   PW_LAST,
}t_POWER_STATE;

extern t_POWER_STATE db_power_state;


void ready_to_stop2_periph(void);

void init_unused_pins(void);
void ready_to_stop2_gpio(void);
void Restore_After_STOP2(void);
void disable_debug(void);

#define TIME_CORRECTION(x)    ((usb_connected ? (7*x)/2:x))


void SystemClock_Config_P280(void);
void SystemClock_Config_P120(void);
void SystemClock_Config_P64(void); // no pll for cpu
void PeriphCommonClock_Config_P16_R64(void);

void    Error_Handler(int err_no);


uint32_t EnterSTOP2(void);


#endif //H7A3_LOW_POWER_H

