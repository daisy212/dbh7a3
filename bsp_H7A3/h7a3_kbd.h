#ifndef H7A3_KBD_H
#define H7A3_KBD_H

#include "stm32h7xx.h"
#include "stdbool.h"
#include "stdint.h"
#include "RTOS.h"
#include <time.h>
#include <stdio.h>



/********************************************************************************
 Keyboard setting
*********************************************************************************/
#define KB_ROW 9
#define KB_COL 6

// maximum keys pressed at same time
#define KB_MAX_KEY 4

// shifting raw 
#define KB_SHIFT_SCRUT 3

// scrutation period in msec
#define KB_SCRUT_PERIOD    (15)

// key ok after x scrutation, anti rebound 
#define KB_VALID_COUNT     (3)

// auto repeat time in msec for db48x
#define KB_DB_REPEAT_FIRST (1000)
#define KB_DB_REPEAT_PERIOD (100)

#define COL_1_Pin          GPIO_PIN_4
#define COL_1_GPIO_Port    GPIOB
#define COL_2_Pin          GPIO_PIN_12
#define COL_2_GPIO_Port    GPIOB
#define COL_3_Pin          GPIO_PIN_15
#define COL_3_GPIO_Port    GPIOB
#define COL_4_Pin          GPIO_PIN_5
#define COL_4_GPIO_Port    GPIOB
#define COL_5_Pin          GPIO_PIN_3   //9
#define COL_5_GPIO_Port    GPIOB
#define COL_6_Pin          GPIO_PIN_8
#define COL_6_GPIO_Port    GPIOB




#define ROW_A_Pin          GPIO_PIN_15
#define ROW_A_GPIO_Port    GPIOD
#define ROW_B_Pin          GPIO_PIN_15
#define ROW_B_GPIO_Port    GPIOA
#define ROW_C_Pin          GPIO_PIN_6
#define ROW_C_GPIO_Port    GPIOC
#define ROW_D_Pin          GPIO_PIN_13
#define ROW_D_GPIO_Port    GPIOB
#define ROW_E_Pin          GPIO_PIN_4
#define ROW_E_GPIO_Port    GPIOA
#define ROW_G_Pin          GPIO_PIN_9
#define ROW_G_GPIO_Port    GPIOG

#define ROW_F_Pin          GPIO_PIN_9     //3
#define ROW_F_GPIO_Port    GPIOB


#define ROW_H_Pin          GPIO_PIN_7
#define ROW_H_GPIO_Port    GPIOC

#define ROW_I_Pin          GPIO_PIN_6
#define ROW_I_GPIO_Port    GPIOA


/********************************************************************************
 button PC13 setting
*********************************************************************************/

/* IRQ priorities */
#define BSP_BUTTON_USER_IT_PRIORITY         15U

/**
 * @brief Key push-button
 */
#define BUTTON_USER_PIN                       GPIO_PIN_13
#define BUTTON_USER_GPIO_PORT                 GPIOC
#define BUTTON_USER_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOC_CLK_ENABLE()
#define BUTTON_USER_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOC_CLK_DISABLE()
#define BUTTON_USER_EXTI_IRQn                 EXTI15_10_IRQn
#define BUTTON_USER_EXTI_LINE                 EXTI_LINE_13


#define KBD_COL1_EXTI_LINE                 EXTI_LINE_4
#define KBD_COL2_EXTI_LINE                 EXTI_LINE_12
#define KBD_COL3_EXTI_LINE                 EXTI_LINE_15
#define KBD_COL4_EXTI_LINE                 EXTI_LINE_5
#define KBD_COL5_EXTI_LINE                 EXTI_LINE_3
#define KBD_COL6_EXTI_LINE                 EXTI_LINE_8




typedef  struct {
   GPIO_TypeDef * gpio;
   uint32_t pin;
   uint32_t missing_key;
}
st_Pin;


typedef enum {
   SYS_nothing =0,
   SYS_1sec,
   SYS_1mn,
   SYS_RESET,
   SYS_P_OFF,
   SYS_WAKE_UP,
   SYS_SLEEPING,
   SYS_USB_event,
   SYS_int_rtc,
   SYS_LAST,
} t_SYS_CMD;

typedef struct {
         uint8_t key3;
         uint8_t key2;
         uint8_t key1;
         bool released;
         uint8_t key;
} st_key_data;

typedef struct {
   t_SYS_CMD  sys_cmd;
   uint32_t sys_data;
} st_sys_data;


typedef  enum {
   kb_scrut_std,
   kb_scrut_int,
   kb_scrut_int_exit
}keyb_mode;


typedef struct {
   uint32_t       key_time[KB_MAX_KEY];
   uint32_t       key_value[KB_MAX_KEY];
   uint32_t       sleeping_soon;
   uint32_t       c_sec;
   uint32_t       sleeping_time_sec;


   uint64_t       raw;
   keyb_mode      mode;
   st_key_data    dts;
} keyboard;

extern const st_Pin kbd_row[KB_ROW];
extern const char SyCmdDesc[SYS_LAST][32];


void init_button_pc13(void);
void KeyboardInit(keyboard *p_kbd);
uint32_t keyb_corr(uint32_t key);
void Kbd_Scrut_Set(keyboard *p_kbd, keyb_mode mode);

uint64_t Scrutation(keyboard *p_kbd, bool sending);

void Send_key(uint8_t key);


#endif // H7A3_KBD_H
