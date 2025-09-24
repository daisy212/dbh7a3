//#include "h7a3_kbd.h"
//#include "h7a3_rtc.h"
//#include "h7a3_usb.h"
//#include "h7a3_low_power.h"
//#include "h7a3_Sharp_LS.h"

#include "DBxxxx.h"




/* stop mode 2, wake-up by button pc13, colons and usb  */

EXTI_HandleTypeDef OS_RAM h_pb_exti_13;
EXTI_HandleTypeDef OS_RAM h_col1_exti_4, h_col2_exti_12, h_col3_exti_15, h_col4_exti_5, h_col5_exti_3, h_col6_exti_8;

extern OS_MAILBOX  Mb_Keyboard;
extern OS_EVENT     EV_USB_Vbus, RTC_Event, PB_Event, KBD_Event, USB_Event, DBu_Start, USB_Start, LED_Start, EV_db_ready, FOR_EVER;

extern   OS_TASK     TKBD, TDB48X, TUSB;

uint8_t OS_RAM wakeup_from = 0;

const char SyCmdDesc[SYS_LAST][32]={
"",
"SYS_1sec",
"SYS_1mn",
"SYS_RESET",
"P_OFF",
"SYS_WAKE_UP",
"SLEEPING",
"USB_event",
"SYS_int_rtc",

};







/* button PC13 */

/**
  * @brief  Key EXTI line detection callbacks.
  * @retval BSP status
  */
static void BUTTON_USER_EXTI_Callback(void)
{
   OS_INT_Enter();
   RTT_vprintf_cr_time( "Button exti13");
   OS_EVENT_Set(&PB_Event);
   OS_INT_Leave();
}


static void KBD_EXTI_Callback(void)
{
   OS_INT_Enter();
   RTT_vprintf_cr_time( "Kbd exti");
   OS_EVENT_Set(&KBD_Event);
   OS_INT_Leave();
   wakeup_from = 1;
}


void USB_EXTI_Callback(void);

void init_button_pc13(void){
  GPIO_InitTypeDef gpio_init_structure;
  BUTTON_USER_GPIO_CLK_ENABLE();

   /* Configure Button pin as input with External interrupt */
   gpio_init_structure.Pin = BUTTON_USER_PIN;
   gpio_init_structure.Pull = GPIO_PULLDOWN;
   gpio_init_structure.Speed = GPIO_SPEED_FREQ_HIGH;
   gpio_init_structure.Mode = GPIO_MODE_IT_RISING;
   HAL_GPIO_Init(BUTTON_USER_GPIO_PORT, &gpio_init_structure);

//#define BUTTON_USER_EXTI_IRQn                 EXTI15_10_IRQn
//#define BUTTON_USER_EXTI_LINE                 EXTI_LINE_13
   (void)HAL_EXTI_GetHandle(&h_pb_exti_13, BUTTON_USER_EXTI_LINE);
   (void)HAL_EXTI_RegisterCallback(&h_pb_exti_13,  HAL_EXTI_COMMON_CB_ID, BUTTON_USER_EXTI_Callback);

 /* Enable and set Button EXTI Interrupt to the lowest priority */
    HAL_NVIC_SetPriority((BUTTON_USER_EXTI_IRQn), BSP_BUTTON_USER_IT_PRIORITY, 0x00);
    HAL_NVIC_EnableIRQ((BUTTON_USER_EXTI_IRQn));
}


void EXTI3_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_col5_exti_3);     // 3
}  

void EXTI4_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_col1_exti_4);     // 4

//   EXTI->PR1 = 0xffff;
}

void EXTI15_10_IRQHandler(void)
{
   HAL_EXTI_IRQHandler(&h_col2_exti_12);     // 12
   HAL_EXTI_IRQHandler(&h_pb_exti_13);       // 13 button
   HAL_EXTI_IRQHandler(&h_col3_exti_15);     // 15


//   EXTI->PR1 = 0xffff;
}

extern EXTI_HandleTypeDef  h_usb_exti_9;

void EXTI9_5_IRQHandler(void)
{

   HAL_EXTI_IRQHandler(&h_col4_exti_5);     // 5
   HAL_EXTI_IRQHandler(&h_col6_exti_8);     // 8
   HAL_EXTI_IRQHandler(&h_usb_exti_9);     // 9

   EXTI->PR1 = 0xffff;
}





/* Keyboard */

const st_Pin kbd_row[KB_ROW]=
// defition of pins raw, outputs, low = scrutation
{
   { ROW_A_GPIO_Port, ROW_A_Pin, 0},
   { ROW_B_GPIO_Port, ROW_B_Pin, 0},
#if KB_ROW==9
   { ROW_C_GPIO_Port, ROW_C_Pin, 0},
#endif
   { ROW_D_GPIO_Port, ROW_D_Pin, 0},
   { ROW_E_GPIO_Port, ROW_E_Pin, 2},
   { ROW_F_GPIO_Port, ROW_F_Pin, 3},
   { ROW_G_GPIO_Port, ROW_G_Pin, 3},
   { ROW_H_GPIO_Port, ROW_H_Pin, 3},
   { ROW_I_GPIO_Port, ROW_I_Pin, 3},
};

const st_Pin kbd_col[]=
// definition of pins col, inputs, (with interrupts)
{
   { COL_1_GPIO_Port, COL_1_Pin, 0},
   { COL_2_GPIO_Port, COL_2_Pin, 0},
   { COL_3_GPIO_Port, COL_3_Pin, 0},
   { COL_4_GPIO_Port, COL_4_Pin, 0},
   { COL_5_GPIO_Port, COL_5_Pin, 0},
   { COL_6_GPIO_Port, COL_6_Pin, 0},
};

void KeyboardInit(keyboard *p_kbd)
/* pin initialisation, no interrupts for column */
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* GPIO Ports Clock Enable */
   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
   __HAL_RCC_GPIOC_CLK_ENABLE();
   __HAL_RCC_GPIOD_CLK_ENABLE();
   __HAL_RCC_GPIOE_CLK_ENABLE();
   __HAL_RCC_GPIOF_CLK_ENABLE();
   __HAL_RCC_GPIOG_CLK_ENABLE();

   // init PC13
   GPIO_InitStruct.Pin = BUTTON_USER_PIN;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   /* Configure Button pin as input with External interrupt */
   GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
   HAL_GPIO_Init(BUTTON_USER_GPIO_PORT, &GPIO_InitStruct);



// init row
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
      GPIO_InitStruct.Pin = kbd_row[i_row].pin;
      HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
      HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);
   }

// init col
   GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
   GPIO_InitStruct.Pull = GPIO_PULLUP;
//   HAL_GPIO_Init(COL_3_GPIO_Port, &GPIO_InitStruct);
   for (uint32_t i_col = 0; i_col < KB_COL; i_col++){
      GPIO_InitStruct.Pin = kbd_col[i_col].pin;
      HAL_GPIO_Init(kbd_col[i_col].gpio, &GPIO_InitStruct);
   }
// init interrupts
  /* EXTI interrupt init*/

    (void)HAL_EXTI_GetHandle(&h_pb_exti_13, BUTTON_USER_EXTI_LINE);
    (void)HAL_EXTI_RegisterCallback(&h_pb_exti_13,  HAL_EXTI_COMMON_CB_ID, BUTTON_USER_EXTI_Callback);

    (void)HAL_EXTI_GetHandle(&h_col1_exti_4, KBD_COL1_EXTI_LINE);
    (void)HAL_EXTI_GetHandle(&h_col2_exti_12, KBD_COL2_EXTI_LINE);
    (void)HAL_EXTI_GetHandle(&h_col3_exti_15, KBD_COL3_EXTI_LINE);
    (void)HAL_EXTI_GetHandle(&h_col4_exti_5, KBD_COL4_EXTI_LINE);
    (void)HAL_EXTI_GetHandle(&h_col5_exti_3, KBD_COL5_EXTI_LINE);
    (void)HAL_EXTI_GetHandle(&h_col6_exti_8, KBD_COL6_EXTI_LINE);
    (void)HAL_EXTI_RegisterCallback(&h_col1_exti_4,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
    (void)HAL_EXTI_RegisterCallback(&h_col2_exti_12,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
    (void)HAL_EXTI_RegisterCallback(&h_col3_exti_15,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
    (void)HAL_EXTI_RegisterCallback(&h_col4_exti_5,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
    (void)HAL_EXTI_RegisterCallback(&h_col5_exti_3,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);
    (void)HAL_EXTI_RegisterCallback(&h_col6_exti_8,  HAL_EXTI_COMMON_CB_ID, KBD_EXTI_Callback);

   HAL_NVIC_SetPriority(EXTI3_IRQn, 12, 0);
   HAL_NVIC_DisableIRQ(EXTI3_IRQn);

   HAL_NVIC_SetPriority(EXTI4_IRQn, 12, 0);
   HAL_NVIC_DisableIRQ(EXTI4_IRQn);

   HAL_NVIC_SetPriority(EXTI9_5_IRQn, 12, 0);
   HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);

   HAL_NVIC_SetPriority(EXTI15_10_IRQn, 12, 0);
   HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);


   for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++){
      p_kbd->key_value[ii] = 0; 
      p_kbd->key_time[ii] = 0;
   }

}


uint32_t keyb_corr(uint32_t key)
/* keyboard correction for missing keys in the 8*6 matrix */
{
   for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
      if (kbd_row[i_row].missing_key != 0){
         if ((key >= (kbd_row[i_row].missing_key + (i_row+1)*10)) &&(key <(kbd_row[i_row].missing_key+6+ (i_row+1)*10))) return key-1;
      }
   }
   return key;
}



void Kbd_Scrut_Set(keyboard *p_kbd, keyb_mode mode)
// mode kb_scrut, no interrupt, one row at low level at a time, normally all to high
{
   GPIO_InitTypeDef GPIO_InitStruct = {0};
//   EXTI->PR1 = 0xffff;
   if ( mode == p_kbd->mode) return;
   switch(mode){
      case    kb_scrut_std:
         // gpio scrutation
   
         // init row
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
            for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
               GPIO_InitStruct.Pin = kbd_row[i_row].pin;
               HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
               HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);
            }
         // init col
            GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
            GPIO_InitStruct.Pull = GPIO_PULLUP;
            HAL_GPIO_Init(COL_3_GPIO_Port, &GPIO_InitStruct);
            for (uint32_t i_col = 0; i_col < KB_COL; i_col++){
               GPIO_InitStruct.Pin = kbd_col[i_col].pin;
               HAL_GPIO_Init(kbd_col[i_col].gpio, &GPIO_InitStruct);
            }

            HAL_NVIC_DisableIRQ(EXTI3_IRQn);
            HAL_NVIC_DisableIRQ(EXTI4_IRQn);
            HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);           // detection usb ???
            HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
            for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
               HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
            }
            for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++){
               p_kbd->key_value[ii] = 0; 
               p_kbd->key_time[ii] = 0;
            }
            p_kbd->mode = kb_scrut_std;
            break;

      case    kb_scrut_int:
         // init row
         GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
         GPIO_InitStruct.Pull = GPIO_NOPULL;
         GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
         for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
            GPIO_InitStruct.Pin = kbd_row[i_row].pin;
            HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_RESET);
            HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);
         }
         // init col
         GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
         GPIO_InitStruct.Pull = GPIO_PULLUP;
         GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
         for (uint32_t i_col = 0; i_col < KB_COL; i_col++){
            GPIO_InitStruct.Pin = kbd_col[i_col].pin;
            HAL_GPIO_Init(kbd_col[i_col].gpio, &GPIO_InitStruct);
         }

         // allow all colomns interrupts
         __HAL_GPIO_EXTI_CLEAR_IT(COL_1_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_2_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_3_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_4_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_5_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_6_Pin);
//         OS_TASK_Delay_us(150);

         HAL_NVIC_EnableIRQ(EXTI3_IRQn);
         HAL_NVIC_EnableIRQ(EXTI4_IRQn);
         HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
         HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
//         OS_TASK_Delay_us(150);        // purge all pending interrupts
//         OS_EVENT_Reset(&KBD_Event);   // purge event 
         p_kbd->mode = kb_scrut_int;
         break;

      case    kb_scrut_int_exit:

         // init col
         GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
         GPIO_InitStruct.Pull = GPIO_PULLUP;
         HAL_GPIO_Init(COL_3_GPIO_Port, &GPIO_InitStruct);
         for (uint32_t i_col = 1; i_col < KB_COL; i_col++){
            GPIO_InitStruct.Pin = kbd_col[i_col].pin;
            HAL_GPIO_Init(kbd_col[i_col].gpio, &GPIO_InitStruct);
         }

         // init row
         GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
         GPIO_InitStruct.Pull = GPIO_PULLUP;
         GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
         for (uint32_t i_row = 0 ; i_row < 2; i_row++){
            GPIO_InitStruct.Pin = kbd_row[i_row].pin;
            HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
            HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);
         }
         for (uint32_t i_row = 3 ; i_row < KB_ROW; i_row++){
            GPIO_InitStruct.Pin = kbd_row[i_row].pin;
            HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
            HAL_GPIO_Init(kbd_row[i_row].gpio, &GPIO_InitStruct);
         }
      // wake up only with [exit]
         __HAL_GPIO_EXTI_CLEAR_IT(COL_1_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_2_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_3_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_4_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_5_Pin);
         __HAL_GPIO_EXTI_CLEAR_IT(COL_6_Pin);
         OS_TASK_Delay_us(150);
         HAL_NVIC_EnableIRQ(EXTI3_IRQn);
         HAL_NVIC_EnableIRQ(EXTI4_IRQn);
         HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
         HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);
         HAL_GPIO_WritePin(kbd_row[2].gpio, kbd_row[2].pin, GPIO_PIN_RESET);

         for (uint32_t i_row = 1 ; i_row < KB_ROW; i_row++){
// BUG
//            HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
// Analog mode


         }
         OS_TASK_Delay_us(150);        // purge all pending interrupts
         OS_EVENT_Reset(&KBD_Event);   // purge event 
         __HAL_GPIO_EXTI_CLEAR_IT(COL_1_Pin);
         p_kbd->mode = kb_scrut_int_exit;
         break;
   }
}


uint64_t Scrutation(keyboard *p_kbd, bool sending)
// scrutation clavier
{
   bool key_pressed_counting = false;
//   bool send_B1r = false;
   uint32_t key_value_act = 0;
//   uint32_t key_data_to_send = 0;
   uint32_t shift = 0;
   p_kbd->raw = 0;
   for (uint32_t i_row = 0 ; i_row < KB_ROW; i_row++){
      HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_RESET);
      OS_TASK_Delay_us(10);
      for (uint32_t i_col = 0; i_col < KB_COL; i_col++){
         key_pressed_counting = false;
//         key_value_act = (KB_ROW - i_row)*10 + (i_col+1);
         key_value_act = (1 + i_row)*10 + (i_col+1);


         if (HAL_GPIO_ReadPin(kbd_col[i_col].gpio, kbd_col[i_col].pin) == GPIO_PIN_RESET)
         { // key pressed
            p_kbd->sleeping_soon = 0;
            p_kbd->raw |= (uint64_t)1<< (uint64_t)( 6 * i_row + i_col + KB_SHIFT_SCRUT);   
//            key_data_to_send = 0;
            shift = 0;
//            p_kbd->dts.sys = 0;
            p_kbd->dts.key = 0;
            p_kbd->dts.key1 =0;
            p_kbd->dts.key2 =0;
            p_kbd->dts.key3 =0;
            for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++){
               if (key_value_act == p_kbd->key_value[ii]){ 
                  p_kbd->key_time[ii]++;
                  key_pressed_counting = true;
                  if (p_kbd->key_time[ii] == KB_VALID_COUNT){
                     p_kbd->dts.key = keyb_corr(p_kbd->key_value[ii]);
                  }
               }else 
               {
                  if (p_kbd->key_time[ii] >= KB_VALID_COUNT){
                     shift += 8;
//                     key_data_to_send |= (keyb_corr(p_kbd->key_value[ii])) << shift;
                     if (8 == shift) p_kbd->dts.key1 = keyb_corr(p_kbd->key_value[ii]);
                     if (16 == shift) p_kbd->dts.key2 = keyb_corr(p_kbd->key_value[ii]);
                     if (24 == shift) p_kbd->dts.key3 = keyb_corr(p_kbd->key_value[ii]);
                  }
               }
            }
            if    (p_kbd->dts.key)     {
               p_kbd->dts.released = 0;      
               if ( true == sending)
               {  
                  OS_MAILBOX_Put(&Mb_Keyboard, &p_kbd->dts);     // nouvelle touche pressée             
                  OS_TASKEVENT_Set( &TDB48X, EV_DBx_KBD);

               }
            }
            if (key_pressed_counting == false){
               for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++){
                  if (p_kbd->key_time[ii] == 0) {
                     p_kbd->key_value[ii] = key_value_act;
                     break;   
                  } 
               }
            }           
         }else 
         { // key not pressed
 //           p_kbd->dts.sys =0;
            p_kbd->dts.key =0;
            p_kbd->dts.key1 =0;
            p_kbd->dts.key2 =0;
            p_kbd->dts.key3 =0;
            shift = 0;
            for (uint32_t ii = 0; ii < KB_MAX_KEY; ii++){
               if ((key_value_act == p_kbd->key_value[ii])&&
                  (p_kbd->key_time[ii] >= KB_VALID_COUNT)){ 
                  // envoyer un message, touche relachée
                     p_kbd->dts.key = keyb_corr(key_value_act);
                     p_kbd->dts.released = 1;
                     p_kbd->key_time[ii] = 0;
                     p_kbd->key_value[ii] = 0;
                  }else 
                  {
                     if (p_kbd->key_time[ii] >= KB_VALID_COUNT){
                        shift += 8;
//                        key_data_to_send |= (keyb_corr(p_kbd->key_value[ii])) << shift;
                        if (8 == shift) p_kbd->dts.key1 = keyb_corr(p_kbd->key_value[ii]);
                        if (16 == shift) p_kbd->dts.key2 = keyb_corr(p_kbd->key_value[ii]);
                        if (24 == shift) p_kbd->dts.key3 = keyb_corr(p_kbd->key_value[ii]);
                     }
                  }
               }
            if    (p_kbd->dts.key)     {
               if ( true == sending )
               {  
                  OS_MAILBOX_Put(&Mb_Keyboard, &p_kbd->dts);     //  touche relachée              
                  OS_TASKEVENT_Set( &TDB48X, EV_DBx_KBD);

               }
            }
         }  
      }  
      HAL_GPIO_WritePin(kbd_row[i_row].gpio, kbd_row[i_row].pin, GPIO_PIN_SET);
   }
   return p_kbd->raw;
}


void Send_key(uint8_t key){
   st_key_data dts;
//   dts.sys = 0;

   dts.key = key;
   dts.key1 = 0;
   dts.key2 = 0;
   dts.key3 = 0;
   dts.released = 0;
   OS_MAILBOX_Put(&Mb_Keyboard, &dts); // press             
   dts.released = 1;
   OS_MAILBOX_Put(&Mb_Keyboard, &dts); // release        
   OS_TASKEVENT_Set( &TDB48X, EV_DBx_KBD);
}


