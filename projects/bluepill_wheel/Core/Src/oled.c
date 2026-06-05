#include "oled.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

int Suspension_Mode;

void drawBars(void) {
  // FL
  ssd1306_DrawRectangle(1, 6, 64, 16, White);
  ssd1306_SetCursor(4, 8);
  ssd1306_WriteString("FL", Font_6x8, White);

  // FR
  ssd1306_DrawRectangle(1, 21, 64, 31, White);
  ssd1306_SetCursor(4, 23);
  ssd1306_WriteString("FR", Font_6x8, White);

  // RL
  ssd1306_DrawRectangle(1, 36, 64, 46, White);
  ssd1306_SetCursor(4, 38);
  ssd1306_WriteString("RL", Font_6x8, White);

  // RR
  ssd1306_DrawRectangle(1, 51, 64, 61, White);
  ssd1306_SetCursor(4, 53);
  ssd1306_WriteString("RR", Font_6x8, White);

  // Icon box
  ssd1306_DrawRectangle(68, 6, 124, 62, White);
}

void updateDampingDisplay(int fl, int fr, int rl, int rr) {
  const int max_val = 100;
  const int max_len = 44; // pixel width: 61 - 17 = 44

  // Clamp values
  if (fl > max_val) fl = max_val;
  if (fl < 0)       fl = 0;
  if (fr > max_val) fr = max_val;
  if (fr < 0)       fr = 0;
  if (rl > max_val) rl = max_val;
  if (rl < 0)       rl = 0;
  if (rr > max_val) rr = max_val;
  if (rr < 0)       rr = 0;

  // Map values to pixel lengths
  int len_fl = (fl * max_len) / max_val;
  int len_fr = (fr * max_len) / max_val;
  int len_rl = (rl * max_len) / max_val;
  int len_rr = (rr * max_len) / max_val;

  // FL bar
  ssd1306_FillRectangle(16, 7, 63, 15, Black);
  if (len_fl > 0) ssd1306_FillRectangle(17, 9, 17 + len_fl, 13, White);

  // FR bar
  ssd1306_FillRectangle(16, 22, 63, 30, Black);
  if (len_fr > 0) ssd1306_FillRectangle(17, 24, 17 + len_fr, 28, White);

  // RL bar
  ssd1306_FillRectangle(16, 37, 63, 45, Black);
  if (len_rl > 0) ssd1306_FillRectangle(17, 39, 17 + len_rl, 43, White);

  // RR bar
  ssd1306_FillRectangle(16, 52, 63, 60, Black);
  if (len_rr > 0) ssd1306_FillRectangle(17, 54, 17 + len_rr, 58, White);
}