#ifndef H7A3_RTC_H
#define H7A3_RTC_H

#include "stdbool.h"
#include "stdint.h"
#include "RTOS.h"
#include <time.h>
#include <stdio.h>


#include "stm32h7xx.h"


#define RTC_MAGIC_NUMBER   (0x32F2)



extern time_t  time_t_week;
extern uint32_t time_t_w_calc_t;

extern RTC_HandleTypeDef hrtc;


//void MX_RTC_Init(void);

void RTC_Config_1024_Granularity(void);
uint32_t rtc_read_ticks(void);
uint32_t rtc_elapsed_ticks(uint32_t rtc_ticks);


//   #define RECORD(rec, fmt, ...)   do { dbxxxx_recorder( #rec, fmt, ##__VA_ARGS__); } while(0)
// #define record(...)             RECORD(__VA_ARGS__)


void RTT_vprintf_cr_time( const char * sFormat, ...);
void RTT_vprintf_cr_time_fct( const char * sFctName, const char * sFormat, ...);
#define RTT_vprintf_cr_T_F(...)    RTT_vprintf_cr_time_f(__VA_ARGS__)

#define RTT_vprintf_cr_time_f(rec, fmt, ...)    do {RTT_vprintf_cr_time_fct(#rec, fmt, ##__VA_ARGS__);} while(0)


void RTC_SetBuildTime(void);
uint8_t RTC_IsConfigured(void);
time_t rtc_read_time_t(char *pCity);

int get_city_utc_offset(const char *city_name, int year, int month, int day);


void RtcIntDisable(void);
void RtcIntXsec(uint32_t t_sec);


/* to do

bool bkSRAM_Init(void);
void bkSRAM_ReadString(uint16_t read_adress, char* read_data, uint32_t length);
void bkSRAM_WriteString(uint16_t read_adress, char* write_data, uint32_t length);
void bkSRAM_ReadVariable(uint16_t read_adress, uint32_t* read_data);
void bkSRAM_WriteVariable(uint16_t write_adress,uint32_t vall);


char buffer[256];

strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", &your_tm);

gmtime : Convert time_t to tm as UTC time
tm : struct
Créer une variable time_t of the week, uint32_t rtc_read_ticks(void) giving time in msec during the week


*/



#endif // H7A3_RTC_H
