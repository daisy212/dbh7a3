
//#include "Global.h"

#include "BSP.h"
#include "DBxxxx.h"


keyboard OS_RAM keybd;
uint64_t scrut_last = 0;

bool       StopMode2_disable = true;


extern uint8_t OS_RAM wakeup_from;

extern OS_MAILBOX  Mb_Keyboard;
extern   OS_TASK     TKBD, TDB48X, TUSB;

void DBx_Task_Kbd(void) 
/* keyboard and scheduler task
 * key repetition by db48x 
 * send struct st_key_data.
 * active key : bool released, int key, allready pressed : key1, key2, key3 
 * sending events to other tasks : EV_DBx_RESET

 * using db_power_state
 */
{
   char toggle='s';
   char buff[40];
//   st_sys_data sys_ts;
   KeyboardInit( &keybd);
   keybd.sleeping_soon = 0;
   while(1) 
   {
      switch (db_power_state){
         case PW_power_on:
             OS_EVENT_GetBlocked(&EV_db_ready);
            db_power_state = PW_wake_up;
            break;
         case PW_wake_up:
            Kbd_Scrut_Set( &keybd,kb_scrut_std);
            db_power_state = PW_running;
            keybd.sleeping_soon=0;
            RtcIntXsec(0);                      // int from rtc each second
            break;
         case PW_running:
            if (( false == usb_connected )&&(Usb_Detect()))
            {  // missed exti interrupt !!!!!
               OS_TASKEVENT_Set( &TDB48X, EV_DBx_USB_CON);
               OS_EVENT_Set(&EV_USB_Vbus);
               usb_connected = true;
            }
            OS_TASK_Delay( TIME_CORRECTION(KB_SCRUT_PERIOD) );
            if ( usb_connected | StopMode2_disable )
                  keybd.sleeping_soon = 0;
            else 
               keybd.sleeping_soon += KB_SCRUT_PERIOD ;

            scrut_last = Scrutation( &keybd, true);
            if ( 0x8108 == scrut_last)       // [F1] + [F6] + [Exit] = reset
            {
               OS_TASKEVENT_Set( &TDB48X, EV_DBx_RESET);
               OS_TASK_Delay_ms(2500);
               NVIC_SystemReset();
            }


            if ((keybd.sleeping_soon > TIME_BEFORE_STOP2_msec) &&( 0 == scrut_last ) && ( !usb_connected )) 
            {
                  keybd.sleeping_time_sec = 1;            // at least 1sec ??
                  RtcIntXsec(keybd.sleeping_time_sec);
                  db_power_state = PW_sleeping;
            }
            if ( ( true == usb_connected ))
            {
               if ( toggle !=  '*'){
//                  snprintf( buff, sizeof(buff),"Usb   ");
//                  LCD_FillRect( &hlcd, 10, 100, 180, 24, 0);
//                  display_text_12x24(&hlcd, 10, 100, buff,1);
               }
               toggle =  '*';
            }
            break;
         case PW_sleeping:
            RTT_vprintf_cr_time( "PW :(%d) Entering sleep for %d", keybd.sleeping_soon, keybd.sleeping_time_sec+1);
            Kbd_Scrut_Set( &keybd,kb_scrut_int); // 60s, 10s, 1s

            toggle= toggle == '/'? '\\':'/';
            keybd.sleeping_soon += EnterSTOP2();

            if (1 == wakeup_from) { // kbd
            //            Kbd_Scrut_Set( &keybd,kb_scrut_std); // 60s, 10s, 1s
                        RTT_vprintf_cr_time( "PW :(%d) Wake up from kbd ", keybd.sleeping_soon);
                        RtcIntXsec(0);       // interruption chaque début de seconde
                        db_power_state = PW_wake_up;
                        break;
            } 
            if (2 == wakeup_from) { // rtc
               RTT_vprintf_cr_time( "PW :(%d) Wake up from rtc ", keybd.sleeping_soon);
               // configuring timer for 1, 10, 60 sec, ever
               if (keybd.sleeping_soon >T_POWER_OFF_sec*1000) 
               {
                  OS_TASKEVENT_Set( &TDB48X, EV_DBx_POFF);
                  OS_TASK_Delay( TIME_CORRECTION(100) );

                  db_power_state = PW_near_deep_sleep;
               }
               else 
               {
                  if (keybd.sleeping_soon >60000) 
                  {
                     keybd.sleeping_time_sec = 59;
                  } 
                  else 
                  {
                     if (keybd.sleeping_soon >10000) keybd.sleeping_time_sec = 9;
                     else keybd.sleeping_time_sec = 0;            // at least 1sec ??
                  }
                  RtcIntXsec(keybd.sleeping_time_sec);
                  OS_TASKEVENT_Set( &TDB48X, EV_DBx_RTC);
                  OS_TASK_Delay( TIME_CORRECTION(100) );
               }
            }
            
            if (3 == wakeup_from) { // usb vbus event
//               usb_connected = Usb_Detect();
                  OS_TASKEVENT_Set( &TDB48X, EV_DBx_USB_CON);

               RTT_vprintf_cr_time( "PW :(%d) Wake up from usb bus : %s ", keybd.sleeping_soon, usb_connected ? "connected":"disconnected");
               RtcIntXsec(0);       // interruption chaque début de seconde
               db_power_state = PW_wake_up;
               break;
            } 
            if ( Usb_Detect())
            {  
               RTT_vprintf_cr_time( "PW :(%d) Missed exti from usb bus : %s ", keybd.sleeping_soon, usb_connected ? "connected":"disconnected");
                  usb_connected = true;
                  keybd.sleeping_soon = 0;
                  db_power_state = PW_wake_up;
                  OS_EVENT_Set(&EV_USB_Vbus);
                  break;
            }
            
            break;

         case PW_near_deep_sleep:
            Kbd_Scrut_Set( &keybd,kb_scrut_int); 
            RtcIntXsec(0); // int next second
            EnterSTOP2();
            db_power_state = PW_deep_sleep;
            break;

         case PW_deep_sleep:
            snprintf( buff, sizeof(buff),"Power Off");
            LCD_FillRect( &hlcd, 10, 100, 180, 24, 0);
            display_text_12x24(&hlcd, 10, 100, buff,1);
            LCD_UpdateModifiedLines(&hlcd);
            OS_TASK_Delay_ms(500);

            RTT_vprintf_cr_time( "PW :(%d) PowerOff", keybd.sleeping_soon);
            RtcIntDisable();
            Kbd_Scrut_Set( &keybd,kb_scrut_int_exit);
            EnterSTOP2();
            keybd.sleeping_soon = 0;
            RTT_vprintf_cr_time( "PW :(%d) Leaving P Off ", keybd.sleeping_soon);
            db_power_state = PW_wake_up;
            break;
         default:
            break;
      }
   }
}
