#include "h7a3_bkpram.h"
#include "h7a3_rtc.h"

/* en attente de connexion vbat */


char     BKP_RAM  Bkp_Strg[BKP_MAX_STR][256];
int32_t  BKP_RAM  Bkp_i32[BKP_MAX_I32];


bool bkSRAM_Init(void)
{
   HAL_FLASH_Unlock();
   /*DBP : Enable access to Backup domain */
   HAL_PWR_EnableBkUpAccess();
   __HAL_RCC_BKPRAM_CLK_ENABLE();
    /*BRE : Enable backup regulator
      BRR : Wait for backup regulator to stabilize */
    HAL_PWREx_EnableBkUpReg();

   /*DBP : Disable access to Backup domain */
    __HAL_RCC_BKPRAM_CLK_DISABLE();
    HAL_PWR_DisableBkUpAccess();
    HAL_FLASH_Lock();


   uint32_t magic_check = 0;
   bkSRAM_ReadVariable(0, &magic_check);
//   if (BKPSRAMMAGIC != magic_check)
   if (1) // battery missing
   {
// init backup sram, 
      char buff[256] = {0};
      bkSRAM_WriteVariable( 0, BKPSRAMMAGIC);
      snprintf(buff, sizeof(buff), "state\\State1.48s");
      bkSRAM_WriteString(1, buff, sizeof(buff));
// ajouter la timezone
      return 0;
   }
   return 1;
}

void bkSRAM_ReadString(uint16_t read_adress, char* read_data, uint32_t length){
snprintf(read_data, length,"state\\state1.48s");
}
void bkSRAM_ReadString_(uint16_t read_adress, char* read_data, uint32_t length)
{
   uint32_t siz = length;
   if (length >256) siz = 256;
   if (read_adress >= BKP_MAX_STR) 
      return ;
   HAL_PWR_EnableBkUpAccess();
   __HAL_RCC_BKPRAM_CLK_ENABLE();

   memcpy( read_data, (void *)(&Bkp_Strg[read_adress][0]), siz);

   __HAL_RCC_BKPRAM_CLK_DISABLE();
   HAL_PWR_DisableBkUpAccess();
}

void bkSRAM_WriteString(uint16_t read_adress, char* write_data, uint32_t length)
{
   uint32_t siz = length;
   if (length >256) siz = 256;
   if ( read_adress >= BKP_MAX_STR )
      return;
   HAL_PWR_EnableBkUpAccess();
   __HAL_RCC_BKPRAM_CLK_ENABLE();
// char * pstring =  (char*) (0x38800000 + read_adress*0x100);
   memcpy( (void *)(&Bkp_Strg[read_adress][0]), write_data, siz);
   __HAL_RCC_BKPRAM_CLK_DISABLE();
   HAL_PWR_DisableBkUpAccess();
}

void bkSRAM_ReadVariable(uint16_t read_adress, uint32_t* read_data)
{
   if (read_adress >= BKP_MAX_I32) return;
       HAL_PWR_EnableBkUpAccess();
       __HAL_RCC_BKPRAM_CLK_ENABLE();
       *read_data =  Bkp_i32[ read_adress];
        __HAL_RCC_BKPRAM_CLK_DISABLE();
        HAL_PWR_DisableBkUpAccess();
}

void bkSRAM_WriteVariable(uint16_t write_adress,uint32_t vall)
{
   if (write_adress >= BKP_MAX_I32) return;
   HAL_PWR_EnableBkUpAccess();
   __HAL_RCC_BKPRAM_CLK_ENABLE();
   Bkp_i32[ write_adress] = vall ;
   SCB_CleanDCache_by_Addr( &Bkp_i32[ write_adress],8);
   __HAL_RCC_BKPRAM_CLK_DISABLE();
   HAL_PWR_DisableBkUpAccess();
}



const char cmd1[][16] = {"nothing", "state", "last"};
uint32_t get_index(  const char *name){
   for (uint32_t ii = 0; ii < sizeof(cmd1)/ sizeof(cmd1[0]); ii++){
      if (strncmp(name, cmd1[ii], sizeof(*name)) == 0 ) return ii;
   }
   return 0;
}                 

void      ui_save_setting(const char *name, const char *value){
   char buff[60];
   uint32_t index= get_index(name);
   if (index != 0) 
      bkSRAM_WriteString( index, (char *)value, sizeof(value));

   snprintf(buff, sizeof(buff), "\nui_save_setting : %s, %s", cmd1[index], value);
   RTT_vprintf_cr_time( "Bkp %s",buff);

//   SEGGER_RTT_WriteString(0,  buff);
}


size_t    ui_read_setting(const char *name, char *value, size_t maxlen){
   char buff[60];
   uint32_t index= get_index(name);
   if (index != 0) {
      bkSRAM_ReadString(1, value, maxlen);
      return sizeof(value);
   }
   return 0;
}


