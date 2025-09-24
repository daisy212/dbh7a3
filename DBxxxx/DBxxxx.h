#ifndef DBxxxx_H
#define DBxxxx_H

#include "stdbool.h"
#include "stdint.h"
#include <time.h>

#include "stm32h7xx.h"

#include "RTOS.h"
#include "FS.h"
#include "FS_OS.h"
#include "SEGGER_RTT.h" // using SEGGER_RTT_printf

#include "h7a3_usb.h"
#include "h7a3_rtc.h"
#include "h7a3_kbd.h"
#include "h7a3_bkpram.h"
#include "h7a3_low_power.h"
#include "h7a3_Sharp_LS.h"

#include "DBx_tsk_pw_kbd.h"



// auto repeat time in msec for db48x
#define KB_DB_REPEAT_FIRST (1000)
#define KB_DB_REPEAT_PERIOD (100)

// events mask for DB48x
#define EV_DBx_KBD      (0x1<<0)
#define EV_DBx_SYS      (0x2)
#define EV_DBx_RTC      (0x4)
#define EV_DBx_USB_CON      (0x8)
#define EV_DBx_USB_DIS      (0x10)
#define EV_DBx_RESET    (0x20)
#define EV_DBx_POFF     (0x40)
#define EV_DBx_PON     (0x80)


#define EV_USB_ACQ     (0x1<<0)




typedef struct {
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
} dt_t;

typedef struct {
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t csec;
  uint8_t dow;
} tm_t;


typedef int (*file_sel_fn)(const char *fpath, const char *fname, void *data);

int       ui_file_selector(const char *title,
                           const char *base_dir,
                           const char *ext,
                           file_sel_fn callback,
                           void       *data,
                           int         disp_new,
                           int         overwrite_check);

void rtc_read(tm_t * tm, dt_t *dt);
void rtc_write(tm_t * tm, dt_t *dt);



extern uint32_t db_calc_state;
extern    bool   StopMode2_disable;



#define calc_state      (db_calc_state)




extern OS_EVENT        KBD_Event,  DBu_Start, USB_Start, LED_Start, EV_db_ready, FOR_EVER, RTC_Event, PB_Event;
extern OS_MAILBOX     Mb_Keyboard;
extern OS_TASK     TKBD, TDB48X;

#endif      // DBxxxx_H


