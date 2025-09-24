/*#include "h7a3_rtc.h"
#include "h7a3_kbd.h"
#include "h7a3_bkpram.h"
#include "h7a3_low_power.h"
#include "h7a3_Sharp_LS.h"

#include "SEGGER_RTT.h"
#include "FS.h"
*/


#include "DBxxxx.h"



extern uint8_t       wakeup_from;
extern OS_EVENT      RTC_Event, PB_Event, KBD_Event, USB_Event, DBu_Start, USB_Start, LED_Start, EV_db_ready, FOR_EVER;
extern OS_MAILBOX    Mb_Keyboard;


RTC_HandleTypeDef hrtc;


time_t  time_t_week = 0;
uint32_t time_t_w_calc_t = 0;


void RTC_WKUP_IRQHandler(void){
   HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
//   RTT_vprintf_cr_time( "Rtc irq");
   
   OS_INT_Enter();
   OS_EVENT_Set(&RTC_Event);
   OS_EVENT_Set(&KBD_Event);
//   st_key_data dts;
//   dts.sys = 1;
//   dts.cmd.sys_cmd = SYS_int_rtc;
//   dts.cmd.sys_data = 0;
//   OS_MAILBOX_Put(&Mb_Keyboard, &dts);  

   wakeup_from =2;
   OS_INT_Leave();

}



/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
  void    Error_Handler(int err_no);

void MX_RTC_Init(void)
/**
  * @brief  Configure RTC with 1024 granularity
  * @note   For 1024 granularity, we need PREDIV_S = 1023 (1024-1)
  *         With LSE = 32.768 kHz, we configure:
  *         - PREDIV_A = 31 (32-1) for async prescaler
  *         - PREDIV_S = 1023 (1024-1) for sync prescaler
  *         This gives: 32768 / (32 * 1024) = 1 Hz calendar clock
  *         And SubSeconds will have 1024 steps (0-1023)
  * @retval None
  */
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 31;
  hrtc.Init.SynchPrediv = 1023;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler(1);
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}



/**
  * @brief RTC MSP Initialization
  * This function configures the hardware resources used in this example
  * @param hrtc: RTC handle pointer
  * @retval None
  */
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  if(hrtc->Instance==RTC)
  {
    /* USER CODE BEGIN RTC_MspInit 0 */

    /* USER CODE END RTC_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler(127);
    }

    /* Peripheral clock enable */
    __HAL_RCC_RTC_ENABLE();
//    __HAL_RCC_RTCAPB_CLK_ENABLE();
//    __HAL_RCC_RTCAPB_CLKAM_ENABLE();
    /* USER CODE BEGIN RTC_MspInit 1 */

    /* USER CODE END RTC_MspInit 1 */

  }

}


/**
  * @brief  Configure RTC with 1024 granularity
  * @note   For 1024 granularity, we need PREDIV_S = 1023 (1024-1)
  *         With LSE = 32.768 kHz, we configure:
  *         - PREDIV_A = 31 (32-1) for async prescaler
  *         - PREDIV_S = 1023 (1024-1) for sync prescaler
  *         This gives: 32768 / (32 * 1024) = 1 Hz calendar clock
  *         And SubSeconds will have 1024 steps (0-1023)
  * @retval None
  */
void RTC_Config_1024_Granularity(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /* Enable access to backup domain */
  HAL_PWR_EnableBkUpAccess();

  /* Configure LSE Drive Capability */
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /* Initializes the RCC Oscillators according to the specified parameters */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(1);
  }

  /* Select LSE as RTC clock source */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler(1);
  }

  /* Enable RTC Clock */
  __HAL_RCC_RTC_ENABLE();

  /* Initialize RTC Only */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  
  /* Configure prescalers for 1024 granularity */
  /* With LSE = 32.768 kHz:
   * - Asynchronous prescaler = 31 (division by 32)
   * - Synchronous prescaler = 1023 (division by 1024)
   * - Result: 32768 / (32 * 1024) = 1 Hz for calendar
   * - SubSeconds will count from 1023 down to 0, providing 1024 steps
   */
  hrtc.Init.AsynchPrediv = 31;   /* 32-1 */
  hrtc.Init.SynchPrediv = 1023;  /* 1024-1 */
  
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler(1);
  }
}




uint32_t rtc_read_ticks(void)
// getting time for os in tickless mode in msec, maximun value a week in msec
{
   RTC_TimeTypeDef      time_rtc;
   RTC_DateTypeDef      date_rtc;

// allways together !!!!!!
   HAL_RTC_GetTime(&hrtc, &time_rtc, RTC_FORMAT_BIN);
   HAL_RTC_GetDate(&hrtc, &date_rtc, RTC_FORMAT_BIN);
   uint32_t prescaler = hrtc.Init.SynchPrediv + 1;
   uint32_t msec = (1000000 - ((time_rtc.SubSeconds * 1000000) / prescaler))/1000;
// prescaler = 1024

   uint32_t   ticks =   (time_rtc.Seconds +
                        time_rtc.Minutes  * 60 +
                        time_rtc.Hours * 3600 +
                        ((date_rtc.WeekDay+6) % 7) * 86400) * 1000 + msec;
   return ticks;
}


uint32_t rtc_elapsed_ticks(uint32_t rtc_ticks)
// returning time elapsed since the argument in msec
{
   uint32_t rtc_c_ticks = rtc_read_ticks();
   if (rtc_c_ticks < rtc_ticks) 
      return (rtc_c_ticks + (24*3600*7 )*1000) - rtc_ticks;
   else
      return rtc_c_ticks - rtc_ticks;
}



void RTT_vprintf_cr_time( const char * sFormat, ...)
// debug message, deleted in release mode
{
#if DEBUG
   va_list args;
   va_start(args, sFormat);

   SEGGER_RTT_printf(0, "\n%03d.%03d : ", (rtc_read_ticks()/1000)%1000,rtc_read_ticks()%1000);
   SEGGER_RTT_vprintf(0, sFormat, &args);
#endif // DEBUG
}


void RTT_vprintf_cr_time_fct( const char * sFctName, const char * sFormat, ...)
// debug message, deleted in release mode
{
#if DEBUG
   va_list args;
   va_start(args, sFormat);

   SEGGER_RTT_printf(0, "\n%03d.%03d [%s] ", (rtc_read_ticks()/1000)%1000,rtc_read_ticks()%1000, sFctName);
   SEGGER_RTT_vprintf(0, sFormat, &args);
#endif // DEBUG
}



/**
 * @brief Check if RTC is already configured
 * @retval 1 if configured, 0 if not configured
 */
uint8_t RTC_IsConfigured(void)
{
    return (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0) == RTC_MAGIC_NUMBER) ? 1 : 0;
}






// Helper to convert month string to number (e.g., "Jun" → 6)
static uint8_t month_str_to_num(const char *month_str) {
    const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    for (int i = 0; i < 12; i++) {
        if (strncmp(month_str, months[i], 3) == 0) {
            return i + 1;
        }
    }
    return 1; // Default to January if something fails
}

void RTC_SetBuildTime(void) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Extract time from __TIME__ = "HH:MM:SS"
    int hour, minute, second;
    sscanf(__TIME__, "%2d:%2d:%2d", &hour, &minute, &second);

    // Extract date from __DATE__ = "Mmm dd yyyy"
    char month_str[4];
    int day, year;
    sscanf(__DATE__, "%3s %2d %4d", month_str, &day, &year);


    // Create a struct tm in local compilation time
    struct tm t = {
        .tm_year = year - 1900,
        .tm_mon  = month_str_to_num(month_str) - 1,
        .tm_mday = day,
        .tm_hour = hour,
        .tm_min  = minute,
        .tm_sec  = second,
        .tm_isdst = -1
    };

    time_t local_time = mktime(&t);

     int offset_min = get_city_utc_offset("Paris", t.tm_year, t.tm_mon, t.tm_mday);

      time_t utc_time = local_time - offset_min*60;

 struct tm *pt =  gmtime(&utc_time);

    // Fill RTC time structure
    sTime.Hours   = pt->tm_hour;
    sTime.Minutes = pt->tm_min;
    sTime.Seconds = pt->tm_sec;
    sTime.TimeFormat = RTC_HOURFORMAT_24;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    
    // Fill RTC date structure
    sDate.Year = pt->tm_year % 100;  // RTC expects year in 2-digit format
    sDate.Month = pt->tm_mon+1;
    sDate.Date = pt->tm_mday;
    sDate.WeekDay = pt->tm_wday;  // You can calculate this if needed

    // Apply the time and date
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
       // Error_HandlerMsg("RtcInit");
    }

    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
      //  Error_HandlerMsg("RtcInit");
    }

   /* Write backup register to indicate RTC is configured */
   HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RTC_MAGIC_NUMBER);

}






time_t rtc_read_time_t(char *pCity)
// getting time for os in tickless mode
{
   struct tm timeinfo;
   RTC_TimeTypeDef      sTime;
   RTC_DateTypeDef      sDate;

   // toujours ensemble !!!!!!
   HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
   HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

   /* fill tm structure */
   timeinfo.tm_year = (sDate.Year) + 100;  /* Years since 1900 (20xx -> 1xx) */
   timeinfo.tm_mon = (sDate.Month) - 1;    /* Months since January (0-11) */
   timeinfo.tm_mday = (sDate.Date);        /* Day of the month (1-31) */
   timeinfo.tm_hour = (sTime.Hours);       /* Hours (0-23) */
   timeinfo.tm_min = (sTime.Minutes);      /* Minutes (0-59) */
   timeinfo.tm_sec = (sTime.Seconds);      /* Seconds (0-59) */
   timeinfo.tm_wday = 0;   /* Not used by mktime */
   timeinfo.tm_yday = 0;   /* Not used by mktime */
   timeinfo.tm_isdst = -1; /* Let mktime determine DST */


   int offset_min = (pCity != NULL) ? get_city_utc_offset(pCity, timeinfo.tm_year, timeinfo.tm_mon, timeinfo.tm_mday) : 0;

   /* Convert to time_t */
   return mktime(&timeinfo)+60*offset_min;

}




/* test ??? */
// Helper: check for leap year
bool is_leap(int year) {
    return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

// Get day of the week (0=Sunday, 6=Saturday) using Zeller's Congruence
int day_of_week(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year -= 1;
    }
    int k = year % 100;
    int j = year / 100;
    return (day + 13*(month+1)/5 + k + k/4 + j/4 + 5*j) % 7;
}

// Find last Sunday of a month
int last_sunday(int year, int month) {
    int days_in_month[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (month == 2 && is_leap(year)) days_in_month[1] = 29;

    int last_day = days_in_month[month - 1];
    for (int d = last_day; d >= last_day - 6; d--) {
        if (day_of_week(year, month, d) == 0)
            return d;
    }
    return -1; // Should not happen
}

// Find nth Sunday of a month
int nth_sunday(int year, int month, int nth) {
    int count = 0;
    for (int d = 1; d <= 31; d++) {
        if (day_of_week(year, month, d) == 0) {
            count++;
            if (count == nth)
                return d;
        }
    }
    return -1; // Should not happen
}

// Date comparison as day of year
int day_of_year(int year, int month, int day) {
    int days_in_month[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if (is_leap(year)) days_in_month[1] = 29;

    int doy = 0;
    for (int i = 0; i < month - 1; i++)
        doy += days_in_month[i];
    return doy + day;
}

// DST rules
bool is_dst_europe(int year, int month, int day) {
    int start_day = last_sunday(year, 3);
    int end_day   = last_sunday(year, 10);
    int doy = day_of_year(year, month, day);

    int start_doy = day_of_year(year, 3, start_day);
    int end_doy   = day_of_year(year, 10, end_day);

    return (doy >= start_doy && doy < end_doy);
}

bool is_dst_us(int year, int month, int day) {
    int start_day = nth_sunday(year, 3, 2);  // Second Sunday in March
    int end_day   = nth_sunday(year, 11, 1); // First Sunday in November
    int doy = day_of_year(year, month, day);

    int start_doy = day_of_year(year, 3, start_day);
    int end_doy   = day_of_year(year, 11, end_day);

    return (doy >= start_doy && doy < end_doy);
}



typedef struct {
    const char *city;
    const char *region;
    int std_offset_min;
    int dst_offset_min;
    bool (*is_dst_func)(int year, int month, int day);
} CityTimeZone;

CityTimeZone cities[] = {
    // Central European Time (UTC+1/+2)
    {"Paris",     "CET",  60, 60, is_dst_europe},
    {"Berlin",    "CET",  60, 60, is_dst_europe},
    {"Warsaw",    "CET",  60, 60, is_dst_europe},
    {"Rome",      "CET",  60, 60, is_dst_europe},
    {"Madrid",    "CET",  60, 60, is_dst_europe},
    {"Vienna",    "CET",  60, 60, is_dst_europe},
    {"Brussels",  "CET",  60, 60, is_dst_europe},
    {"Amsterdam", "CET",  60, 60, is_dst_europe},
    {"Prague",    "CET",  60, 60, is_dst_europe},
    {"Budapest",  "CET",  60, 60, is_dst_europe},
    {"Zagreb",    "CET",  60, 60, is_dst_europe},

    // Eastern European Time (UTC+2/+3)
    {"Sofia",     "EET", 120, 60, is_dst_europe},
    {"Bucharest", "EET", 120, 60, is_dst_europe},
    {"Athens",    "EET", 120, 60, is_dst_europe},
    {"Helsinki",  "EET", 120, 60, is_dst_europe},
    {"Vilnius",   "EET", 120, 60, is_dst_europe},
    {"Riga",      "EET", 120, 60, is_dst_europe},
    {"Tallinn",   "EET", 120, 60, is_dst_europe},

    // Western European Time (UTC+0/+1)
    {"Lisbon",    "WET",   0, 60, is_dst_europe},
    {"Dublin",    "WET",   0, 60, is_dst_europe},

    // Tokyo (no DST)
    {"Tokyo",     "JST", 540,  0, NULL},

    // US Zones
    {"New York",  "EST", -300, 60, is_dst_us},
    {"Chicago",   "CST", -360, 60, is_dst_us},
    {"Los Angeles", "PST", -480, 60, is_dst_us},

    {NULL, NULL, 0, 0, NULL}
};


int get_city_utc_offset(const char *city_name, int year, int month, int day) {
    for (int i = 0; cities[i].city != NULL; i++) {
        if (strcmp(cities[i].city, city_name) == 0) {
            int offset = cities[i].std_offset_min;
            if (cities[i].is_dst_func && cities[i].is_dst_func(year, month, day))
                offset += cities[i].dst_offset_min;
            return offset;
        }
    }
    return 0; // Default UTC
}


void RtcIntXsec(uint32_t t_sec)
// raise an interrupt at the next second + t_sec
{
   HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
   HAL_PWR_EnableBkUpAccess();
   HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
   while ((RTC->ICSR & RTC_ICSR_WUTWF) == 0) {}
   __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
   HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, t_sec, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);     //T-1sec
   HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0x08, 0);
   HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
}

void RtcIntDisable(void)
{
   HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
   HAL_PWR_EnableBkUpAccess();
   HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
   while ((RTC->ICSR & RTC_ICSR_WUTWF) == 0) {}
   __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
}



