/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2024  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : h7a3_FS_ConfigMMC.c
Purpose : Configuration functions for FS with MMC/SD card mode driver
Board nucleo H7A3ZIQ
*/

/*********************************************************************
*
*       #include section
*
**********************************************************************
*/

#include "RTOS.h"
#include "FS.h"
#include "FS_OS.h"
#include "SEGGER.h"
#include "FS_ConfDefaults.h"

#include "h7a3_FS_HW_MMC.h"
#include "h7a3_FS_ConfigMMC.h"
#include "h7a3_rtc.h"


//#include "..\FS\OS\FS_OS_embOS.c"

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static OS_RSEMA   * _paSema;
static OS_EVENT     _Event;
#if FS_SUPPORT_DEINIT
  static unsigned   _NumLocks;
#endif

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/
#ifndef   ALLOC_SIZE
  #define ALLOC_SIZE          0x2500      // Size defined in bytes
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

/*********************************************************************
*
*       Memory pool used for semi-dynamic allocation
*/
#ifdef __ICCARM__
  #pragma location="FS_RAM"
  static __no_init U32 _aMemBlock[ALLOC_SIZE / 4];
#endif
#ifdef __CC_ARM
  static U32 _aMemBlock[ALLOC_SIZE / 4] __attribute__ ((section ("FS_RAM"), zero_init));
#endif
#if (!defined(__ICCARM__) && !defined(__CC_ARM))
  static U32 _aMemBlock[ALLOC_SIZE / 4];
#endif

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_X_AddDevices
*
*  Function description
*    This function is called by the FS during FS_Init().
*    It is supposed to add all devices, using primarily FS_AddDevice().
*
*  Note
*    (1) Other API functions may NOT be called, since this function is called
*        during initialization. The devices are not yet ready at this point.
*/
void FS_OS_Init(unsigned NumLocks);

OS_MUTEX OS_RAM MTX_FS1, MTX_FS2;

void FS_X_AddDevices(void) {

//FS_OS_Init(10);
   OS_EVENT_Create(&_Event);
   OS_MUTEX_Create( &MTX_FS1);
   OS_MUTEX_Create( &MTX_FS2);


  //
  // Give file system memory to work with.
  //
  FS_AssignMemory(&_aMemBlock[0], sizeof(_aMemBlock));
  //
  // Add and configure the driver.
  //
  FS_AddDevice(&FS_MMC_CardMode_Driver);
  FS_MMC_CM_Allow4bitMode(0, 1);
  FS_MMC_CM_AllowHighSpeedMode(0, 1);
  FS_MMC_CM_SetHWType(0, &FS_MMC_HW_STM32H7A3_nucleo);
  //
  // Configure the file system for fast write operations.
  //
#if FS_SUPPORT_FILE_BUFFER
  FS_ConfigFileBufferDefault(512, FS_FILE_BUFFER_WRITE);
#endif
  FS_SetFileWriteMode(FS_WRITEMODE_FAST);
}

/*********************************************************************
*
*       FS_X_GetTimeDate
*
*  Function description
*    Current time and date in a format suitable for the file system.
*
*  Additional information
*    Bit 0-4:   2-second count (0-29)
*    Bit 5-10:  Minutes (0-59)
*    Bit 11-15: Hours (0-23)
*    Bit 16-20: Day of month (1-31)
*    Bit 21-24: Month of year (1-12)
*    Bit 25-31: Count of years from 1980 (0-127)
*/
U32 FS_X_GetTimeDate(void) 
// Rtc read for file system time
{
   U32 r;
   U16 Sec, Min, Hour;
   U16 Day, Month, Year;

   RTC_TimeTypeDef      time_rtc;
   RTC_DateTypeDef      date_rtc;
// allway together !!!!!!
   HAL_RTC_GetTime(&hrtc, &time_rtc, RTC_FORMAT_BIN);
   HAL_RTC_GetDate(&hrtc, &date_rtc, RTC_FORMAT_BIN);

   Sec   = time_rtc.Seconds;     // 0 based.  Valid range: 0..59
   Min   = time_rtc.Minutes;     // 0 based.  Valid range: 0..59
   Hour  = time_rtc.Hours;       // 0 based.  Valid range: 0..23
   Day   = date_rtc.Date;        // 1 based.    Means that 1 is 1. Valid range is 1..31 (depending on month)
   Month = date_rtc.Month;       // 1 based.    Means that January is 1. Valid range is 1..12.
   Year  = date_rtc.Year + 20;      // 1980 based. Means that 2007 would be 27.
   r   = Sec / 2 + (Min << 5) + (Hour  << 11);
   r  |= (U32)(Day + (Month << 5) + (Year  << 9)) << 16;
   return r;
}








/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       FS_X_OS_Lock
*
*  Function description
*    Acquires the specified OS resource.
*
*  Parameters
*    LockIndex    Identifies the OS resource (0-based).
*
*  Additional information
*    This function has to block until it can acquire the OS resource.
*    The OS resource is later released via a call to FS_X_OS_Unlock().

undefined symbol FS_X_OS_Lock referenced by symbol FS_OS_Lock (section .text.FS_OS_Lock in file FS_OS_Interface.o (libFS_v7m_t_vfpv4_le_r.a))
*/

void FS_OS_Lock_ (unsigned LockIndex) {
//void  FS_X_OS_Lock(unsigned LockIndex) {
  OS_RSEMA * pSema;

  pSema = _paSema + LockIndex;
  (void)OS_Use(pSema);
//  FS_DEBUG_LOG((FS_MTYPE_OS, "OS: LOCK Index: %d\n", LockIndex));
}

void FS_OS_Lock (unsigned LockIndex) {

   if ( 0 == LockIndex)
      OS_MUTEX_LockBlocked(&MTX_FS1);
   if ( 1 == LockIndex)
      OS_MUTEX_LockBlocked(&MTX_FS2);

}
/*********************************************************************
*
*       FS_X_OS_Unlock
*
*  Function description
*    Releases the specified OS resource.
*
*  Parameters
*    LockIndex    Identifies the OS resource (0-based).
*
*  Additional information
*    The OS resource to be released was acquired via a call to FS_X_OS_Lock()
*/
void FS_OS_Unlock_(unsigned LockIndex) {
  OS_RSEMA * pSema;

  pSema = _paSema + LockIndex;
  //FS_DEBUG_LOG((FS_MTYPE_OS, "OS: UNLOCK Index: %d\n", LockIndex));
  OS_Unuse(pSema);
}

void FS_OS_Unlock(unsigned LockIndex) {
   if ( 0 == LockIndex)
      OS_MUTEX_Unlock(&MTX_FS1);
   if ( 1 == LockIndex)
      OS_MUTEX_Unlock(&MTX_FS2);

}
/*********************************************************************
*
*       FS_X_OS_Init
*
*  Function description
*    Initializes the OS resources.
*
*  Parameters
*    NumLocks   Number of locks that should be created.
*
*  Additional information
*    This function is called by FS_Init(). It has to create all resources
*    required by the OS to support multi tasking of the file system.
*/
void FS_OS_Init(unsigned NumLocks) {}

void FS_OS_Init_(unsigned NumLocks) {
  unsigned   i;
  OS_RSEMA * pSema;
  unsigned   NumBytes;

  NumBytes = NumLocks * sizeof(OS_RSEMA);
//  _paSema = MTX_FS;
  //SEGGER_PTR2PTR(OS_RSEMA, FS_ALLOC_ZEROED((I32)NumBytes, "OS_RSEMA"));
  pSema =_paSema;
  for (i = 0; i < NumLocks; i++) {
    OS_CREATERSEMA(pSema++);
  }
  OS_EVENT_Create(&_Event);
#if FS_SUPPORT_DEINIT
  _NumLocks = NumLocks;
#endif
}

#if FS_SUPPORT_DEINIT

/*********************************************************************
*
*       FS_X_OS_DeInit
*
*  Function description
*    This function has to release all the resources that have been
*    allocated by FS_X_OS_Init().
*/
void FS_OS_DeInit_(void) {
  unsigned   i;
  OS_RSEMA * pSema;
  unsigned   NumLocks;

  NumLocks = _NumLocks;
  pSema   = &_paSema[0];
  for (i = 0; i < NumLocks; i++) {
    OS_DeleteRSema(pSema);
    pSema++;
  }
  OS_EVENT_Delete(&_Event);
  FS_Free(_paSema);
  _paSema  = NULL;
  _NumLocks = 0;
}

#endif // FS_SUPPORT_DEINIT



/*********************************************************************
*
*       FS_X_OS_Wait
*
*  Function description
*    Waits for an event to be signaled.
*
*  Parameters
*    Timeout  Time to be wait for the event object.
*
*  Return value
*    ==0      Event object was signaled within the timeout value
*    !=0      An error or a timeout occurred.
*/
int FS_X_OS_Wait(int TimeOut) {
  int r;

  r = -1;
  if ((U8)OS_EVENT_WaitTimed(&_Event, TimeOut) == 0u) {
    r = 0;
  }
  return r;
}

/*********************************************************************
*
*       FS_X_OS_Signal
*
*  Function description
*    Signals an event.
*/
void FS_X_OS_Signal(void) {
  OS_EVENT_Set(&_Event);
}

/*********************************************************************
*
*       FS_X_OS_Delay
*
*  Function description
*    Blocks the execution for the specified number of milliseconds.
*/
void FS_X_OS_Delay(int ms) {
  OS_Delay(ms + 1);
}











/*************************** End of file ****************************/
