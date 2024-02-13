//*****************************************************************************
//
// Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
//
// Application Name     - I2C
// Application Overview - The objective of this application is act as an I2C
//                        diagnostic tool. The demo application is a generic
//                        implementation that allows the user to communicate
//                        with any I2C device over the lines.
//
//*****************************************************************************

//*****************************************************************************
//
//! \addtogroup i2c_demo
//! @{
//
//*****************************************************************************

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
#include "rom.h"
#include "rom_map.h"
#include "interrupt.h"
#include "prcm.h"
#include "utils.h"
#include "uart.h"
#include "spi.h"
#include "systick.h"
#include "hw_nvic.h"

// Common interface includes
#include "uart_if.h"
#include "i2c_if.h"
#include "gpio_if.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1351.h"

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

//*****************************************************************************
//                 GLOBAL VARIABLES -- Start
//*****************************************************************************
#if defined(ccs)
extern void (*const g_pfnVectors[])(void);
#endif
#if defined(ewarm)
extern uVectorEntry __vector_table;
#endif


// some helpful macros for systick

// the cc3200's fixed clock frequency of 80 MHz
// note the use of ULL to indicate an unsigned long long constant
#define SYSCLKFREQ 80000000ULL

// macro to convert ticks to microseconds
#define TICKS_TO_US(ticks) \
    ((((ticks) / SYSCLKFREQ) * 1000000ULL) + \
    ((((ticks) % SYSCLKFREQ) * 1000000ULL) / SYSCLKFREQ))\

// macro to convert microseconds to ticks
#define US_TO_TICKS(us) ((SYSCLKFREQ / 1000000ULL) * (us))

// systick reload value set to 40ms period
// (PERIOD_SEC) * (SYSCLKFREQ) = PERIOD_TICKS
#define SYSTICK_RELOAD_VAL 3200000UL

// track systick counter periods elapsed
// if it is not 0, we know the transmission ended
volatile int systick_cnt = 0;

extern void (* const g_pfnVectors[])(void);

// static unsigned char aucRdDataBuf[256];
//*****************************************************************************
//                  GLOBAL VARIABLES -- End
//*****************************************************************************

//****************************************************************************
//                      LOCAL FUNCTION DEFINITIONS
//****************************************************************************
/**
 * Reset SysTick Counter
 */
static inline void SysTickReset(void) {
    // any write to the ST_CURRENT register clears it
    // after clearing it automatically gets reset without
    // triggering exception logic
    // see reference manual section 3.2.1
    HWREG(NVIC_ST_CURRENT) = 1;

    // clear the global count variable
    systick_cnt = 0;
}

/**
 * SysTick Interrupt Handler
 *
 * Keep track of whether the systick counter wrapped
 */
static void SysTickHandler(void) {
    // increment every time the systick handler fires
    systick_cnt++;
}

/**
 * Initializes SysTick Module
 */
static void SysTickInit(void) {

    // configure the reset value for the systick countdown register
    MAP_SysTickPeriodSet(SYSTICK_RELOAD_VAL);

    // register interrupts on the systick module
    MAP_SysTickIntRegister(SysTickHandler);

    // enable interrupts on systick
    // (trigger SysTickHandler when countdown reaches 0)
    MAP_SysTickIntEnable();

    // enable the systick module itself
    MAP_SysTickEnable();
}
//*****************************************************************************
//
//! Display a prompt for the user to enter command
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void DisplayPrompt()
{
    UART_PRINT("\n\rcmd#");
}

//*****************************************************************************
//
//! Display the usage of the I2C commands supported
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
void DisplayUsage()
{
    UART_PRINT("Command Usage \n\r");
    UART_PRINT("------------- \n\r");
    UART_PRINT("write <dev_addr> <wrlen> <<byte0> [<byte1> ... ]> <stop>\n\r");
    UART_PRINT("\t - Write data to the specified i2c device\n\r");
    UART_PRINT("read  <dev_addr> <rdlen> \n\r\t - Read data frpm the specified "
               "i2c device\n\r");
    UART_PRINT("writereg <dev_addr> <reg_offset> <wrlen> <<byte0> [<byte1> ... "
               "]> \n\r");
    UART_PRINT("\t - Write data to the specified register of the i2c device\n\r");
    UART_PRINT("readreg <dev_addr> <reg_offset> <rdlen> \n\r");
    UART_PRINT("\t - Read data from the specified register of the i2c device\n\r");
    UART_PRINT("\n\r");
    UART_PRINT("Parameters \n\r");
    UART_PRINT("---------- \n\r");
    UART_PRINT("dev_addr - slave address of the i2c device, a hex value "
               "preceeded by '0x'\n\r");
    UART_PRINT("reg_offset - register address in the i2c device, a hex value "
               "preceeded by '0x'\n\r");
    UART_PRINT("wrlen - number of bytes to be written, a decimal value \n\r");
    UART_PRINT("rdlen - number of bytes to be read, a decimal value \n\r");
    UART_PRINT("bytex - value of the data to be written, a hex value preceeded "
               "by '0x'\n\r");
    UART_PRINT("stop - number of stop bits, 0 or 1\n\r");
    UART_PRINT("--------------------------------------------------------------"
               "--------------- \n\r\n\r");
}

//*****************************************************************************
//
//! Display the buffer contents over I2C
//!
//! \param  pucDataBuf is the pointer to the data store to be displayed
//! \param  ucLen is the length of the data to be displayed
//!
//! \return none
//!
//*****************************************************************************
void DisplayBuffer(unsigned char *pucDataBuf, unsigned char ucLen)
{
    unsigned char ucBufIndx = 0;
    UART_PRINT("Read contents");
    UART_PRINT("\n\r");
    while (ucBufIndx < ucLen)
    {
        UART_PRINT(" 0x%x, ", pucDataBuf[ucBufIndx]);
        ucBufIndx++;
        if ((ucBufIndx % 8) == 0)
        {
            UART_PRINT("\n\r");
        }
    }
    UART_PRINT("\n\r");
}

//*****************************************************************************
//
//! Application startup display on UART
//!
//! \param  none
//!
//! \return none
//!
//*****************************************************************************
static void
DisplayBanner(char *AppName)
{

    Report("\n\n\n\r");
    Report("\t\t *************************************************\n\r");
    Report("\t\t      CC3200 %s Application       \n\r", AppName);
    Report("\t\t *************************************************\n\r");
    Report("\n\n\n\r");
}

//*****************************************************************************
//
//! Board Initialization & Configuration
//!
//! \param  None
//!
//! \return None
//
//*****************************************************************************
static void
BoardInit(void)
{
/* In case of TI-RTOS vector table is initialize by OS itself */
#ifndef USE_TIRTOS
    //
    // Set vector table base
    //
#if defined(ccs)
    MAP_IntVTableBaseSet((unsigned long)&g_pfnVectors[0]);
#endif
#if defined(ewarm)
    MAP_IntVTableBaseSet((unsigned long)&__vector_table);
#endif
#endif
    //
    // Enable Processor
    //
    MAP_IntMasterEnable();
    MAP_IntEnable(FAULT_SYSTICK);

    PRCMCC3200MCUInit();
}

void MasterMain()
{
    int SPI_IF_BIT_RATE = 100000;

    MAP_SPIReset(GSPI_BASE);
    MAP_SPIConfigSetExpClk(GSPI_BASE, MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                           SPI_IF_BIT_RATE, SPI_MODE_MASTER, SPI_SUB_MODE_0,
                           (SPI_SW_CTRL_CS |
                            SPI_4PIN_MODE |
                            SPI_TURBO_OFF |
                            SPI_CS_ACTIVEHIGH |
                            SPI_WL_8));
    MAP_SPIEnable(GSPI_BASE);
    Adafruit_Init();
}

//*****************************************************************************
//
//! Main function handling the I2C example
//!
//! \param  None
//!
//! \return None
//!
//*****************************************************************************
void main()
{
    //
    // Initialize board configurations
    //
    BoardInit();

    //
    // Configure the pinmux settings for the peripherals exercised
    //
    PinMuxConfig();

    // Enable SysTick
    SysTickInit();

    //
    // Configuring UART
    //
    InitTerm();

    // Clear UART Terminal
    ClearTerm();

    Message("\t\t****************************************************\n\r");
    Message("\t\t\tSystick Example\n\r");
    Message("\t\t ****************************************************\n\r");
    Message("\n\n\n\r");

    //
    // I2C Init
    //
    // I2C_IF_Open(I2C_MASTER_MODE_FST);

    // MAP_PRCMPeripheralClkEnable(PRCM_GSPI,PRCM_RUN_MODE_CLK);
    // MAP_SPIReset(GSPI_BASE);
    // MasterMain();

    //
    // Display the banner followed by the usage description
    //
    // DisplayBanner(APP_NAME);
    // DisplayUsage();

    while (1)
    {
        // reset the countdown register
        SysTickReset();

        // wait for a fixed number of cycles
        // should be 3000 i think (see utils.c)
        UtilsDelay(1000);

        // read the countdown register and compute elapsed cycles
        unsigned long valu = SysTickValueGet();
        uint64_t delta = SYSTICK_RELOAD_VAL - valu;

        // convert elapsed cycles to microseconds
        uint64_t delta_us = TICKS_TO_US(delta);

        // print measured time to UART
        Report("cycles = %d\tus = %d\n\r", delta, delta_us);
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @
//
//*****************************************************************************
