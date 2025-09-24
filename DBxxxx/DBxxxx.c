#include "DBxxxx.h"



int       ui_file_selector(const char *title,
                           const char *base_dir,
                           const char *ext,
                           file_sel_fn callback,
                           void       *data,
                           int         disp_new,
                           int         overwrite_check){
                     
   RTT_vprintf_cr_time( "ui_file selector : %s, %s, %s", title, base_dir, ext);
}                    






int key_pop_last();
int key_pop();

int runner_get_key_(int *repeat)
{

    return repeat ? key_pop_last() :  key_pop();
}


void rtc_write(tm_t * tm, dt_t *dt)
// to do
{
    RTT_vprintf_cr_time( "Writing RTC %u/%u/%u %u:%u:%u (ignored)",
           dt->day, dt->month, dt->year,
           tm->hour, tm->min, tm->sec);


}


void rtc_read(tm_t * tm, dt_t *dt)
{
    time_t           now;
    struct tm        utm;
    struct timeval      tv;

/*  utilisation des fonction std avec  __SEGGER_RTL_X_get_time_of_day() déclarée
         corruption mémoire ???

   time(&now);
    localtime_r(&now, &utm);
    gettimeofday(&tv, NULL);
*/
   RTC_TimeTypeDef      time_rtc;
   RTC_DateTypeDef      date_rtc;
// toujours ensemble !!!!!!
   HAL_RTC_GetTime(&hrtc, &time_rtc, RTC_FORMAT_BIN);
   HAL_RTC_GetDate(&hrtc, &date_rtc, RTC_FORMAT_BIN);
   uint32_t prescaler = hrtc.Init.SynchPrediv + 1;
   uint32_t msec = (1000000 - ((time_rtc.SubSeconds * 1000000) / prescaler))/1000;

// no prescaler with u585, 
// using embOs ?



// Fill the struct tm utm
    utm.tm_sec  = time_rtc.Seconds;
    utm.tm_min  = time_rtc.Minutes;
    utm.tm_hour = time_rtc.Hours;
    utm.tm_mday = date_rtc.Date;
    utm.tm_mon  = date_rtc.Month - 1;   // struct tm month range: 0-11
    utm.tm_year = date_rtc.Year + 100;  // struct tm year = years since 1900 (assuming 2000-based RTC)
    utm.tm_wday = date_rtc.WeekDay % 7; // tm_wday: Sunday = 0, RTC: Monday = 1

// time_t epoch_time = mktime(&utm);


// result setting
    dt->year = 1900 + utm.tm_year;
    dt->month = utm.tm_mon + 1;
    dt->day = utm.tm_mday;

    tm->hour = utm.tm_hour;
    tm->min = utm.tm_min;
    tm->sec = utm.tm_sec;
    tm->csec = msec/10;
 //   tm->csec = tv.tv_usec/10000;
    tm->dow = (utm.tm_wday + 6) % 7;
/*
 uint   ticks = ((tm->sec +
             tm->min  * 60 +
             tm->hour * 3600 +
              tm->dow * 86400) * 100 + tm->csec) * 10;

   SEGGER_RTT_printf(0, "\nTub time : prescaler %03X, sub sec : %03x, ticks : %d", prescaler, time_rtc.SubSeconds, ticks);
*/
}
