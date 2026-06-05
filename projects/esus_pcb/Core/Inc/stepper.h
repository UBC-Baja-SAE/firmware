#ifndef STEPPER_H
#define STEPPER_H

#include "main.h"

typedef enum {
    MOTOR_NEMA17 = 0,
    MOTOR_NEMA23 = 1
} MotorID_t;

typedef struct {
    // Control pins
    GPIO_TypeDef *nsleep_port; uint16_t nsleep_pin;
    GPIO_TypeDef *enable_port; uint16_t enable_pin;
    GPIO_TypeDef *dir_port;    uint16_t dir_pin;
    GPIO_TypeDef *step_port;   uint16_t step_pin;
    GPIO_TypeDef *nscs_port;   uint16_t nscs_pin;
    GPIO_TypeDef *nfault_port; uint16_t nfault_pin;
} StepperMotor_t;

void Stepper_Init(void);
void Stepper_Enable(MotorID_t id);
void Stepper_Disable(MotorID_t id);
void Stepper_Sleep(MotorID_t id);
void Stepper_Wake(MotorID_t id);
void Stepper_SetDir(MotorID_t id, uint8_t forward);
void Stepper_Step(MotorID_t id, uint32_t steps, uint32_t delay_us);
uint8_t Stepper_IsFault(MotorID_t id);
void Stepper_ClearFault(MotorID_t id);

// DRV8461 SPI register access
uint16_t DRV8461_ReadReg(MotorID_t id, uint8_t reg);
void     DRV8461_WriteReg(MotorID_t id, uint8_t reg, uint8_t data);

// DRV8461 Register addresses
#define DRV8461_REG_FAULT    0x00
#define DRV8461_REG_DIAG1    0x01
#define DRV8461_REG_DIAG2    0x02
#define DRV8461_REG_CTRL1    0x03
#define DRV8461_REG_CTRL2    0x04
#define DRV8461_REG_CTRL3    0x05
#define DRV8461_REG_CTRL4    0x06
#define DRV8461_REG_CTRL5    0x07
#define DRV8461_REG_CTRL6    0x08
#define DRV8461_REG_CTRL7    0x09
#define DRV8461_REG_CTRL8    0x0A
#define DRV8461_REG_CTRL9    0x0B
#define DRV8461_REG_CTRL10   0x0C
#define DRV8461_REG_CTRL11   0x0D
#define DRV8461_REG_CTRL12   0x0E
#define DRV8461_REG_CTRL13   0x0F
#define DRV8461_REG_INDEX1   0x10
#define DRV8461_REG_INDEX2   0x11
#define DRV8461_REG_INDEX3   0x12
#define DRV8461_REG_INDEX4   0x13
#define DRV8461_REG_INDEX5   0x14

#endif