



#include "gc9a01.h"

extern SPI_HandleTypeDef GC9A01_SPI;

#if USE_DMA
static volatile uint8_t tx_busy = 0;
#endif

// === INTERNAL GPIO CONTROL ===
static void GC9A01_Select(void)   { HAL_GPIO_WritePin(GC9A01_CS_PORT, GC9A01_CS_PIN, GPIO_PIN_RESET); }
static void GC9A01_Unselect(void){ HAL_GPIO_WritePin(GC9A01_CS_PORT, GC9A01_CS_PIN, GPIO_PIN_SET);   }
static void GC9A01_DC_Command(void){ HAL_GPIO_WritePin(GC9A01_DC_PORT, GC9A01_DC_PIN, GPIO_PIN_RESET); }
static void GC9A01_DC_Data(void)   { HAL_GPIO_WritePin(GC9A01_DC_PORT, GC9A01_DC_PIN, GPIO_PIN_SET);   }

// === RESET ===
void GC9A01_Reset(void)
{
	// Software Reset
	GC9A01_WriteCommand(0x01);

	HAL_Delay(150);

	GC9A01_WriteCommand(0x11);

	HAL_Delay(120);
}

// === COMMAND/DATA ===
void GC9A01_WriteCommand(uint8_t cmd)
{
	/* Wait for any previous LVGL DMA transfers to finish before sending commands */
#if USE_DMA
	while(tx_busy);
#endif

	GC9A01_Select();
	GC9A01_DC_Command();
	HAL_SPI_Transmit(&GC9A01_SPI, &cmd, 1, HAL_MAX_DELAY);
	GC9A01_Unselect();
}

void GC9A01_WriteData(uint8_t data)
{
	GC9A01_Select();
	GC9A01_DC_Data();
	HAL_SPI_Transmit(&GC9A01_SPI, &data, 1, HAL_MAX_DELAY);
	GC9A01_Unselect();
}

void GC9A01_WriteDataBuffer(uint8_t *data, uint32_t size)
{
	GC9A01_Select();
	GC9A01_DC_Data();
	/* IMPORTANT: Force blocking transmit for short configuration buffers */
	HAL_SPI_Transmit(&GC9A01_SPI, data, size, HAL_MAX_DELAY);
	GC9A01_Unselect();
}

// === SET ADDRESS WINDOW ===
void GC9A01_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	GC9A01_WriteCommand(0x2A);  // Column
	uint8_t col_data[] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
	GC9A01_WriteDataBuffer(col_data, 4);

	GC9A01_WriteCommand(0x2B);  // Row
	uint8_t row_data[] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};
	GC9A01_WriteDataBuffer(row_data, 4);

	GC9A01_WriteCommand(0x2C);  // Write
}

// === DRAW PIXEL ===
void GC9A01_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	GC9A01_SetAddressWindow(x, y, x, y);
	uint8_t data[] = {color >> 8, color & 0xFF};
	GC9A01_WriteDataBuffer(data, 2);
#if USE_DMA
	while (tx_busy);
#endif
}

// === FILL RECTANGLE ===
void GC9A01_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
	uint32_t size = w * h;
	GC9A01_SetAddressWindow(x, y, x + w - 1, y + h - 1);

	uint8_t buf[512];
	for (int i = 0; i < sizeof(buf); i += 2) {
		buf[i] = color >> 8;
		buf[i + 1] = color & 0xFF;
	}

	while (size > 0) {
		uint16_t chunk = (size > 256) ? 256 : size;
		GC9A01_WriteDataBuffer(buf, chunk * 2);
#if USE_DMA
while (tx_busy);
#endif
size -= chunk;
	}
}

// === FLUSH for LVGL ===
void GC9A01_Flush(const void *color_map, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;

	/* 1. Set window (this safely uses blocking SPI internally now) */
	GC9A01_SetAddressWindow(x1, y1, x2, y2);

	GC9A01_Select();
	GC9A01_DC_Data();

	/* 2. Blast the pixel payload via DMA */
#if USE_DMA
	tx_busy = 1;
	HAL_SPI_Transmit_DMA(&GC9A01_SPI, (uint8_t *)color_map, size);
#else
	HAL_SPI_Transmit(&GC9A01_SPI, (uint8_t *)color_map, size, HAL_MAX_DELAY);
	GC9A01_Unselect();
	GC9A01_FlushReady();
#endif
}

// === DMA CALLBACK ===
#if USE_DMA
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (hspi == &GC9A01_SPI) {
		GC9A01_Unselect();
		tx_busy = 0;
		GC9A01_FlushReady();
	}
}
#endif

// === OPTIONAL WEAK CALLBACK ===
__weak void GC9A01_FlushReady(void)
{
	// Implement in user code:
}

// === DISPLAY INIT ===
void GC9A01_Init(void)
{
	GC9A01_Reset();

	GC9A01_WriteCommand(0xEF);
	GC9A01_WriteCommand(0xEB);
	GC9A01_WriteData(0x14);

	GC9A01_WriteCommand(0xFE);
	GC9A01_WriteCommand(0xEF);

	GC9A01_WriteCommand(0xEB);
	GC9A01_WriteData(0x14);

	GC9A01_WriteCommand(0x84);
	GC9A01_WriteData(0x40);

	GC9A01_WriteCommand(0x85);
	GC9A01_WriteData(0xFF);

	GC9A01_WriteCommand(0x86);
	GC9A01_WriteData(0xFF);

	GC9A01_WriteCommand(0x87);
	GC9A01_WriteData(0xFF);

	GC9A01_WriteCommand(0x88);
	GC9A01_WriteData(0x0A);

	GC9A01_WriteCommand(0x89);
	GC9A01_WriteData(0x21);

	GC9A01_WriteCommand(0x8A);
	GC9A01_WriteData(0x00);

	GC9A01_WriteCommand(0x8B);
	GC9A01_WriteData(0x80);

	GC9A01_WriteCommand(0x8C);
	GC9A01_WriteData(0x01);

	GC9A01_WriteCommand(0x8D);
	GC9A01_WriteData(0x01);

	GC9A01_WriteCommand(0x8E);
	GC9A01_WriteData(0xFF);

	GC9A01_WriteCommand(0x8F);
	GC9A01_WriteData(0xFF);


	GC9A01_WriteCommand(0xB6);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x20);

	GC9A01_WriteCommand(0x36);
	GC9A01_WriteData(0x08);//Set as vertical screen

	GC9A01_WriteCommand(0x3A);
	GC9A01_WriteData(0x05);


	GC9A01_WriteCommand(0x90);
	GC9A01_WriteData(0x08);
	GC9A01_WriteData(0x08);
	GC9A01_WriteData(0x08);
	GC9A01_WriteData(0x08);

	GC9A01_WriteCommand(0xBD);
	GC9A01_WriteData(0x06);

	GC9A01_WriteCommand(0xBC);
	GC9A01_WriteData(0x00);

	GC9A01_WriteCommand(0xFF);
	GC9A01_WriteData(0x60);
	GC9A01_WriteData(0x01);
	GC9A01_WriteData(0x04);

	GC9A01_WriteCommand(0xC3);
	GC9A01_WriteData(0x13);
	GC9A01_WriteCommand(0xC4);
	GC9A01_WriteData(0x13);

	GC9A01_WriteCommand(0xC9);
	GC9A01_WriteData(0x22);

	GC9A01_WriteCommand(0xBE);
	GC9A01_WriteData(0x11);

	GC9A01_WriteCommand(0xE1);
	GC9A01_WriteData(0x10);
	GC9A01_WriteData(0x0E);

	GC9A01_WriteCommand(0xDF);
	GC9A01_WriteData(0x21);
	GC9A01_WriteData(0x0c);
	GC9A01_WriteData(0x02);

	GC9A01_WriteCommand(0xF0);
	GC9A01_WriteData(0x45);
	GC9A01_WriteData(0x09);
	GC9A01_WriteData(0x08);
	GC9A01_WriteData(0x08);
	GC9A01_WriteData(0x26);
	GC9A01_WriteData(0x2A);

	GC9A01_WriteCommand(0xF1);
	GC9A01_WriteData(0x43);
	GC9A01_WriteData(0x70);
	GC9A01_WriteData(0x72);
	GC9A01_WriteData(0x36);
	GC9A01_WriteData(0x37);
	GC9A01_WriteData(0x6F);


	GC9A01_WriteCommand(0xF2);
	GC9A01_WriteData(0x45);
	GC9A01_WriteData(0x09);
	GC9A01_WriteData(0x08);
	GC9A01_WriteData(0x08);
	GC9A01_WriteData(0x26);
	GC9A01_WriteData(0x2A);

	GC9A01_WriteCommand(0xF3);
	GC9A01_WriteData(0x43);
	GC9A01_WriteData(0x70);
	GC9A01_WriteData(0x72);
	GC9A01_WriteData(0x36);
	GC9A01_WriteData(0x37);
	GC9A01_WriteData(0x6F);

	GC9A01_WriteCommand(0xED);
	GC9A01_WriteData(0x1B);
	GC9A01_WriteData(0x0B);

	GC9A01_WriteCommand(0xAE);
	GC9A01_WriteData(0x77);

	GC9A01_WriteCommand(0xCD);
	GC9A01_WriteData(0x63);


	GC9A01_WriteCommand(0x70);
	GC9A01_WriteData(0x07);
	GC9A01_WriteData(0x07);
	GC9A01_WriteData(0x04);
	GC9A01_WriteData(0x0E);
	GC9A01_WriteData(0x0F);
	GC9A01_WriteData(0x09);
	GC9A01_WriteData(0x07);
	GC9A01_WriteData(0x08);
	GC9A01_WriteData(0x03);

	GC9A01_WriteCommand(0xE8);
	GC9A01_WriteData(0x34);

	GC9A01_WriteCommand(0x62);
	GC9A01_WriteData(0x18);
	GC9A01_WriteData(0x0D);
	GC9A01_WriteData(0x71);
	GC9A01_WriteData(0xED);
	GC9A01_WriteData(0x70);
	GC9A01_WriteData(0x70);
	GC9A01_WriteData(0x18);
	GC9A01_WriteData(0x0F);
	GC9A01_WriteData(0x71);
	GC9A01_WriteData(0xEF);
	GC9A01_WriteData(0x70);
	GC9A01_WriteData(0x70);

	GC9A01_WriteCommand(0x63);
	GC9A01_WriteData(0x18);
	GC9A01_WriteData(0x11);
	GC9A01_WriteData(0x71);
	GC9A01_WriteData(0xF1);
	GC9A01_WriteData(0x70);
	GC9A01_WriteData(0x70);
	GC9A01_WriteData(0x18);
	GC9A01_WriteData(0x13);
	GC9A01_WriteData(0x71);
	GC9A01_WriteData(0xF3);
	GC9A01_WriteData(0x70);
	GC9A01_WriteData(0x70);

	GC9A01_WriteCommand(0x64);
	GC9A01_WriteData(0x28);
	GC9A01_WriteData(0x29);
	GC9A01_WriteData(0xF1);
	GC9A01_WriteData(0x01);
	GC9A01_WriteData(0xF1);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x07);

	GC9A01_WriteCommand(0x66);
	GC9A01_WriteData(0x3C);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0xCD);
	GC9A01_WriteData(0x67);
	GC9A01_WriteData(0x45);
	GC9A01_WriteData(0x45);
	GC9A01_WriteData(0x10);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x00);

	GC9A01_WriteCommand(0x67);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x3C);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x01);
	GC9A01_WriteData(0x54);
	GC9A01_WriteData(0x10);
	GC9A01_WriteData(0x32);
	GC9A01_WriteData(0x98);

	GC9A01_WriteCommand(0x74);
	GC9A01_WriteData(0x10);
	GC9A01_WriteData(0x85);
	GC9A01_WriteData(0x80);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x00);
	GC9A01_WriteData(0x4E);
	GC9A01_WriteData(0x00);

	GC9A01_WriteCommand(0x98);
	GC9A01_WriteData(0x3e);
	GC9A01_WriteData(0x07);

	GC9A01_WriteCommand(0x35);
	GC9A01_WriteCommand(0x21);

	GC9A01_WriteCommand(0x11);
	HAL_Delay(120);
	GC9A01_WriteCommand(0x29);
	HAL_Delay(20);
}
