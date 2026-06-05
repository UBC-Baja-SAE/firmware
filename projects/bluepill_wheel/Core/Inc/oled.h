#ifndef BLUEPILL_WHEEL_COLED_H
#define BLUEPILL_WHEEL_COLED_H



extern int Suspension_Mode;

void displaySuspensionMode(int Suspension_Mode);
void drawBars();
void updateDampingDisplay(int fl, int fr, int rl, int rr);

#endif //BLUEPILL_WHEEL_COLED_H