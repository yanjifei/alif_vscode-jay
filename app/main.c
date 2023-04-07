#include "RTE_Components.h"
#include CMSIS_device_header

#include "Driver_GPIO.h"

#define GET_DRIVER_REF(ref, peri, chan) \
    extern ARM_DRIVER_##peri Driver_##peri##chan; \
    static ARM_DRIVER_##peri * ref = &Driver_##peri##chan;

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

#ifdef CORE_M55_HE
    SysTick_Config(SystemCoreClock/10);
#else
    SysTick_Config(SystemCoreClock/25);
#endif

    while (1) __WFI();
}

void SysTick_Handler (void)
{
#ifdef CORE_M55_HE
    gpio1->SetValue(14, GPIO_PIN_OUTPUT_STATE_TOGGLE);
#else
    gpio1->SetValue(15, GPIO_PIN_OUTPUT_STATE_TOGGLE);
#endif
}

// Stubs to suppress missing stdio definitions for nosys
#define TRAP_RET_ZERO  {__BKPT(0); return 0;}
int _close(int val) TRAP_RET_ZERO
int _lseek(int val0, int val1, int val2) TRAP_RET_ZERO
int _read(int val0, char * val1, int val2) TRAP_RET_ZERO
int _write(int val0, char * val1, int val2) TRAP_RET_ZERO