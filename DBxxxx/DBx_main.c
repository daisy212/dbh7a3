
#include <stdio.h>

#include "Global.h"

#include "RTOS.h"
#include "SEGGER.h"

#include "SEGGER_RTT.h" // using SEGGER_RTT_printf
//#include "FS.h"
//#include "FS_OS.h"


//#include "RTOS.h"
#include "BSP.h"


//#include "h7a3_usb.h"

//#include "h7a3_rtc.h"
//#include "h7a3_kbd.h"
//#include "h7a3_bkpram.h"
//#include "h7a3_low_power.h"
//#include "h7a3_Sharp_LS.h"

#include "DBx_tsk_usb.h"
//#include "DBx_tsk_pw_kbd.h"


#include "DBxxxx.h"




#define MAX_MB_KEYBOARD 32
#define MAX_MB_SYSTEM 32
#define SIZE_DATA_KEYBOARD (sizeof(st_key_data))
#define SIZE_DATA_SYSTEM (sizeof(st_sys_data))



static OS_RAM  OS_TASK     TCBHP, TCBLP;                // Task control blocks
OS_RAM  OS_TASK     TKBD, TDB48X, TUSB, TLCD, TLPW, TTST_DB, TTST_FS;    /* Task-control-block */

static OS_STACKPTR OS_RAM  int StackHP[128], StackLP[1024];  // Task stacks
static OS_STACKPTR OS_RAM  int StackLpwr[512];
static OS_STACKPTR OS_RAM  int StackTest_DB[1024];
static OS_STACKPTR OS_RAM  int StackTest_FS[4096];
static OS_STACKPTR OS_RAM  int StackDb48x[8192];        /* Task stack 32ko */
static OS_STACKPTR OS_RAM  int StackKbd[512];
static OS_STACKPTR OS_RAM  int StackUsb[512];

OS_EVENT    OS_RAM    KBD_Event,  DBu_Start, USB_Start, LED_Start, EV_db_ready, FOR_EVER, RTC_Event, PB_Event;

OS_MAILBOX  OS_RAM   Mb_Keyboard, Mb_Sys;
static char   OS_RAM   Mb_KeyboardBuffer[MAX_MB_KEYBOARD * SIZE_DATA_KEYBOARD ];
static char   OS_RAM   Mb_SysBuffer[MAX_MB_SYSTEM * SIZE_DATA_SYSTEM ];
char OS_RAM buff[256];
uint32_t db_calc_state = 0;


//static char BOARD_RAM1 lcd_mem[536][336/8];

#define PWM_FREQ                (100u)                         // 100 Hz PWM frequency
#define PWM_PERIOD              (1000000u / PWM_FREQ)          // PWM period in microseconds (100 Hz => 10,000 microseconds)
#define PWM_RESOLUTION          (100u)                         // PWM resolution
#define PWM_RESOLUTION_PERIOD   (PWM_PERIOD / PWM_RESOLUTION)  // PWM resolution period in microseconds (100 microseconds)


// compiling EmFile with embOs Ultra
void FS_ErrorOutf     (U32 Type, const char * sFormat, ...){
   va_list args;
   va_start(args, sFormat);
   SEGGER_RTT_vprintf(0, sFormat, &args);
}
int  SEGGER_vsnprintf    (char* pBuffer, int BufferSize, const char* sFormat, va_list ParamList){
   return SEGGER_RTT_vprintf(0, sFormat, &ParamList);
}

   char buffext[1024];

void dbxxxx_recorder(const char * sFctName, const char * sFormat, ...)
// redirection record
// debug message, deleted in release mode
{
#if DEBUG
   va_list args;
   va_start(args, sFormat);
   FS_FILE * pFile;
   char      acFileName[40] = {"debug\\record.txt"};
   char      sTime[64];
int nb =strcmp("sparse_fonts", sFctName);
  if (0 == nb) return;
   nb =strcmp("gc_details", sFctName);
  if (0 == nb) return;
   nb =strcmp("editor", sFctName);
  if (0 == nb) return;


   snprintf(sTime, sizeof(sTime), "\n%03d.%03d[%s] ", (rtc_read_ticks()/1000)%1000,rtc_read_ticks()%1000, sFctName);
   vsnprintf(buffext, sizeof(buffext),  sFormat, args);
   va_end(args);

   #if RecorderToDisk
      pFile = FS_FOpen(acFileName, "ab");
      if (pFile != NULL) {
         (void)FS_Write(pFile, sTime, strlen(sTime));
         (void)FS_Write(pFile, buffext, strlen(buffext));
         (void)FS_FClose(pFile);
      }
   #endif
   #if RecorderToRTT
      SEGGER_RTT_printf(0, sTime );
      SEGGER_RTT_printf(0,  buffext);
   #endif

#endif // DEBUG
}




// draw keyboard keys
#define KEY_H     ((LCD_HEIGHT -40)/ 6 -5)
#define KEY_W     (LCD_WIDTH / 12 -5)
void      draw_key(int x , int y, bool release ){

   // calculate coordinates
   int k_x, k_y;
   int k_w= KEY_W;
   int m_k = kbd_row[y-1].missing_key;
   if ((m_k !=0)&&(6==x)) return;

   k_x = 5 + (x-1) * (KEY_W + 3);
   k_y = 40 + 3*(KEY_H + 3) + (y-1)* (KEY_H + 3);
   if (y> 3) {
      k_x += (KEY_W + 3) * 6 + 5;
      k_y -=  (KEY_H + 3) *6;
   }

   if (2 == m_k )  { // ligne enter
      if (x==1){
         k_x += KEY_W/3;
         k_w += KEY_W/2;
      }
      else {
         k_x += (KEY_W + 3);      
      }
   }
   if (3 == m_k )  { // ligne operand
      if (x>1){
         k_x += 3 + (x-1) * KEY_W/4;      
      }
   }

   if ((k_x <0) | ((k_x + k_w) >= LCD_WIDTH)) return;
   if ((k_y <0) | ((k_y + KEY_H) >= LCD_HEIGHT)) return;

   if (release)  {
      LCD_FillRect( &hlcd, k_x, k_y, k_w, KEY_H, 0);
      LCD_DrawRect( &hlcd, k_x, k_y, k_w, KEY_H, 1);
   }
   else{ 
      LCD_FillRect( &hlcd, k_x, k_y, k_w, KEY_H, 1);
   }
}


__weak void DBx_Task_App(void){
   char result = 0;
   uint32_t t_lcd_buff =0, t_lcd_dma =0, t_mark =0;
   st_key_data drcvd;
   st_sys_data srcvd;

   int  key        = 0;
   uint32_t key_tmp=0, key_p1 =0, key_p2=0, key_p3=0, keybdata=0;
   bool transalpha = false;
   bool key_release = false;
   OS_TASKEVENT MyEvents;

   LCD_Status_t lcd_res = LCD_Sharp_Init(&hlcd, true);

   RTT_vprintf_cr_time( "LCD status : %s, add : %08x", LCD_Status_Desc[lcd_res], LCD_GetFramebuffer());

   snprintf(buff, sizeof(buff), "%s v%s %dMhz %dko Lcd %dx%d 49keys H",HARD_NAME, HARD_VERSION, SystemCoreClock/1000000, DB_MEM_SIZE, LCD_WIDTH, LCD_HEIGHT);

   display_text_12x24(&hlcd, 2, 4, buff,1);

//   snprintf(buff, sizeof(buff), "Kbd H 49keys");
//   display_text_12x24(&hlcd, 10, 52, buff,1);

   snprintf(buff, sizeof(buff), "-");
   display_text_12x24(&hlcd, 10, 76, buff,1);

   LCD_UpdateModifiedLines(&hlcd);
   for (int x = 1; x<=KB_COL ; x++)
      for (int y = 1; y<=KB_ROW ; y++)
         draw_key(x , y, 1);
//   LCD_UpdateModifiedLines(&hlcd);
   t_mark = rtc_read_ticks();
   LCD_UpdateModifiedLines(&hlcd);
   t_lcd_buff = rtc_elapsed_ticks(t_mark);
   LCD_WaitForTransfer(&hlcd);
   t_lcd_dma = rtc_elapsed_ticks(t_mark);


   OS_EVENT_Set(&EV_db_ready);

   while (1)
   {

      MyEvents = OS_TASKEVENT_GetSingleTimed( EV_DBx_KBD 
         | EV_DBx_RTC 
         | EV_DBx_USB_CON 
         | EV_DBx_USB_DIS
         | EV_DBx_RESET, KB_SCRUT_PERIOD);

      if (MyEvents)
      {
         LCD_FillRect( &hlcd, 10, 28, 250, 24, 0);
         snprintf(buff, sizeof(buff), "%03d.%03d [%02X] %d, %d", (rtc_read_ticks()/1000)%1000,rtc_read_ticks()%1000, MyEvents, t_lcd_buff, t_lcd_dma);
         display_text_12x24(&hlcd, 10, 28, buff,1);

uint32_t raw_ll = keybd.raw &  ((uint64_t)0xffffffff);
uint32_t raw_hh = keybd.raw >> ((uint64_t) 32);
   
   LCD_FillRect( &hlcd, 10, 76, 250, 24, 0);
   snprintf(buff, sizeof(buff), "%08x %08x ", raw_hh, raw_ll);
   display_text_12x24(&hlcd, 10, 76, buff,1);
      }

      if ( EV_DBx_KBD & MyEvents) 
      {
         while (      0 == OS_MAILBOX_GetTimed(&Mb_Keyboard, &drcvd, KB_SCRUT_PERIOD))
         { // key
            key_release = drcvd.released;
            RTT_vprintf_cr_time( "key %02d %02d %02d %02d, %02d, %02d, %02d %s %02d", 
               drcvd.key3, drcvd.key2, drcvd.key1, drcvd.key,
               key_p1, key_p2, key_p3, key_release ?"Rls":"Psh", key );
            draw_key( drcvd.key %10 , drcvd.key /10 , key_release);
         }
         LCD_UpdateModifiedLines(&hlcd);
      }
      else if ( EV_DBx_RESET & MyEvents)  
      { // message reset
         RTT_vprintf_cr_time( "Db48x reset");
         snprintf(buff, sizeof(buff), "[F1] [F6] [Exit] reset");
         LCD_FillRect( &hlcd, 10, 100, 180, 24, 0);
         display_text_12x24(&hlcd, 10, 100, buff,1);
         LCD_UpdateModifiedLines(&hlcd);
         while(1){}
      }
      else if ( EV_DBx_RTC & MyEvents)  
      { // message rtc
            RTT_vprintf_cr_time( "xxxDb48x wake-up from rtc");
            snprintf(buff, sizeof(buff), "xxxrtc : %d", keybd.sleeping_time_sec+1);
         LCD_FillRect( &hlcd, 10, 100, 180, 24, 0);
         display_text_12x24(&hlcd, 10, 100, buff,1);
         LCD_UpdateModifiedLines(&hlcd);

      }
      else if ( EV_DBx_USB_CON & MyEvents)  
      { // message usb, display new frequency
         int length= 12*  snprintf(buff, sizeof(buff), "USB_connected    ");
         
         LCD_FillRect( &hlcd, 10, 125, length, 24, 0);
         display_text_12x24(&hlcd, 10, 125, buff,1);
         snprintf(buff, sizeof(buff), "%s v%s %dMhz %dko Lcd %dx%d 49keys H",HARD_NAME, HARD_VERSION, SystemCoreClock/1000000, DB_MEM_SIZE, LCD_WIDTH, LCD_HEIGHT);
         LCD_FillRect( &hlcd, 2, 4, 520, 24, 0);
         display_text_12x24(&hlcd, 2, 4, buff,1);
         LCD_UpdateModifiedLines(&hlcd);
         OS_TASKEVENT_Set( &TUSB, EV_USB_ACQ);


      }

      else if ( EV_DBx_USB_DIS & MyEvents)  
      { // message usb, display new frequency
         int length= 12*  snprintf(buff, sizeof(buff), "USB_disconnected ");
         LCD_FillRect( &hlcd, 10, 125, length, 24, 0);
         display_text_12x24(&hlcd, 10, 125, buff,1);
         snprintf(buff, sizeof(buff), "%s v%s %dMhz %dko Lcd %dx%d 49keys H",HARD_NAME, HARD_VERSION, SystemCoreClock/1000000, DB_MEM_SIZE, LCD_WIDTH, LCD_HEIGHT);
         LCD_FillRect( &hlcd, 2, 4, 520, 24, 0);
         display_text_12x24(&hlcd, 2, 4, buff,1);
         LCD_UpdateModifiedLines(&hlcd);
         OS_TASKEVENT_Set( &TUSB, EV_USB_ACQ);
      }
   }
}
/*********************************************************************
*
*       main()
*/
void MainTask_Test_FS(void);


int main(void) {
  OS_Init();    // Initialize embOS
  OS_InitHW();  // Initialize required hardware

  init_unused_pins();   // analog input, pull-down
  BSP_Init();   // Initialize LED ports

   RTC_Config_1024_Granularity();
   RTC_SetBuildTime();
   bool res_bkpram = bkSRAM_Init();
   SEGGER_RTT_printf(0, "\nBackup sram : %s", res_bkpram ? "ok" : "initialized");

   bkSRAM_ReadString(1, buff, sizeof(buff));
   SEGGER_RTT_printf(0, "\nBkp string n1 : %s\n", buff);



   FS_Init();
   FS_FAT_SupportLFN();
   init_button_pc13();




   OS_MAILBOX_Create(&Mb_Keyboard, SIZE_DATA_KEYBOARD, MAX_MB_KEYBOARD, &Mb_KeyboardBuffer);
   OS_MAILBOX_Create(&Mb_Sys, SIZE_DATA_SYSTEM, MAX_MB_SYSTEM, &Mb_SysBuffer);

   OS_EVENT_Create(&EV_USB_Vbus);
   OS_EVENT_Create(&KBD_Event);
   OS_EVENT_Create(&PB_Event);
   OS_EVENT_Create(&USB_Event);
   OS_EVENT_Create(&DBu_Start);
   OS_EVENT_Create(&USB_Start);
   OS_EVENT_Create(&EV_db_ready);
   OS_EVENT_Create(&LED_Start);
   OS_EVENT_Create(&FOR_EVER);
   OS_EVENT_Create(&RTC_Event);


// Application task, weak for testing kbd
OS_TASK_CREATE(&TDB48X, "App task",  50, DBx_Task_App, StackDb48x); 
// testing fs
//OS_TASK_CREATE(&TTST_FS, "testing FS",  50, MainTask_Test_FS, StackTest_FS); 



   OS_TASK_CREATE(&TUSB, "Usb Task", 70, DBx_Task_Usb, StackUsb);



   OS_TASK_CREATE(&TKBD, "Kbd Task", 110, DBx_Task_Kbd, StackKbd);


// stop mode 2, in this task, or in keyboard
//#if !DEBUG 
//   OS_TASK_CREATE(&TLPW, "low power Task",  1, task_lpwr, StackLpwr);
//#endif

  OS_Start();   // Start embOS
  return 0;
}
