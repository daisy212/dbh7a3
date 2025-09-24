#ifndef H7A3_BKPRAM_H
#define H7A3_BKPRAM_H


#include "stm32h7xx.h"
#include "stdbool.h"
#include "stdint.h"
#include "RTOS.h"
#include <time.h>
#include <stdio.h>


#define BKPSRAMMAGIC (0x11d23546)
#define BKP_MAX_STR  (10)
#define BKP_MAX_I32  (256)


#define BKP_RAM __attribute__((section(".Backup_RAM1")))



bool bkSRAM_Init(void);
void bkSRAM_ReadString(uint16_t read_adress, char* read_data, uint32_t length);
void bkSRAM_WriteString(uint16_t read_adress, char* write_data, uint32_t length);
void bkSRAM_ReadVariable(uint16_t read_adress, uint32_t* read_data);
void bkSRAM_WriteVariable(uint16_t write_adress,uint32_t vall);
size_t    ui_read_setting(const char *name, char *value, size_t maxlen);
void      ui_save_setting(const char *name, const char *value);


#endif // H7A3_BKPRAM_H