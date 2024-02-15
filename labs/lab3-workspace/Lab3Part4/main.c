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

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F
#define	GREEN           0x07E0
#define CYAN            0x07FF
#define	RED             0xF800
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0
#define WHITE           0xFFFF
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
unsigned long buffer[100];
volatile int butPressedCount = 0;
volatile int msgReadyFlag = 0;

volatile int prevSize = 0;
volatile int compSize = 0;
volatile int reciSize = 0;

volatile int comp_x = 5, comp_y = 68;
volatile int reci_x = 5, reci_y = 4;

extern void (* const g_pfnVectors[])(void);
//*****************************************************************************
//                  GLOBAL VARIABLES -- End
//*****************************************************************************

typedef struct Letter {
    unsigned int x;
    unsigned int y;
    char letter;
} Letter;

Letter compMessage[100];
Letter reciMessage[100];
Letter prevMessage[100];

//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//****************************************************************************

void IRHandler();
void Display(unsigned long value);
unsigned long Decode(unsigned long* buffer);
char LetterCalc(unsigned long value);
char AdvanceLetter(char letter, unsigned long value);

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
    if(count == 35) {
        flag = 1;
        count = 0;
        Timer_IF_Start(TIMERA2_BASE, TIMER_A, 600);
    }
    temp = TimerValueGet(TIMERA0_BASE, TIMER_A) >> 17;
    if(temp == 58 || temp == 59) {
        flag = 0;
        count = -2;
        Timer_IF_Start(TIMERA1_BASE, TIMER_A, 200);
    }
    buffer[count] = temp;
    if (count == 2 && buffer[count] != 5) {
        count = 0;
    }
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);
}

static void RepeatHandler(void)
{
    Timer_IF_InterruptClear(TIMERA1_BASE);
}

static void TimeoutHandler(void)
{
    Timer_IF_InterruptClear(TIMERA2_BASE);
    previous = -1;
}

void SendMessage(void) {
    //Send Message
    if(compSize > 0) {
        int i;
        for(i = 0; i < compSize; i++) {
            while(UARTBusy(UARTA1_BASE));
            UARTCharPut(UARTA1_BASE, compMessage[i].letter);
        }
        UARTCharPut(UARTA1_BASE,'\0');
        for(i = 0; i < compSize; i++) {
            drawChar(compMessage[i].x, compMessage[i].y, compMessage[i].letter, BLACK, BLACK, 1);
        }
        compSize = 0;
        comp_x = 5;
        comp_y = 68;
    }
}

void IRHandler(void) {
    // Steps for part 4
    // Keep track of the number of times this button has been pressed if repeated
    // Set a delay inbetween buttons pressed
        // If the delay is too long, reset the count and print out the letter
        // If the button is pressed fast and is the a different button, reset the count and print out the letter
        // If the button is pressed fast and is the same button, increment the count

    // DO THIS IN THE IR INTERRUPT HANDLER
    if (flag == 1) {
        flag = 0;
        current = Decode(buffer + 19);
        if(previous == current) {
           same = 1;
        }
        else {
           same = 0;
        }
        previous = current;

        if(LetterCalc(current) == '-') {
            if(compSize > 0) {
                compSize--;
                drawChar(compMessage[compSize].x, compMessage[compSize].y, compMessage[compSize].letter, BLACK, BLACK, 1);
            }
            if(comp_x >= 12) {
                comp_x -= 7;
            }
            else if(comp_x == 5) {
                if(comp_y >= 78) {
                    comp_y -= 10;
                    comp_x = 117;
                }
            }
        }

        else if(LetterCalc(current) == '+') {
            SendMessage();
        }
        else {
            char letter;
            letter = LetterCalc(current);
            if(compSize < 100) {
                if(same == 1) {
                    int index = compSize - 1;
                    char let = compMessage[index].letter;

                    drawChar(compMessage[index].x, compMessage[index].y, let, BLACK, BLACK, 1);

                    compMessage[index].letter = AdvanceLetter(let, current);
                    drawChar(compMessage[index].x, compMessage[index].y, compMessage[index].letter, BLUE, BLUE, 1);
                }

                else {
                    Letter newLet;
                    newLet.x = comp_x;
                    newLet.y = comp_y;
                    newLet.letter = letter;
                    compMessage[compSize] = newLet;

                    drawChar(comp_x, comp_y, letter, BLUE, BLUE, 1);

                    comp_x += 7;
                    if(comp_x >= 124) {
                        comp_x = 5;
                        comp_y += 10;
                    }

                    compSize++;
                }
            }
        }
    }
}

void UARTIntHandler(void) {
    unsigned long ulStatus;
    ulStatus = UARTIntStatus(UARTA1_BASE, true);
    UARTIntClear(UARTA1_BASE, ulStatus);
    while(UARTCharsAvail(UARTA1_BASE))
    {
        char c = UARTCharGet(UARTA1_BASE);
        if(c == '\0') {
            msgReadyFlag = 1;
        }
        else {
            reciMessage[reciSize].letter = c;
            reciMessage[reciSize].x = reci_x;
            reciMessage[reciSize].y = reci_y;
            reciSize++;
            reci_x += 7;
            if(reci_x >= 124) {
                reci_x = 5;
                reci_y += 10;
            }
        }
    }
}

char LetterCalc(unsigned long value) {
    char letter;
    switch(value) {
        case ZERO:
            letter = ' ';
            break;
        case ONE:
            letter = '?';
            break;
        case TWO:
            letter = 'a';
            break;
        case THREE:
            letter = 'd';
            break;
        case FOUR:
            letter = 'g';
            break;
        case FIVE:
            letter = 'j';
            break;
        case SIX:
            letter = 'm';
            break;
        case SEVEN:
            letter = 'p';
            break;
        case EIGHT:
            letter = 't';
            break;
        case NINE:
            letter = 'w';
            break;
        case MUTE:
            letter = '-';
            break;
        case LAST:
            letter = '+';
            break;
        default:
            letter = '/';
            break;
    }
    return letter;
}

char AdvanceLetter(char letter, unsigned long value) {
    char newLetter;
    switch(value) {
        case ONE:
            if(letter == '?') { newLetter = '.'; }
            else if(letter == '.') { newLetter = ','; }
            else { newLetter = '?'; }
            break;
        case TWO:
            if(letter == 'c') { newLetter = 'a'; }
            else { newLetter = letter + 1; }
            break;
        case THREE:
            if(letter == 'f') { newLetter = 'd'; }
            else { newLetter = letter + 1; }
            break;
        case FOUR:
            if(letter == 'i') { newLetter = 'g'; }
            else {newLetter = letter + 1; }
            break;
        case FIVE:
            if(letter == 'l') { newLetter = 'j'; }
            else { newLetter = letter + 1; }
            break;
        case SIX:
            if(letter == 'o') { newLetter = 'm'; }
            else { newLetter = letter + 1; }
            break;
        case SEVEN:
            if(letter == 's') { newLetter = 'p'; }
            else { newLetter = letter + 1; }
            break;
        case EIGHT:
            if(letter == 'v') { newLetter = 't'; }
            else { newLetter = letter + 1; }
            break;
        case NINE:
            if(letter == 'z') { newLetter = 'w'; }
            else { newLetter = letter + 1; }
            break;
        default:
            newLetter = letter;
            break;
    }
    return newLetter;
}

void DisplayMessage() {
    if(msgReadyFlag) {
        msgReadyFlag = 0;
        int i;
        for(i = 0; i < prevSize; i++) {
            drawChar(prevMessage[i].x, prevMessage[i].y, prevMessage[i].letter, BLACK, BLACK, 1);
        }
        for(i = 0; i < reciSize; i++) {
            drawChar(reciMessage[i].x, reciMessage[i].y, reciMessage[i].letter, RED, RED, 1);
            prevMessage[i] = reciMessage[i];
        }
        prevSize = reciSize;
        reci_x = 5;
        reci_y = 4;
        reciSize = 0;
    }
}

unsigned long Decode(unsigned long* buffer) {
    unsigned long value = 0;
    int i;
    for(i = 0; i < 16; i++) {
        value += *(buffer + i) << (15 - i);
    }
    return value;
}


// UART APIs to start with
// MAP_UARTConfigSetExpClk(CONSOLE, MAP_PRCMPeripheralClockGet(CONSOLE),
//                         UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
//                         UART_CONFIG_PAR_NONE));
// MAP_UARTIntRegister(CONSOLE, UARTIntHandler);
// // Clear interupts by getting status
// MAP_UARTIntStatus(CONSOLE, true);
// MAP_UARTIntEnable(CONSOLE, UART_INT_RX | UART_INT_RT);
void InitBoardUART(){
    MAP_UARTConfigSetExpClk(UARTA1_BASE,MAP_PRCMPeripheralClockGet(UARTA1_BASE),
                            UART_BAUD_RATE, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                            UART_CONFIG_PAR_NONE));
    
    // MAP_UARTIntRegister(UARTA1_BASE, UARTIntHandler);
    // MAP_UARTIntStatus(UARTA1_BASE, true);
    // MAP_UARTIntEnable(UARTA1_BASE, UART_INT_RX | UART_INT_RT);
}
void testhelloworld() {
    fillScreen(BLACK);
    drawChar(5, 64, 'H', RED, BLUE, 2);
    drawChar(15, 64, 'e', RED, BLUE, 2);
    drawChar(25, 64, 'l', RED, BLUE, 2);
    drawChar(35, 64, 'l', RED, BLUE, 2);
    drawChar(45, 64, 'o', RED, BLUE, 2);
    drawChar(65, 64, 'W', RED, BLUE, 2);
    drawChar(75, 64, 'o', RED, BLUE, 2);
    drawChar(85, 64, 'r', RED, BLUE, 2);
    drawChar(95, 64, 'l', RED, BLUE, 2);
    drawChar(105, 64, 'd', RED, BLUE, 2);
    drawChar(115, 64, '!', RED, BLUE, 2);
}

void main()
{
    unsigned long ulStatus;
    count = 0;
    flag = 0;
    current = 0;
    previous = 0;

    // Init the board
    BoardInit();
    PinMuxConfig();

    // Init the UART for Console
    InitTerm();
    ClearTerm();

    // Init the UART for the board
    InitBoardUART();

    // Init the OLED
    Adafruit_Init();
    fillScreen(BLACK);
    
    // Init the GPIO for the IR sensor
    GPIOIntRegister(GPIOA0_BASE, GPIOIntHandler);
    GPIOIntTypeSet(GPIOA0_BASE, 0x80, GPIO_FALLING_EDGE);
    ulStatus = GPIOIntStatus (GPIOA0_BASE, false);
    GPIOIntClear(GPIOA0_BASE, ulStatus);
    GPIOIntEnable(GPIOA0_BASE, 0x80);

    // Init the Interupt for baord to baord UART
    UARTIntRegister(UARTA1_BASE, UARTIntHandler);
    MAP_UARTIntEnable(UARTA1_BASE, UART_INT_RX | UART_INT_RT);

    Timer_IF_Init(PRCM_TIMERA2, TIMERA2_BASE, TIMER_CFG_ONE_SHOT, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA2_BASE, TIMER_A, TimeoutHandler);

    Timer_IF_Init(PRCM_TIMERA1, TIMERA1_BASE, TIMER_CFG_ONE_SHOT, TIMER_A, 0);
    Timer_IF_IntSetup(TIMERA1_BASE, TIMER_A, RepeatHandler);

    // Init the Timer for the IR sensor input
    Timer_IF_Init(PRCM_TIMERA0, TIMERA0_BASE, TIMER_CFG_PERIODIC_UP, TIMER_A, 0);
    TimerEnable(TIMERA0_BASE, TIMER_A);
    TimerValueSet(TIMERA0_BASE, TIMER_A, 0);

    while (1)
    {
        IRHandler();
        DisplayMessage();
    }
}
