// Standard includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Driverlib includes
#include "hw_types.h"
#include "hw_ints.h"
#include "hw_memmap.h"
#include "hw_common_reg.h"
#include "pin.h"
#include "interrupt.h"
#include "hw_apps_rcm.h"
#include "spi.h"
#include "uart.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "prcm.h"
#include "gpio.h"
#include "utils.h"
#include "timer.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"
#include "glcdfont.h"

// Common interface includes
#include "uart_if.h"
#include "i2c_if.h"
#include "gpio_if.h"
#include "timer_if.h"

#include "pin_mux_config.h"

//*****************************************************************************
//                      MACRO DEFINITIONS
//*****************************************************************************
#define APPLICATION_VERSION "1.4.0"
#define APP_NAME "I2C Demo"
#define UART_PRINT Report
#define FOREVER 1
#define CONSOLE UARTA0_BASE
#define FAILURE -1
#define SUCCESS 0
#define RETERR_IF_TRUE(condition) \
    {                             \
        if (condition)            \
            return FAILURE;       \
    }
#define RET_IF_ERR(Func)        \
    {                           \
        int iRetVal = (Func);   \
        if (SUCCESS != iRetVal) \
            return iRetVal;     \
    }

#define BMA222_ADDRESS 0x18
#define BASE_OFFSET 0x2
#define X_OFFSET 0x3
#define Y_OFFSET 0x5

#define ONE   0x00FF
#define TWO   0x807F
#define THREE 0x40BF
#define FOUR  0xC03F
#define FIVE  0x20DF
#define SIX   0xA05F
#define SEVEN 0x609F
#define EIGHT 0xE01F
#define NINE  0x10EF
#define ZERO  0x906F
#define MUTE  0x708F
#define LAST  0x08F7
//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs)
    extern void (*const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
    extern uVectorEntry __vector_table;
#endif

#define SYSCLKFREQ 80000000ULL

#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))\

#define US_TO_TICKS(us) ((SYSCLKFREQ / 1000000ULL) * (us))

#define SYSTICK_RELOAD_VAL 3200000UL

volatile int count = 0;
volatile int flag = 0;
volatile unsigned long temp = 0;
volatile int current = 0;
volatile int previous = 0;
volatile int same = 0;
volatile int pressed = 0;
unsigned long buffer[1000];

extern void (* const g_pfnVectors[])(void);
//*****************************************************************************
//                  GLOBAL VARIABLES -- End
//*****************************************************************************

//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//****************************************************************************

void IRHandler();
void Display(unsigned long value);
unsigned long Decode(unsigned long* buffer);

static void BoardInit(void) {
    #ifndef USE_TIRTOS
        #if defined(ccs)
            MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
        #endif
        #if defined(ewarm)
            MAP_IntVTableBaseSet((unsigned long)&__vector_table);
        #endif
    #endif
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);
    PRCMCC3200MCUInit();
}

static void GPIOIntHandler(void) {
    unsigned long ulStatus;
    ulStatus = GPIOIntStatus(GPIOA0_BASE, true);
    GPIOIntClear(GPIOA0_BASE, ulStatus);
    count++;
    if(count == 37) {
        flag = 1;
        count = 0;
        Timer_IF_Start(TIMERA1_BASE, TIMER_A, 400);
    }
    temp = TimerValueGet(TIMERA0_BASE, TIMER_A) >> 17;
    Report("Count %d = Value: %d\n\r", count, temp);
    if(temp == 58 || temp == 59) {
        flag = 0;
        count = -2;
        Timer_IF_Start(TIMERA1_BASE, TIMER_A, 400);
    }
    buffer[count] = temp;
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);
}

static void RepeatHandler(void)
{
    Timer_IF_InterruptClear(TIMERA1_BASE);
}

void IRHandler(void) {
    if (flag == 1) {
        flag = 0;
        current = Decode(buffer + 18);
        Display(current);
        if(previous == current) {
           same = 1;
        }
        else {
           same = 0;
        }
        previous = current;
    }
}

void Display(unsigned long value) {
    switch(value) {
        case ZERO:
            Report("You pressed 0.\n\r");
            break;
        case ONE:
            Report("You pressed 1.\n\r");
            break;
        case TWO:
            Report("You pressed 2.\n\r");
            break;
        case THREE:
            Report("You pressed 3.\n\r");
            break;
        case FOUR:
            Report("You pressed 4.\n\r");
            break;
        case FIVE:
            Report("You pressed 5.\n\r");
            break;
        case SIX:
            Report("You pressed 6.\n\r");
            break;
        case SEVEN:
            Report("You pressed 7.\n\r");
            break;
        case EIGHT:
            Report("You pressed 8.\n\r");
            break;
        case NINE:
            Report("You pressed 9.\n\r");
            break;
        case MUTE:
            Report("You pressed MUTE.\n\r");
            break;
        case (LAST):
            Report("You pressed LAST.\n\r");
            break;
        default:
            break;
    }
}

unsigned long Decode(unsigned long* buffer) {
    unsigned long value = 0;
    int i;
    for(i = 0; i < 16; i++) {
        value += *(buffer + i) << (15 - i);
    }
//    Report("Binary: %x\n\r", value);
    return value;
}

void main()
{
    unsigned long ulStatus;
    count = 0;
    flag = 0;
    current = 0;
    previous = 0;

    BoardInit();
    PinMuxConfig();
    InitTerm();
    ClearTerm();

    GPIOIntRegister(GPIOA0_BASE, GPIOIntHandler);
    GPIOIntTypeSet(GPIOA0_BASE, 0x80, GPIO_FALLING_EDGE);
    ulStatus = GPIOIntStatus (GPIOA0_BASE, false);
    GPIOIntClear(GPIOA0_BASE, ulStatus);
    GPIOIntEnable(GPIOA0_BASE, 0x80);

    Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_ONE_SHOT, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, RepeatHandler);

    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 0);
    TimerEnable(TIMERA0_BASE, TIMER_A);
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);

    while (1)
    {
        //IRHandler();
    }
}
