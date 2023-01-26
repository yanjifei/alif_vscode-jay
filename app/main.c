#include "RTE_Components.h"
#include CMSIS_device_header

#include "Driver_GPIO.h"

#define GET_DRIVER_REF(ref, peri, chan) \
    extern ARM_DRIVER_##peri ARM_Driver_##peri##_(chan); \
    ARM_DRIVER_##peri * ref = &ARM_Driver_##peri##_(chan);

GET_DRIVER_REF(gpio1, GPIO, 1);

void main (void)
{

    gpio1->Initialize(14, NULL);
    gpio1->PowerControl(14, ARM_POWER_FULL);
    gpio1->SetDirection(14, GPIO_PIN_DIRECTION_OUTPUT);
    gpio1->SetValue(14, GPIO_PIN_OUTPUT_STATE_HIGH);

    gpio1->Initialize(15, NULL);
    gpio1->PowerControl(15, ARM_POWER_FULL);
    gpio1->SetDirection(15, GPIO_PIN_DIRECTION_OUTPUT);
    gpio1->SetValue(15, GPIO_PIN_OUTPUT_STATE_LOW);

    SysTick_Config(SystemCoreClock/10);

    while (1) __WFI();

    __BKPT(0);
}

void SysTick_Handler (void)
{
    gpio1->SetValue(14, GPIO_PIN_OUTPUT_STATE_TOGGLE);
    gpio1->SetValue(15, GPIO_PIN_OUTPUT_STATE_TOGGLE);
}