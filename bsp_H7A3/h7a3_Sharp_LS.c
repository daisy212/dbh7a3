#include "h7a3_Sharp_LS.h"
#include "h7a3_low_power.h"
#include "h7a3_rtc.h"


#include "SEGGER_RTT.h"


/* LCD Display SHARP LH032 // LH027 */



//#define DEBUG_LCD 1
#define MIRROR 1


/* Static memory allocation */
static uint8_t LCD_RAM lcd_framebuffer[LCD_TOTAL_BYTES] = {0};
static uint8_t LCD_RAM lcd_dma_buffer[LCD_MAX_DMA_SIZE] = {0};

SPI_HandleTypeDef   OS_RAM hspi1;
DMA_HandleTypeDef OS_RAM hdma_spi1_tx;
#define HSPILCD            hspi1
LCD_Handle_t     OS_RAM       hlcd;
static LCD_Handle_t OS_RAM *g_hlcd = &hlcd; // Global handle for DMA callback


/* Private function prototypes */
static LCD_Status_t LCD_SendCommand(LCD_Handle_t *hlcd, uint8_t cmd);
static uint16_t LCD_BuildDMABuffer_27(LCD_Handle_t *hlcd, uint8_t *buffer);
static uint16_t LCD_BuildDMABuffer_32(LCD_Handle_t *hlcd, uint8_t *buffer);
static void LCD_UpdateModifiedRange(LCD_Handle_t *hlcd, uint16_t line);
LCD_Status_t SHARP_SPI1_Init(LCD_Handle_t *p_hlcd);


/* debugging status messages */
const char LCD_Status_Desc[LCD_LAST][LCD_MESS_STATUS_LENGTH] = {
    "ok",
    "ERROR",
    "BUSY",
    "TIMEOUT",
    "DMA1",
    "DMA2"
};


/**
 * @brief Initialize the LCD driver
 * @param hlcd: LCD handle
 * @param hspi: SPI handle (must be pre-configured)
 * @param use_dma: Enable DMA transfers
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_Sharp_Init(LCD_Handle_t *p_hlcd, bool use_dma)
{
   if (!p_hlcd) {
      return LCD_ERROR;
   }
   p_hlcd->spi_off = true;
   SHARP_SPI1_Init(p_hlcd);

    /* Initialize handle */
    memset(p_hlcd, 0, sizeof(LCD_Handle_t));
    p_hlcd->config.hspi = &HSPILCD;
    p_hlcd->config.use_dma = use_dma;
    p_hlcd->config.timeout_ms = 1000;
    p_hlcd->vcom_state = false;
    p_hlcd->transfer_complete = true;

    /* Initialize modified lines tracking */
    memset(&p_hlcd->modified, 0, sizeof(LCD_ModifiedLines_t));
    p_hlcd->modified.first_modified = LCD_HEIGHT;
    p_hlcd->modified.last_modified = 0;

    /* Set global handle for DMA callback */
    g_hlcd = p_hlcd;

    /* Verify SPI configuration */
    if (p_hlcd->config.hspi->Init.DataSize != SPI_DATASIZE_8BIT) {
        return LCD_ERROR;
    }
/* HAL_StatusTypeDef status2;
modif suppression spi avec callback, utilisation weak
status2 = HAL_SPI_RegisterCallback( hlcd->config.hspi, HAL_SPI_TX_COMPLETE_CB_ID, &LCD_SPI_TxCpltCallback);
*/
   /* Clear static buffers */
   memset(lcd_framebuffer, 0, LCD_TOTAL_BYTES);
   memset(lcd_dma_buffer, 0, LCD_MAX_DMA_SIZE);

   p_hlcd->initialized = true; // LCD_Clear() check initialized !

   /* Clear display */
   LCD_Status_t status = LCD_Clear(p_hlcd);
   if (status != LCD_OK) {
      return status;
   }
   return LCD_OK;
}

/**
 * @brief Get pointer to the framebuffer
 * @retval uint8_t*: Pointer to framebuffer
 */
uint8_t* LCD_GetFramebuffer(void)
{
    return lcd_framebuffer;
}


/**
 * @brief Get pointer to DMA buffer
 * @retval uint8_t*: Pointer to DMA buffer
 */
uint8_t* LCD_GetDMABuffer(void)
{
    return lcd_dma_buffer;
}

/**
 * @brief Get pointer to modified lines table
 * @retval bool*: Pointer to modified lines boolean array
 */
bool* LCD_GetModifiedLinesTable_not_used(void)
{
    return (g_hlcd) ? g_hlcd->modified.lines : NULL;
}




/**
 * @brief Deinitialize the LCD driver
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_DeInit(LCD_Handle_t *hlcd)
{
    if (!hlcd || !hlcd->initialized) {
        return LCD_ERROR;
    }

    /* Wait for any ongoing transfer */
    LCD_WaitForTransfer(hlcd);

    /* Clear display */
//    LCD_Clear(hlcd);

    /* Clear static buffers */
//    memset(lcd_framebuffer, 0, LCD_TOTAL_BYTES);
//    memset(lcd_dma_buffer, 0, LCD_MAX_DMA_SIZE);

    hlcd->initialized = false;
    g_hlcd = NULL;

    return LCD_OK;
}


/**
 * @brief Clear the entire display
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_Clear(LCD_Handle_t *p_hlcd)
{
    if (!p_hlcd || !p_hlcd->initialized) {
        return LCD_ERROR;
    }

     if (true == p_hlcd->spi_off) {
         SHARP_SPI1_Init(p_hlcd);
      p_hlcd->spi_off = false;
    }


    /* Wait for any ongoing transfer */
    LCD_Status_t status = LCD_WaitForTransfer(p_hlcd);
    if (status != LCD_OK) {
        return status;
    }

    /* Toggle VCOM */
    LCD_ToggleVCOM(p_hlcd);

    /* Send clear command */
    uint8_t cmd = LCD_CMD_CLEAR_ALL | (p_hlcd->vcom_state ? LCD_CMD_VCOM : 0);
    status = LCD_SendCommand(p_hlcd, cmd);
    if (status != LCD_OK) {
        return status;
    }

    /* Clear framebuffer and modified lines */
    memset(lcd_framebuffer, 0xff, LCD_TOTAL_BYTES);
    LCD_ClearModifiedLines(p_hlcd);

    return LCD_OK;
}

/**
 * @brief Clear the entire display
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_ClearFramebuffer(LCD_Handle_t *p_hlcd)
{
    if (!p_hlcd || !p_hlcd->initialized) {
        return LCD_ERROR;
    }
    /* Clear framebuffer and modified lines */
    memset(lcd_framebuffer, 0xff, LCD_TOTAL_BYTES);
    LCD_ClearModifiedLines(p_hlcd);

    return LCD_OK;
}


/**
 * @brief Mark a specific line as modified
 * @param hlcd: LCD handle
 * @param line: Line number (0 to LCD_HEIGHT-1)
 */
void LCD_MarkLineModified(LCD_Handle_t *p_hlcd, uint16_t line)
{
#if LCD_VER
    int lcd_height = LCD_WIDTH;
#elif
   int lcd_height = LCD_HEIGHT;
#endif

    if (!p_hlcd || !p_hlcd->initialized || line >= lcd_height) {
        return;
    }


    if (!p_hlcd->modified.lines[line]) {
        p_hlcd->modified.lines[line] = true;
        p_hlcd->modified.count++;
        p_hlcd->modified.has_changes = true;
        LCD_UpdateModifiedRange(p_hlcd, line);
    }
}

/**
 * @brief Mark all lines as modified
 * @param hlcd: LCD handle
 */
void LCD_MarkAllLinesModified(LCD_Handle_t *p_hlcd)
{
    if (!p_hlcd || !p_hlcd->initialized) {
        return;
    }

#if LCD_VER
    memset(p_hlcd->modified.lines, true, LCD_WIDTH);
    p_hlcd->modified.count = LCD_WIDTH;
    p_hlcd->modified.first_modified = 0;
    p_hlcd->modified.last_modified = LCD_WIDTH - 1;
    p_hlcd->modified.has_changes = true;
#elif
    memset(p_hlcd->modified.lines, true, LCD_HEIGHT);
    p_hlcd->modified.count = LCD_HEIGHT;
    p_hlcd->modified.first_modified = 0;
    p_hlcd->modified.last_modified = LCD_HEIGHT - 1;
    p_hlcd->modified.has_changes = true;
#endif

}

/**
 * @brief Clear all modified line flags
 * @param hlcd: LCD handle
 */
void LCD_ClearModifiedLines(LCD_Handle_t *p_hlcd)
{
    if (!p_hlcd || !p_hlcd->initialized) {
        return;
    }

#if LCD_VER
    memset(p_hlcd->modified.lines, false, LCD_WIDTH);
    p_hlcd->modified.count = 0;
    p_hlcd->modified.first_modified = LCD_WIDTH;
    p_hlcd->modified.last_modified = 0;
    p_hlcd->modified.has_changes = false;
#elif
    memset(p_hlcd->modified.lines, false, LCD_HEIGHT);
    p_hlcd->modified.count = 0;
    p_hlcd->modified.first_modified = LCD_HEIGHT;
    p_hlcd->modified.last_modified = 0;
    p_hlcd->modified.has_changes = false;

#endif

}

/**
 * @brief Toggle VCOM state (must be called periodically)
 * @param hlcd: LCD handle
 * it's only a boolean flag, no direct action on display, soft VCOM
 */
void LCD_ToggleVCOM(LCD_Handle_t *p_hlcd)
{
    if (p_hlcd) {
        p_hlcd->vcom_state = !p_hlcd->vcom_state;
    }
}


/**
 * @brief Wait for transfer completion
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_WaitForTransfer(LCD_Handle_t *p_hlcd)
{
    if (!p_hlcd) {
        return LCD_ERROR;
    }
// do not use : Cnt_ms
    uint32_t timeout = rtc_read_ticks();

    while (!p_hlcd->transfer_complete) {
        if (rtc_elapsed_ticks(timeout) > p_hlcd->config.timeout_ms) {
            return LCD_TIMEOUT;
        }
    }

    return LCD_OK;
}



/**
 * @brief Update the entire display with framebuffer content
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_UpdateDisplay(LCD_Handle_t *p_hlcd)
{
    if (!p_hlcd || !p_hlcd->initialized) {
        return LCD_ERROR;
    }

    /* Mark all lines as modified */
    LCD_MarkAllLinesModified(p_hlcd);

    /* Update all modified lines */
    return LCD_UpdateModifiedLines(p_hlcd);
}


/**
 * @brief Update only the modified lines in a single DMA transfer
 * @param hlcd: LCD handle
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_UpdateModifiedLines(LCD_Handle_t *p_hlcd)
{
   /* initialisation error */
   if (!p_hlcd || !p_hlcd->initialized)  return LCD_ERROR;

   /* return from sleeping with spi & dma clocks off */
   if (true == p_hlcd->spi_off)    SHARP_SPI1_Init(p_hlcd);

   /* speed optimisation, do nothing during a transfert */
   if (!p_hlcd->transfer_complete) 
   {
      p_hlcd->need_refresh += 1;
//      if (100 > p_hlcd->need_refresh)
//         return LCD_OK;
   }

    /* Check if there are any changes */
    #if (LCD_VER == 0)
 //   if ((!p_hlcd->need_refresh )&&(!p_hlcd->modified.has_changes || p_hlcd->modified.count == 0)) {
 //       return LCD_OK;
 //   }
   #endif


    /* Wait for any ongoing transfer */
    LCD_Status_t status = LCD_WaitForTransfer(p_hlcd);
    if (status != LCD_OK) {
        return status;
    }


    /* Toggle VCOM */
    LCD_ToggleVCOM(p_hlcd);

    /* Build DMA buffer with only modified lines 
      And clear modified lines
   */
    uint16_t dma_size;
#if SHARP_27_400x240
     dma_size = LCD_BuildDMABuffer_27(p_hlcd, lcd_dma_buffer);
#endif
#if SHARP_32_536x336
     dma_size = LCD_BuildDMABuffer_32(p_hlcd, lcd_dma_buffer);
#endif

    if (dma_size == 0) {
        return LCD_OK;
    }
// for h7, clean Dcache
SCB_CleanDCache_by_Addr((uint32_t*)lcd_dma_buffer, sizeof(lcd_dma_buffer));


    /* Send data */
    p_hlcd->transfer_complete = false;

    if (p_hlcd->config.use_dma) {
         /* Single DMA transfer with all modified lines */
         HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
         HAL_StatusTypeDef hal_status = HAL_SPI_Transmit_DMA(p_hlcd->config.hspi, 
                                                           lcd_dma_buffer, 
                                                           dma_size);
           if (hal_status != HAL_OK) {
               p_hlcd->transfer_complete = true;
               return LCD_DMA1;
           }
    } else {
        /* Polling transfer */
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
        HAL_StatusTypeDef hal_status = HAL_SPI_Transmit(p_hlcd->config.hspi, 
                                                       lcd_dma_buffer, 
                                                       dma_size, 
                                                       p_hlcd->config.timeout_ms);
        p_hlcd->transfer_complete = true;
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

        if (hal_status != HAL_OK) {
            return LCD_DMA2;
        }
    }
   p_hlcd->need_refresh = 0;
   return LCD_OK;
}


/**
 * @brief Set a pixel in the framebuffer
 * @param hlcd: LCD handle
 * @param x: X coordinate (0 to LCD_WIDTH-1)
 * @param y: Y coordinate (0 to LCD_HEIGHT-1)
 * @param pixel: Pixel state (true = 1, 0 = white, -1 : invert)
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_SetPixel(LCD_Handle_t *p_hlcd, uint16_t x, uint16_t y, int8_t pixel)
{
    if (!p_hlcd || !p_hlcd->initialized || x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return LCD_ERROR;
    }

#if  MIRROR 
   x = LCD_WIDTH -x -1;
#endif

    uint32_t byte_index = (y * LCD_BYTES_PER_LINE) + (x / 8);
    uint8_t bit_index = x % 8;
    uint8_t old_value = lcd_framebuffer[byte_index];

    if (pixel==0) {
        lcd_framebuffer[byte_index] |= (1 << bit_index);
    } 
   else    if (pixel==-1) {
        lcd_framebuffer[byte_index] ^= (1 << bit_index);
    } 
   else if (pixel== 1){
        lcd_framebuffer[byte_index] &= ~(1 << bit_index);
    }

    /* Mark line as modified if pixel actually changed */
    if (lcd_framebuffer[byte_index] != old_value) {
    #if LCD_VER
   //     LCD_MarkLineModified(p_hlcd, x);
    #elif
        LCD_MarkLineModified(p_hlcd, y);
   #endif
    }

    return LCD_OK;
}


/**
 * @brief Get a pixel from the framebuffer
 * @param hlcd: LCD handle
 * @param x: X coordinate (0 to LCD_WIDTH-1)
 * @param y: Y coordinate (0 to LCD_HEIGHT-1)
 * @retval bool: Pixel state (true = black, false = white)
 */
bool LCD_GetPixel(LCD_Handle_t *p_hlcd, uint16_t x, uint16_t y)
{
    if (!p_hlcd || !p_hlcd->initialized || x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return false;
    }

    uint32_t byte_index = (y * LCD_BYTES_PER_LINE) + (x / 8);
    uint8_t bit_index = x % 8;

    return (lcd_framebuffer[byte_index] & (1 << bit_index)) == 0;
}



/**
 * @brief Send a command to the LCD
 * @param hlcd: LCD handle
 * @param cmd: Command byte
 * @retval LCD_Status_t
 */
static LCD_Status_t LCD_SendCommand(LCD_Handle_t *p_hlcd, uint8_t cmd)
{
   lcd_dma_buffer[0] = cmd;
   lcd_dma_buffer[1] = 0x00; // Dummy byte

   p_hlcd->transfer_complete = false;

   HAL_StatusTypeDef hal_status;
   if (p_hlcd->config.use_dma) {

      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
      hal_status = HAL_SPI_Transmit_DMA(p_hlcd->config.hspi, lcd_dma_buffer, 2);
      if (hal_status != HAL_OK) {
         p_hlcd->transfer_complete = true;
         return LCD_ERROR;
      }
   } 
   else {
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);
      hal_status = HAL_SPI_Transmit(p_hlcd->config.hspi, lcd_dma_buffer, 2, p_hlcd->config.timeout_ms);
      HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
      p_hlcd->transfer_complete = true;
      if (hal_status != HAL_OK) {
         return LCD_ERROR;
      }
   }
   return LCD_OK;
}


/**
 * @brief Build DMA buffer with only modified lines
 * @param hlcd: LCD handle
 * @param buffer: DMA buffer to fill
 * clear modified flags
 * @retval uint16_t: Size of data in buffer (0 on error)
 */

static uint16_t LCD_BuildDMABuffer_27(LCD_Handle_t *p_hlcd, uint8_t *buffer)
{
   if (!p_hlcd || !buffer || !p_hlcd->modified.has_changes) {
     return 0;
   }

   uint16_t buffer_index = 0;
   uint8_t base_cmd = LCD_CMD_WRITE_LINE | (p_hlcd->vcom_state ? LCD_CMD_VCOM : 0);

   /* Add only modified lines to buffer */
   for (uint16_t line = 0; line < LCD_HEIGHT; line++) {
      if ((p_hlcd->modified.lines[line]) ||
      ((line >= p_hlcd->modified.first_modified)&&
      (line <= p_hlcd->modified.last_modified)&&0))
      {
         /* Command byte */
         buffer[buffer_index++] = base_cmd;
         /* Line number (1-based) */
         buffer[buffer_index++] = (uint8_t)(line + 1);
#if MIRROR 
         uint32_t line_offset = (line+1) * LCD_BYTES_PER_LINE;
         for (int32_t ii = 0; ii<   LCD_BYTES_PER_LINE; ii++){

            uint8_t data = lcd_framebuffer[--line_offset];
            data = (data & 0xF0) >> 4 | (data & 0x0F) << 4;
            data = (data & 0xCC) >> 2 | (data & 0x33) << 2;
            data = (data & 0xAA) >> 1 | (data & 0x55) << 1;
            buffer[buffer_index++] = data;
         }
#else
         /* Copy line data from framebuffer */
         uint32_t line_offset = line * LCD_BYTES_PER_LINE;
         memcpy(&buffer[buffer_index], &lcd_framebuffer[line_offset], LCD_BYTES_PER_LINE);
         buffer_index += LCD_BYTES_PER_LINE;
#endif
      }
   }
   /* Add final dummy byte */
   buffer[buffer_index++] = 0x00;
// here,not at the end of dma transfert, otherwise, miss some modifications
   LCD_ClearModifiedLines(p_hlcd);
   return buffer_index;
}

static uint16_t LCD_BuildDMABuffer_32(LCD_Handle_t *p_hlcd, uint8_t *buffer)
{
   char mat_8[8];
#if (LCD_VER ==0)
   if (!p_hlcd || !buffer || !p_hlcd->modified.has_changes) {
     return 0;
   }
#endif

   uint16_t buffer_index = 0;
   uint8_t base_cmd = LCD_CMD_WRITE_LINE | (p_hlcd->vcom_state ? LCD_CMD_VCOM : 0);

   /* Add only modified lines to buffer */
   // all for testing
   for (uint16_t col = 0; col <LCD_WIDTH ; col= col +8) {      // Vertical refresh 536
         /* Command byte *8 */
         uint16_t col_rev =0;
         buffer_index = col*LCD_COL_TOTAL_SIZE ;
         for ( uint16_t b_col = 0; b_col < 8; b_col++)
         {
            col_rev = LCD_WIDTH - col - b_col;
            buffer[buffer_index + b_col * LCD_COL_TOTAL_SIZE] = base_cmd | (( (col_rev  )&0x3)<<6 );
            buffer[buffer_index + 1 + b_col * LCD_COL_TOTAL_SIZE] = (uint8_t)((col_rev )>>2);
         }

         for ( uint16_t row = 0; row <LCD_HEIGHT ; row= row + 8)
         {
            for ( uint16_t b_col = 0; b_col < 8; b_col++)
            {
               mat_8[b_col] = lcd_framebuffer[(LCD_HEIGHT - row -b_col -1) * LCD_BYTES_PER_LINE  + col/8];
            }

            for ( uint16_t b_col = 0; b_col < 8; b_col++)
            {
               buffer[buffer_index + 2 + b_col * LCD_COL_TOTAL_SIZE] = 
                     ((mat_8[0]>> b_col) & 0x1) |
                     ((mat_8[1]>> b_col) & 0x1)<<1 |
                     ((mat_8[2]>> b_col) & 0x1)<<2 |
                     ((mat_8[3]>> b_col) & 0x1)<<3 |
                     ((mat_8[4]>> b_col) & 0x1)<<4 |
                     ((mat_8[5]>> b_col) & 0x1)<<5 |
                     ((mat_8[6]>> b_col) & 0x1)<<6 |
                     ((mat_8[7]>> b_col) & 0x1)<<7;

            }
            buffer_index += 1;
         
         } // inc y

   } // inc x +8
   
   /* Add final dummy byte */
   buffer_index = LCD_COL_TOTAL_SIZE * LCD_WIDTH;
   buffer[buffer_index++] = 0x00;
   buffer[buffer_index++] = 0x00;

// BUG here,not at the end of dma transfert, otherwise, miss some modifications
   LCD_ClearModifiedLines(p_hlcd);
   return LCD_COL_TOTAL_SIZE * LCD_WIDTH + 2;
}


/**
 * @brief Update modified line range tracking
 * @param hlcd: LCD handle
 * @param line: Line number that was modified
 */
static void LCD_UpdateModifiedRange(LCD_Handle_t *p_hlcd, uint16_t line)
{
   if (line < p_hlcd->modified.first_modified) {
      p_hlcd->modified.first_modified = line;
   }
   if (line > p_hlcd->modified.last_modified) {
      p_hlcd->modified.last_modified = line;
   }
}


/**
  * @brief This function handles GPDMA1 Channel 0 global interrupt.
  */
void DMA1_Stream0_IRQHandler(void)
{
   HAL_DMA_IRQHandler(&hdma_spi1_tx);
}


/**
  * @brief This function handles SPI1 global interrupt.
  */
void SPI1_IRQHandler(void)
{
  HAL_SPI_IRQHandler(&hspi1);
}


/**
 * @brief SPI transfer complete callback
 * @param spi: SPI handle
 weak fonction, no registration 
 Clear modified lines at the end of dma_buff contruction, NOT after DMA transfer  
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *spi)
{
   HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);
   if ((g_hlcd)&& (spi->Instance == SPI1))  {

#ifdef  DEBUG_LCD
      HAL_SPI_StateTypeDef status = HAL_SPI_GetState(spi);
      SEGGER_RTT_printf(0, "\nSPI4 Ended Call back Status ( 1 ready) %02x ", (int)status );
#endif
        g_hlcd->transfer_complete = true;
    }
}


// Cascadia Code inspired font 12x24 bitmap for ASCII characters (32-126)
// Each character is 12 pixels wide, 24 pixels tall
// Each row is stored as 2 bytes (16 bits, using lower 12 bits)
static const uint16_t DB_ROM2 cascadia_font_12x24[][24] = 
{
    // Space (32)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ! (33)
    {0x000, 0x000, 0x060, 0x060, 0x060, 0x060, 0x060, 0x060, 0x060, 0x060, 0x060, 0x060,
     0x060, 0x060, 0x000, 0x000, 0x060, 0x060, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // " (34)
    {0x000, 0x000, 0x198, 0x198, 0x198, 0x198, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // # (35)
    {0x000, 0x000, 0x000, 0x198, 0x198, 0x198, 0x7FE, 0x198, 0x198, 0x330, 0x330, 0x7FE,
     0x330, 0x330, 0x330, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // $ (36)
    {0x000, 0x000, 0x030, 0x0FC, 0x1FE, 0x330, 0x330, 0x330, 0x1F0, 0x0FC, 0x03E, 0x033,
     0x033, 0x330, 0x1FE, 0x0FC, 0x030, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // % (37)
    {0x000, 0x000, 0x000, 0x1C0, 0x360, 0x360, 0x1CC, 0x018, 0x030, 0x060, 0x0C0, 0x198,
     0x336, 0x336, 0x01C, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // & (38)
    {0x000, 0x000, 0x000, 0x078, 0x0CC, 0x0CC, 0x0CC, 0x078, 0x1BC, 0x3CE, 0x3C6, 0x3C6,
     0x3C6, 0x1CE, 0x0FF, 0x079, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ' (39)
    {0x000, 0x000, 0x060, 0x060, 0x060, 0x060, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ( (40)
    {0x000, 0x000, 0x018, 0x030, 0x060, 0x060, 0x0C0, 0x0C0, 0x0C0, 0x0C0, 0x0C0, 0x0C0,
     0x0C0, 0x0C0, 0x060, 0x060, 0x030, 0x018, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ) (41)
    {0x000, 0x000, 0x180, 0x0C0, 0x060, 0x060, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030,
     0x030, 0x030, 0x060, 0x060, 0x0C0, 0x180, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // * (42)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x066, 0x03C, 0x0FF, 0x03C, 0x066, 0x000, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // + (43)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x030, 0x030, 0x030, 0x3FC, 0x030, 0x030, 0x030,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // , (44)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x060, 0x060, 0x060, 0x0C0, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // - (45)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x1FE, 0x000, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // . (46)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x060, 0x060, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // / (47)
    {0x000, 0x000, 0x006, 0x006, 0x00C, 0x00C, 0x018, 0x018, 0x030, 0x030, 0x060, 0x060,
     0x0C0, 0x0C0, 0x180, 0x180, 0x300, 0x300, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 0 (48)
    {0x000, 0x000, 0x000, 0x07C, 0x0FE, 0x1C7, 0x383, 0x383, 0x383, 0x383, 0x383, 0x383,
     0x383, 0x1C7, 0x0FE, 0x07C, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 1 (49)
    {0x000, 0x000, 0x000, 0x030, 0x070, 0x0F0, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030,
     0x030, 0x030, 0x3FC, 0x3FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 2 (50)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x003, 0x003, 0x00E, 0x01C, 0x038, 0x070,
     0x0E0, 0x1C0, 0x3FF, 0x3FF, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 3 (51)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x003, 0x003, 0x07C, 0x07C, 0x003, 0x003,
     0x003, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 4 (52)
    {0x000, 0x000, 0x000, 0x00E, 0x01E, 0x036, 0x066, 0x0C6, 0x186, 0x306, 0x3FF, 0x3FF,
     0x006, 0x006, 0x006, 0x006, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 5 (53)
    {0x000, 0x000, 0x000, 0x1FF, 0x1FF, 0x180, 0x180, 0x180, 0x1FC, 0x1FE, 0x003, 0x003,
     0x003, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 6 (54)
    {0x000, 0x000, 0x000, 0x07C, 0x0FE, 0x1C0, 0x380, 0x380, 0x3FC, 0x3FE, 0x383, 0x383,
     0x383, 0x1C7, 0x0FE, 0x07C, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 7 (55)
    {0x000, 0x000, 0x000, 0x3FF, 0x3FF, 0x003, 0x006, 0x00C, 0x018, 0x030, 0x060, 0x060,
     0x0C0, 0x0C0, 0x0C0, 0x0C0, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 8 (56)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x303, 0x307, 0x1FE, 0x1FE, 0x307, 0x303,
     0x303, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // 9 (57)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x303, 0x303, 0x1FF, 0x0FF, 0x007, 0x007,
     0x00E, 0x01C, 0x1F8, 0x0F0, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // : (58)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x060, 0x060, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x060, 0x060, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ; (59)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x060, 0x060, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x060, 0x060, 0x060, 0x0C0, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // < (60)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x00C, 0x018, 0x030, 0x060, 0x0C0, 0x060, 0x030,
     0x018, 0x00C, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // = (61)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x1FE, 0x000, 0x000, 0x1FE, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // > (62)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x180, 0x0C0, 0x060, 0x030, 0x018, 0x030, 0x060,
     0x0C0, 0x180, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ? (63)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x003, 0x003, 0x00E, 0x01C, 0x030, 0x030,
     0x000, 0x000, 0x030, 0x030, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // @ (64)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x303, 0x37B, 0x37B, 0x37B, 0x37B, 0x1FB,
     0x180, 0x1C0, 0x0FE, 0x07C, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // A (65)
    {0x000, 0x000, 0x000, 0x078, 0x0FC, 0x18E, 0x307, 0x303, 0x303, 0x3FF, 0x3FF, 0x303,
     0x303, 0x303, 0x303, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // B (66)
    {0x000, 0x000, 0x000, 0x3FC, 0x3FE, 0x307, 0x303, 0x307, 0x3FE, 0x3FE, 0x307, 0x303,
     0x303, 0x307, 0x3FE, 0x3FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // C (67)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300,
     0x300, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // D (68)
    {0x000, 0x000, 0x000, 0x3F8, 0x3FC, 0x30E, 0x307, 0x303, 0x303, 0x303, 0x303, 0x303,
     0x307, 0x30E, 0x3FC, 0x3F8, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // E (69)
    {0x000, 0x000, 0x000, 0x3FF, 0x3FF, 0x300, 0x300, 0x300, 0x3FC, 0x3FC, 0x300, 0x300,
     0x300, 0x300, 0x3FF, 0x3FF, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // F (70)
    {0x000, 0x000, 0x000, 0x3FF, 0x3FF, 0x300, 0x300, 0x300, 0x3FC, 0x3FC, 0x300, 0x300,
     0x300, 0x300, 0x300, 0x300, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // G (71)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x300, 0x300, 0x300, 0x31F, 0x31F, 0x303,
     0x303, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // H (72)
    {0x000, 0x000, 0x000, 0x303, 0x303, 0x303, 0x303, 0x303, 0x3FF, 0x3FF, 0x303, 0x303,
     0x303, 0x303, 0x303, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // I (73)
    {0x000, 0x000, 0x000, 0x1FE, 0x1FE, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030,
     0x030, 0x030, 0x1FE, 0x1FE, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // J (74)
    {0x000, 0x000, 0x000, 0x01F, 0x01F, 0x003, 0x003, 0x003, 0x003, 0x003, 0x003, 0x303,
     0x303, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // K (75)
    {0x000, 0x000, 0x000, 0x303, 0x307, 0x30E, 0x31C, 0x338, 0x370, 0x3E0, 0x3E0, 0x370,
     0x338, 0x31C, 0x30E, 0x307, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // L (76)
    {0x000, 0x000, 0x000, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300, 0x300,
     0x300, 0x300, 0x3FF, 0x3FF, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // M (77)
    {0x000, 0x000, 0x000, 0x383, 0x3C7, 0x3EF, 0x3FF, 0x3BB, 0x393, 0x303, 0x303, 0x303,
     0x303, 0x303, 0x303, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // N (78)
    {0x000, 0x000, 0x000, 0x303, 0x383, 0x3C3, 0x3E3, 0x3F3, 0x3BB, 0x31F, 0x30F, 0x307,
     0x303, 0x303, 0x303, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // O (79)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x303, 0x303, 0x303, 0x303, 0x303, 0x303,
     0x303, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // P (80)
    {0x000, 0x000, 0x000, 0x3FC, 0x3FE, 0x307, 0x303, 0x303, 0x307, 0x3FE, 0x3FC, 0x300,
     0x300, 0x300, 0x300, 0x300, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // Q (81)  
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x303, 0x303, 0x303, 0x303, 0x303, 0x31B,
     0x30F, 0x307, 0x1FE, 0x0FC, 0x006, 0x003, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // R (82)
    {0x000, 0x000, 0x000, 0x3FC, 0x3FE, 0x307, 0x303, 0x303, 0x307, 0x3FE, 0x3FC, 0x338,
     0x31C, 0x30E, 0x307, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // S (83)
    {0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x300, 0x300, 0x1F0, 0x0FC, 0x00E, 0x003,
     0x003, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // T (84)
    {0x000, 0x000, 0x000, 0x3FF, 0x3FF, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030,
     0x030, 0x030, 0x030, 0x030, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // U (85)
    {0x000, 0x000, 0x000, 0x303, 0x303, 0x303, 0x303, 0x303, 0x303, 0x303, 0x303, 0x303,
     0x303, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // V (86)
    {0x000, 0x000, 0x000, 0x303, 0x303, 0x303, 0x303, 0x307, 0x18E, 0x18C, 0x0D8, 0x0D8,
     0x070, 0x070, 0x030, 0x030, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // W (87)
    {0x000, 0x000, 0x000, 0x303, 0x303, 0x303, 0x303, 0x303, 0x303, 0x393, 0x3BB, 0x3FF,
     0x3EF, 0x3C7, 0x383, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // X (88)
    {0x000, 0x000, 0x000, 0x307, 0x18E, 0x0DC, 0x078, 0x030, 0x030, 0x078, 0x0DC, 0x18E,
     0x307, 0x303, 0x303, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // Y (89)
    {0x000, 0x000, 0x000, 0x307, 0x307, 0x18E, 0x0DC, 0x078, 0x030, 0x030, 0x030, 0x030,
     0x030, 0x030, 0x030, 0x030, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // Z (90)
    {0x000, 0x000, 0x000, 0x3FF, 0x3FF, 0x00E, 0x018, 0x030, 0x060, 0x0C0, 0x180, 0x300,
     0x300, 0x380, 0x3FF, 0x3FF, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // [ (91)
    {0x000, 0x000, 0x0F8, 0x0F8, 0x0C0, 0x0C0, 0x0C0, 0x0C0, 0x0C0, 0x0C0, 0x0C0, 0x0C0,
     0x0C0, 0x0C0, 0x0C0, 0x0C0, 0x0F8, 0x0F8, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // \ (92)
    {0x000, 0x000, 0x300, 0x300, 0x180, 0x180, 0x0C0, 0x0C0, 0x060, 0x060, 0x030, 0x030,
     0x018, 0x018, 0x00C, 0x00C, 0x006, 0x006, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ] (93)
    {0x000, 0x000, 0x0F8, 0x0F8, 0x018, 0x018, 0x018, 0x018, 0x018, 0x018, 0x018, 0x018,
     0x018, 0x018, 0x018, 0x018, 0x0F8, 0x0F8, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ^ (94)
    {0x000, 0x000, 0x020, 0x070, 0x0D8, 0x18C, 0x306, 0x000, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // _ (95)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x3FF, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ` (96)
    {0x000, 0x000, 0x0C0, 0x060, 0x030, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // a (97)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x007, 0x0FF, 0x1FF, 0x303,
     0x303, 0x30F, 0x1FF, 0x0F7, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // b (98)
    {0x000, 0x000, 0x300, 0x300, 0x300, 0x300, 0x3FC, 0x3FE, 0x307, 0x303, 0x303, 0x303,
     0x303, 0x307, 0x3FE, 0x3FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // c (99)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x300, 0x300, 0x300,
     0x300, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // d (100)
    {0x000, 0x000, 0x003, 0x003, 0x003, 0x003, 0x0FF, 0x1FF, 0x307, 0x303, 0x303, 0x303,
     0x303, 0x307, 0x1FF, 0x0FF, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // e (101)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x303, 0x3FF, 0x300,
     0x300, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // f (102)
    {0x000, 0x000, 0x03E, 0x07F, 0x070, 0x070, 0x3FC, 0x3FC, 0x070, 0x070, 0x070, 0x070,
     0x070, 0x070, 0x070, 0x070, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // g (103)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x0FF, 0x1FF, 0x307, 0x303, 0x303, 0x303,
     0x303, 0x307, 0x1FF, 0x0FF, 0x003, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000},
    
    // h (104)
    {0x000, 0x000, 0x300, 0x300, 0x300, 0x300, 0x3FC, 0x3FE, 0x307, 0x303, 0x303, 0x303,
     0x303, 0x303, 0x303, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // i (105)
    {0x000, 0x000, 0x030, 0x030, 0x000, 0x000, 0x0F0, 0x0F0, 0x030, 0x030, 0x030, 0x030,
     0x030, 0x030, 0x1FE, 0x1FE, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // j (106)
    {0x000, 0x000, 0x018, 0x018, 0x000, 0x000, 0x078, 0x078, 0x018, 0x018, 0x018, 0x018,
     0x018, 0x018, 0x018, 0x018, 0x018, 0x318, 0x1F0, 0x0E0, 0x000, 0x000, 0x000, 0x000},
    
    // k (107)
    {0x000, 0x000, 0x300, 0x300, 0x300, 0x300, 0x307, 0x30E, 0x31C, 0x338, 0x3F0, 0x3F0,
     0x338, 0x31C, 0x30E, 0x307, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // l (108)
    {0x000, 0x000, 0x0F0, 0x0F0, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030,
     0x030, 0x030, 0x1FE, 0x1FE, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // m (109)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x3EC, 0x3FE, 0x37B, 0x333, 0x333, 0x333,
     0x333, 0x333, 0x333, 0x333, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // n (110)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x3FC, 0x3FE, 0x307, 0x303, 0x303, 0x303,
     0x303, 0x303, 0x303, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // o (111)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x0FC, 0x1FE, 0x307, 0x303, 0x303, 0x303,
     0x303, 0x307, 0x1FE, 0x0FC, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // p (112)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x3FC, 0x3FE, 0x307, 0x303, 0x303, 0x303,
     0x303, 0x307, 0x3FE, 0x3FC, 0x300, 0x300, 0x300, 0x300, 0x000, 0x000, 0x000, 0x000},
    
    // q (113)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x0FF, 0x1FF, 0x307, 0x303, 0x303, 0x303,
     0x303, 0x307, 0x1FF, 0x0FF, 0x003, 0x003, 0x003, 0x003, 0x000, 0x000, 0x000, 0x000},
    
    // r (114)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x1EC, 0x1FE, 0x1C0, 0x180, 0x180, 0x180,
     0x180, 0x180, 0x180, 0x180, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // s (115)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x0FE, 0x1FE, 0x300, 0x1E0, 0x07C, 0x00E,
     0x003, 0x1FF, 0x1FE, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // t (116)
    {0x000, 0x000, 0x000, 0x060, 0x060, 0x060, 0x3FC, 0x3FC, 0x060, 0x060, 0x060, 0x060,
     0x060, 0x067, 0x03F, 0x01E, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // u (117)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x303, 0x303, 0x303, 0x303, 0x303, 0x303,
     0x303, 0x307, 0x1FF, 0x0F7, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // v (118)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x303, 0x303, 0x307, 0x18E, 0x0DC, 0x0D8,
     0x070, 0x070, 0x030, 0x030, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // w (119)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x303, 0x303, 0x333, 0x333, 0x333, 0x37B,
     0x3FF, 0x3EF, 0x3C7, 0x183, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // x (120)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x307, 0x18E, 0x0DC, 0x078, 0x030, 0x078,
     0x0DC, 0x18E, 0x307, 0x303, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // y (121)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x303, 0x303, 0x307, 0x18E, 0x0DC, 0x0D8,
     0x070, 0x070, 0x030, 0x030, 0x060, 0x1E0, 0x1C0, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // z (122)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x1FF, 0x1FF, 0x00E, 0x01C, 0x038, 0x070,
     0x0E0, 0x1C0, 0x1FF, 0x1FF, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // { (123)
    {0x000, 0x000, 0x03C, 0x07C, 0x060, 0x060, 0x060, 0x060, 0x060, 0x1C0, 0x060, 0x060,
     0x060, 0x060, 0x060, 0x060, 0x07C, 0x03C, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // | (124)
    {0x000, 0x000, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030, 0x030,
     0x030, 0x030, 0x030, 0x030, 0x030, 0x030, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // } (125)
    {0x000, 0x000, 0x1E0, 0x1F0, 0x030, 0x030, 0x030, 0x030, 0x030, 0x01C, 0x030, 0x030,
     0x030, 0x030, 0x030, 0x030, 0x1F0, 0x1E0, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000},
    
    // ~ (126)
    {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x071, 0x1FB, 0x38E, 0x000, 0x000,
     0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000}
}; 



/**
 * Draw a single character using 12x24 font 
 */
void draw_char_12x24(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, char c, int8_t pixel) 
{
    int char_index = c-0x20;
   if ((c<' ')|(c>'~')) return;
        
    for (int row = 0; row < 24; row++) {
        uint16_t row_data = cascadia_font_12x24[char_index][row];
        
        // Draw 8 pixels from the byte (can be extended to 12 if needed)
        for (int col = 0; col < 12; col++) {
            if (row_data & (0x800 >> col)) {
                LCD_SetPixel(hlcd, x + col, y + row, pixel);
            }
        }
    }
}


/**
 * Display text string using 12x24 font
 */
void display_text_12x24(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, const char* text, int8_t pixel) {
    uint16_t cursor_x = x;
    uint16_t cursor_y = y;
    
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\n') {
            // New line
            cursor_x = x;
            cursor_y += 26; // 24 pixels height + 2 pixels spacing
        } else {
            // Draw character
            draw_char_12x24(hlcd, cursor_x, cursor_y, text[i], pixel);
            cursor_x += 12; // 12 pixels width + 0 pixels spacing (or 14 for full 12-bit width)
        }
    }
}

/**
 * @brief Draw a segment using Bresenham's algorithm
 * @param hlcd: LCD handle
 * @param x0, y0 : coordinates of first point
 * @param x1, y1 : coordinates of second point
 * @param pixel: Pixel state (1 = black, 0 = white, -1 = toggle)
 * @retval LCD_Status_t
 */
void LCD_DrawLine(LCD_Handle_t *hlcd,uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int8_t pixel) 
{
    int16_t dx = abs(x1 - x0);
    int16_t dy = abs(y1 - y0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx - dy;
    
    while (1) {
        LCD_SetPixel(hlcd, x0, y0, pixel);
        
        if (x0 == x1 && y0 == y1) break;
        
        int16_t e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/**
 * @brief Fill a rectangle in the framebuffer
 * @param hlcd: LCD handle
 * @param x: X coordinate of top-left corner
 * @param y: Y coordinate of top-left corner
 * @param w: Width in pixels
 * @param h: Height in pixels
 * @param pixel: Pixel state (1 = black, 0 = white, -1 = toggle)
 * @retval LCD_Status_t
 */
LCD_Status_t LCD_FillRect(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, int8_t pixel)
{
    if (!hlcd || !hlcd->initialized) {
        return LCD_ERROR;
    }

   

    /* Clip rectangle to display bounds */
    if (x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return LCD_OK;
    }
    if (x + w > LCD_WIDTH) {
        w = LCD_WIDTH - x;
    }
    if (y + h > LCD_HEIGHT) {
        h = LCD_HEIGHT - y;
    }

    for (uint16_t dy = 0; dy < h; dy++) {
       LCD_DrawLine(hlcd, x, y+dy, x + w - 1, y+dy, pixel);              // Top

   }
   return LCD_OK;
}


/**
 * @brief Draw a rectangle in the framebuffer
 * @param hlcd: LCD handle
 * @param x: X coordinate of top-left corner
 * @param y: Y coordinate of top-left corner
 * @param w: Width in pixels
 * @param h: Height in pixels
 * @param pixel: Pixel state (1 = black, 0 = white, -1 = toggle)
 * @retval LCD_Status_t
 */
// Draw rectangle outline
void LCD_DrawRect(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, int8_t pixel) 
{
    LCD_DrawLine(hlcd, x, y, x + w - 1, y, pixel);                      // Top
    LCD_DrawLine(hlcd, x, y + h - 1, x + w - 1, y + h - 1, pixel);      // Bottom
    LCD_DrawLine(hlcd, x, y, x, y + h - 1, pixel);                      // Left
    LCD_DrawLine(hlcd, x + w - 1, y, x + w - 1, y + h - 1, pixel);      // Right
}



LCD_Status_t SHARP_SPI1_Init(LCD_Handle_t *p_hlcd) {
     p_hlcd->status = LCD_OK;

#if USE_LCD_OFF
   #define LCD_OFF_Pin GPIO_PIN_13
   #define LCD_OFF_GPIO_Port GPIOE
   __HAL_RCC_GPIOE_CLK_ENABLE();
   HAL_GPIO_WritePin(LCD_OFF_GPIO_Port, LCD_OFF_Pin, GPIO_PIN_SET);
#endif

   __HAL_RCC_GPIOD_CLK_ENABLE();
   HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

   GPIO_InitTypeDef GPIO_InitStruct = {0};

   /*Configure GPIO pins : LED_Pin LCD_CS_Pin LCD_OFF_Pin */
#if USE_LCD_OFF
   GPIO_InitStruct.Pin = LCD_OFF_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(LCD_OFF_GPIO_Port, &GPIO_InitStruct);
#endif

   GPIO_InitStruct.Pin = LCD_CS_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
   HAL_GPIO_Init(LCD_CS_GPIO_Port, &GPIO_InitStruct);

/* config clock 16Mhz  */
   PeriphCommonClock_Config_P16_R64();

   /* DMA controller clock enable */
   __HAL_RCC_DMA1_CLK_ENABLE();

   /* Spi1 clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**  SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA7     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* SPI1 DMA Init */
    /* SPI1_TX Init */
    hdma_spi1_tx.Instance = DMA1_Stream0;
    hdma_spi1_tx.Init.Request = DMA_REQUEST_SPI1_TX;
    hdma_spi1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_spi1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_spi1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_spi1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_spi1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_spi1_tx.Init.Mode = DMA_NORMAL;
    hdma_spi1_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_spi1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_spi1_tx) != HAL_OK)
    {
         p_hlcd->status = LCD_ERROR;

      return LCD_ERROR;
    }

    __HAL_LINKDMA(&hspi1,hdmatx,hdma_spi1_tx);

    /* SPI1 interrupt Init */
    HAL_NVIC_SetPriority(SPI1_IRQn, 9, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);

  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;

  // clk = HSE = 25Mhz, still ok at 25Mhz/2 = 12.5, (full refresh = 8.2msec)

// 16Mhz, lcd ok at 8Mhz
  // 400x240 at 3.125Mhz, full refresh : 33msec
  // 536x336 at 8Mhz, full refresh : 35msec, (25msec ???)
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
// warning Sharp : lsb first
  hspi1.Init.FirstBit = SPI_FIRSTBIT_LSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_HIGH;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;

// tsSPS datasheet = 3µs, attentre sck apres CS
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_03CYCLE;

// dans un transfert multi-octets, cs redevient inactif entre les octets
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
     p_hlcd->status = LCD_ERROR;
      return LCD_ERROR;
  }

/* DMA Init */

   /* DMA interrupt init */
   /* DMA1_Stream0_IRQn interrupt configuration */
   HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 9, 2);
   HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

   p_hlcd->spi_off = false;
   p_hlcd->transfer_complete = true;
   return LCD_OK;
}



LCD_Status_t SHARP_SPI1_DeInit(LCD_Handle_t *p_hlcd) 
{
   if   ( true == p_hlcd->spi_off)
      return LCD_OK;

   LCD_WaitForTransfer(p_hlcd);
//   OS_TASK_Delay(20);
   
   GPIO_InitTypeDef GPIO_InitStruct = {0};

   GPIO_InitStruct.Pin = LCD_CS_Pin;
   GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
   GPIO_InitStruct.Pull = GPIO_PULLDOWN;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
   HAL_GPIO_Init(LCD_CS_GPIO_Port, &GPIO_InitStruct);

/* config clock 16Mhz  */
//   PeriphCommonClock_Config_P16_R64();

   /* DMA controller clock enable */
   __HAL_RCC_DMA1_CLK_DISABLE();

   /* Spi1 clock enable */
    __HAL_RCC_SPI1_CLK_DISABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**  SPI1 GPIO Configuration
    PA5     ------> SPI1_SCK
    PA7     ------> SPI1_MOSI
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
//    __HAL_RCC_GPIOA_CLK_DISABLE();


   p_hlcd->spi_off = true;
   return LCD_OK;

}
