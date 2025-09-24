#ifndef H7A3_SHARP_LS_H
#define H7A3_SHARP_LS_H

#include "stm32h7xx.h"
#include "stdbool.h"
#include "stdint.h"
#include "RTOS.h"
#include <time.h>
#include <stdio.h>




/* LCD Specifications */


// BUG  
// LS027 : horizontal refresh 240 lines, 400
// LS032 : vertical refresh : 536 lines, 336
#define LCD_CMD_HEADER_SIZE     2  // Command + Line number
#if SHARP_32_536x336
  
   #define LCD_COL_TOTAL_SIZE (LCD_CMD_HEADER_SIZE + LCD_BYTES_PER_COL)
   #define LCD_MAX_DMA_SIZE    (LCD_WIDTH * LCD_COL_TOTAL_SIZE + 2) // All lines + final dummy
   #define LCD_TOTAL_BYTES          (LCD_WIDTH * LCD_BYTES_PER_COL)
#endif

#if SHARP_27_400x240
//   #define LCD_BYTES_PER_LINE       (LCD_WIDTH/8)  // 400 pixels / 8 bits per byte
   #define LCD_LINE_TOTAL_SIZE (LCD_CMD_HEADER_SIZE + LCD_BYTES_PER_LINE)
   #define LCD_MAX_DMA_SIZE    (LCD_HEIGHT * LCD_LINE_TOTAL_SIZE + 1) // All lines + final dummy
   #define LCD_TOTAL_BYTES          (LCD_HEIGHT * LCD_BYTES_PER_LINE)
#endif

#define LCD_BYTES_PER_LINE       (LCD_WIDTH/8)  // 400/8 or 536/8
#define LCD_BYTES_PER_COL       (LCD_HEIGHT/8)  // 240  336 pixels / 8 bits per byte

/* Command bits */
#define LCD_CMD_WRITE_LINE  0x01
#define LCD_CMD_VCOM        0x02
#define LCD_CMD_CLEAR_ALL   0x04

#define LCD_CS_Pin         GPIO_PIN_14
#define LCD_CS_GPIO_Port   GPIOD


/* Driver Status */
#define LCD_MESS_STATUS_LENGTH 32
typedef enum {
    LCD_OK = 0,
    LCD_ERROR,
    LCD_BUSY,
    LCD_TIMEOUT,
   LCD_DMA1,
   LCD_DMA2,
   LCD_LAST
} LCD_Status_t;




/* Driver Configuration */


typedef struct {
    SPI_HandleTypeDef *hspi;
    bool use_dma;
    uint32_t timeout_ms;
} LCD_Config_t;


/* Modified line tracking */
typedef struct {
#if LCD_VER
    bool lines[ LCD_WIDTH ];     // Track which vertical lines are modified
#else
    bool lines[LCD_HEIGHT];     // Track which horizontal  lines are modified
#endif
    uint16_t first_modified;    // First modified line index
    uint16_t last_modified;     // Last modified line index
    uint16_t count;             // Number of modified lines
    bool has_changes;           // Any changes pending
} LCD_ModifiedLines_t;

/* Driver Handle */
typedef struct {
    volatile bool transfer_complete;
    uint16_t need_refresh;
    bool initialized;
    bool vcom_state;
    bool spi_off;
    LCD_Config_t config;
    LCD_ModifiedLines_t modified;
    LCD_Status_t status;
} LCD_Handle_t;

extern SPI_HandleTypeDef hspi1;

extern LCD_Handle_t hlcd;
//extern uint8_t lcd_framebuffer[LCD_TOTAL_BYTES];
extern const char LCD_Status_Desc[LCD_LAST][LCD_MESS_STATUS_LENGTH];




/* Function Prototypes */
LCD_Status_t LCD_Sharp_Init(LCD_Handle_t *hlcd, bool use_dma);
LCD_Status_t SHARP_SPI1_DeInit(LCD_Handle_t *hlcd);

LCD_Status_t LCD_DeInit(LCD_Handle_t *hlcd);
LCD_Status_t LCD_Clear(LCD_Handle_t *hlcd);
LCD_Status_t LCD_ClearFramebuffer(LCD_Handle_t *hlcd);
LCD_Status_t LCD_UpdateDisplay(LCD_Handle_t *hlcd);
LCD_Status_t LCD_UpdateModifiedLines(LCD_Handle_t *hlcd);
LCD_Status_t LCD_SetPixel(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, int8_t pixel);
bool LCD_GetPixel(LCD_Handle_t *hlcd, uint16_t x, uint16_t y);
LCD_Status_t LCD_FillRect(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, int8_t pixel);
void LCD_DrawLine(LCD_Handle_t *hlcd,uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, int8_t pixel);
void LCD_DrawRect(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, uint16_t w, uint16_t h, int8_t pixel);
void draw_char_12x24(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, char c, int8_t pixel);
void display_text_12x24(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, const char* text, int8_t pixel);
void display_text_16x32(LCD_Handle_t *hlcd, uint16_t x, uint16_t y, const char* text, int8_t pixel);


void LCD_ToggleVCOM(LCD_Handle_t *hlcd);
LCD_Status_t LCD_WaitForTransfer(LCD_Handle_t *hlcd);
void LCD_MarkLineModified(LCD_Handle_t *hlcd, uint16_t line);
void LCD_MarkAllLinesModified(LCD_Handle_t *hlcd);
void LCD_ClearModifiedLines(LCD_Handle_t *hlcd);
uint16_t LCD_GetModifiedLinesCount(LCD_Handle_t *hlcd);



/* Memory allocation functions */
uint8_t* LCD_GetFramebuffer(void);


#endif // H7A3_SHARP_LS_H