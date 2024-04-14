/* Copyright (C) 2023 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/**************************************************************************//**
 * @file     LED_blink_baremetal.c
 * @author   Girish BN, Manoj A Murudi
 * @email    girish.bn@alifsemi.com, manoj.murudi@alifsemi.com
 * @version  V1.0.0
 * @date     25-May-2023
 * @brief    DEMO application for LED blink.
 * @bug      None.
 * @Note     None.
 ******************************************************************************/

#include "Driver_GPIO.h"
#include "pinconf.h"
#include <stdio.h>
#include <RTE_Components.h>
#include CMSIS_device_header
#if defined(RTE_Compiler_IO_STDOUT)
#include "retarget_stdout.h"
#endif  /* RTE_Compiler_IO_STDOUT */


/* Uncomment to use the pin configuration provided by the conductor tool */
//#define USE_CONDUCTOR_PIN_CONFIG

#ifdef USE_CONDUCTOR_PIN_CONFIG
#include "conductor_board_config.h"
#endif


/* LED0 gpio pins */
#define GPIO0_PORT                      0   /*< Use BUTTON_PURPLE GPIO port >*/
#define PIN0                            0   /*< LED0_B gpio pin >*/
#define PIN1                            1   /*< LED0_G gpio pin >*/

#define BUTTON_STEP 10 // in ms
#define MAX_TIMEOUT 90000 // in ms
#define LONG_PRESS_TIME 30000 // in ms
#define PRE_RELEASE_TIME 2000 // in ms
#define BUTTON_EVENT_QUEUE_SIZE 32

extern ARM_DRIVER_GPIO ARM_Driver_GPIO_(GPIO0_PORT);
ARM_DRIVER_GPIO *gpioDrv0 = &ARM_Driver_GPIO_(GPIO0_PORT);

uint32_t volatile ms_ticks = 0;

typedef enum {
  SHORT_PRESS_EVENT,
  PRESS_AND_HOLD_EVENT,
  LONG_PRESS_RELEASE_EVENT,
  BOTH_BUTTONS_PRESSED_EVENT,
  BUTTON_ERROR_EVENT,
} button_event_t;

typedef struct  {
    button_event_t items[BUTTON_EVENT_QUEUE_SIZE];
    int front;
    int rear;
    int size;
} button_event_queue_t;



button_event_queue_t button_event_queue;

void SysTick_Handler (void) {
  ms_ticks++;
}
void delay(uint32_t nticks)
{
      uint32_t c_ticks;

      c_ticks = ms_ticks;
      while ((ms_ticks - c_ticks) < nticks) ;
}


uint8_t get_buttons(ARM_DRIVER_GPIO *dev) {
  uint8_t buttons = 0;
  uint32_t button_value = 1;
  dev->GetValue(PIN0, &button_value);
  if (button_value == 0) {
    buttons |= 0x1;
  }
  dev->GetValue(PIN1, &button_value);
  if (button_value == 0) {
    buttons |= 0x2;
  }
  return buttons;
}

void reset_button_event_queue( button_event_queue_t *queue) {
    queue->size = 0;
    queue->front = -1; // Empty queue
    queue->rear = -1;  // Empty queue
    for (int i = 0; i < BUTTON_EVENT_QUEUE_SIZE; ++i) {
      queue->items[i] = BUTTON_ERROR_EVENT;
    }
}

void enqueue( button_event_queue_t *queue, button_event_t value) {
    if (queue->size == BUTTON_EVENT_QUEUE_SIZE) {
        printf("Queue is full. Cannot enqueue.\n");
        return;
    }
    if (queue->size == 0) {
        queue->front = 0; // If the queue was empty, set front to 0
    }
    queue->rear = (queue->rear + 1) % BUTTON_EVENT_QUEUE_SIZE;//corcular queue
    queue->items[queue->rear] = value;
    queue->size++;
}

button_event_t dequeue( button_event_queue_t *queue) {
    if (queue->size == 0) {
        printf("Queue is empty. Cannot dequeue.\n");
        return BUTTON_ERROR_EVENT;
    }
    button_event_t removedItem = queue->items[queue->front];
    queue->front = (queue->front + 1) % BUTTON_EVENT_QUEUE_SIZE;
    queue->size--;
    return removedItem;
}


void* button_monitor(void *arg) {

  enum state_t {IDLE, DEBOUNCE, BUTTON_PRESSED, BUTTON_RELEASED, 
    SHORT_PRESS_CONFIRMED, LONG_PRESS_CONFIRMED, BUTTON_HELD};
  enum state_t state = IDLE;
  int timer = 0;
  uint8_t buttons;
  while (1) {
    switch (state)
    {
    case IDLE:
      delay(BUTTON_STEP);
      buttons = get_buttons(&gpioDrv0);

      if (buttons == 0x3) {
        state = IDLE;
      } else {
        printf("button pressed, but need to confirm, 0x%x\n", buttons);
        state = DEBOUNCE;
      }
      timer = 0;
      break;
    
    case DEBOUNCE:
      delay(100);
      buttons = get_buttons(&gpioDrv0);
      if (buttons == 0x3) {
        state = IDLE;
      } else {
        printf("button press confirmed,0x%x\n", buttons);
        state = BUTTON_PRESSED;
      }
      break;

    
    case BUTTON_PRESSED:
      do {
        delay(BUTTON_STEP);
        buttons = get_buttons(&gpioDrv0);
        timer += BUTTON_STEP;
        if (timer > LONG_PRESS_TIME) {
          break;
        }
      } while (buttons != 0x3);

      if (timer > LONG_PRESS_TIME) {
        printf("button held,0x%x\n", buttons);
        if (buttons == 0x0){
          enqueue(&button_event_queue, BOTH_BUTTONS_PRESSED_EVENT);
          printf("Both button press confirmed,0x%x\n", buttons);
          state = BUTTON_HELD;
        } else {
          enqueue(&button_event_queue, PRESS_AND_HOLD_EVENT);
          printf("One button press and hold confirmed,0x%x\n", buttons);
          state = BUTTON_HELD;
        }
        timer = 0;
        break;
      } else {
        printf("button short press confirmed,0x%x\n", buttons);
        enqueue(&button_event_queue, SHORT_PRESS_EVENT);
        timer = 0;
        state = IDLE;
        break;
      }    
      break;
    
    case BUTTON_HELD:
      do {
        delay(BUTTON_STEP);
        buttons = get_buttons(&gpioDrv0);
      } while (buttons != 0x3);
      printf("button released,0x%x\n", buttons);
      enqueue(&button_event_queue, LONG_PRESS_RELEASE_EVENT);
      state = BUTTON_RELEASED;
      break;

    case BUTTON_RELEASED:
      state = IDLE;
      timer = 0;
      break;

    default:
      state = IDLE;
      timer = 0;
      break;
    }
  }
  
}

/**
  \fn         void led_blink_app(void)
  \brief      LED blinky function
  \param[in]  none
  \return     none
*/
void led_blink_app (void)
{
  /*
   * gpio12 pin3 can be used as Red LED of LED0.
   * gpio7 pin4 can be used as Green LED of LED0.
   * gpio12 pin0 can be used as Blue LED of LED0.
   *
   * gpio6 pin2 can be used as Red LED of LED1.
   * gpio6 pin4 can be used as Green LED of LED1.
   * gpio6 pin6 can be used as Blue LED of LED1.
   *
   * This demo application is about.
   *   - Blink LED0_R and LED1_R, then LED0_B and LED1_B, then LED0_G and LED1_G simultaneously in rotation.
   */

    int32_t ret1 = 0;
    int32_t ret2 = 0;
    uint8_t BUTTON_PURPLE = PIN1;
    uint32_t button_value = 1;
    uint32_t button_pad_config = PADCTRL_READ_ENABLE | PADCTRL_DRIVER_DISABLED_PULL_UP;
    

    printf("led blink demo application started\n\n");

#ifdef USE_CONDUCTOR_PIN_CONFIG
    ret1 = conductor_pins_config();

    if (ret1 != 0) {
        printf("ERROR: Conductor pin configuration failed\n");
        return;
    }
#else
    /* pinmux configurations for all GPIOs */
    pinconf_set(GPIO0_PORT, BUTTON_PURPLE, PINMUX_ALTERNATE_FUNCTION_0, button_pad_config);
#endif

    ret1 = gpioDrv0->Initialize(BUTTON_PURPLE, NULL);
    if ((ret1 != ARM_DRIVER_OK)) {
        printf("ERROR: Failed to initialize\n");
        return;
    }

    ret1 = gpioDrv0->PowerControl(BUTTON_PURPLE, ARM_POWER_FULL);
    if ((ret1 != ARM_DRIVER_OK)) {
        printf("ERROR: Failed to powered full\n");
        goto error_uninitialize;
    }

    ret1 = gpioDrv0->SetDirection(BUTTON_PURPLE, GPIO_PIN_DIRECTION_INPUT);
    if ((ret1 != ARM_DRIVER_OK)) {
        printf("ERROR: Failed to configure\n");
        goto error_power_off;
    }

    while (1)
    {
        delay(1000);

        ret1 = gpioDrv0->GetValue(BUTTON_PURPLE,&button_value);
        if(button_value == 1)
        {
            printf("Button is pressed\n");
        }
        else
        {
            printf("Button is not pressed\n");
        }
    }

error_power_off:
error_uninitialize:
	while(1);
}

/* Define main entry point.  */
int main (void)
{
    #if defined(RTE_Compiler_IO_STDOUT_User)
    int32_t ret;
    ret = stdout_init();
    if(ret != ARM_DRIVER_OK)
    {
        while(1)
        {
        }
    }
    #endif
    /* Configure Systick for each millisec */
    SysTick_Config(SystemCoreClock/1000);

    // led_blink_app();
    button_monitor(NULL);
    return 0;
}

