#pragma once
#include "main.h"

// ─── Potentiometer / position ────────────────────────────────────────────────

#define X_EXTEND_CM  10.0f
#define X_COMP_CM     0.0f
#define V_EXTEND      1.8f
#define V_COMP        0.0f

float VoltageToPosition(float voltage);
void  SendPotOnCan(uint32_t can_id);

// ─── DRV8461 register addresses ──────────────────────────────────────────────

#define DRV8461_REG_FAULT 0x00
#define DRV8461_REG_DIAG1 0x01
#define DRV8461_REG_CTRL1 0x02
#define DRV8461_REG_CTRL2 0x03
#define DRV8461_REG_CTRL3 0x04
#define DRV8461_REG_CTRL4 0x05
#define DRV8461_REG_CTRL5 0x06
#define DRV8461_REG_CTRL6 0x07
#define DRV8461_REG_CTRL7 0x08

#define DRV8461_READ_FLAG   0x40   // set bit6 for a read transaction

// ─── Motor identifiers ───────────────────────────────────────────────────────

typedef enum {
    MOTOR_NEMA17 = 0,
    MOTOR_NEMA23 = 1
} DRV8461_Motor;

// ─── Public API ──────────────────────────────────────────────────────────────

void    DWT_Init(void);
void    delay_us(uint32_t us);
void    DRV8461_InitAll(void);
void    DRV8461_Write(DRV8461_Motor motor, uint8_t reg, uint8_t data);
uint8_t DRV8461_Read(DRV8461_Motor motor, uint8_t reg);
uint8_t DRV8461_GetFault(DRV8461_Motor motor);
void    DRV8461_ClearFault(DRV8461_Motor motor);
void    DRV8461_Enable(DRV8461_Motor motor, uint8_t enable);
void    DRV8461_SetDirection(DRV8461_Motor motor, uint8_t dir);
void    DRV8461_Step(DRV8461_Motor motor);
void    RunMotorTimerTest(void);
