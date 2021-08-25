/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#if 0

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "board.h"

#include "pin_mux.h"
#include "clock_config.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/


/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */


int test(void);
void mem_dump(void * addr, uint32_t len);
int a[1024];
 
int main(void)
{
    char ch;

    /* Init board hardware. */
    BOARD_ConfigMPU();
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();

    PRINTF("hello world 002 abc, good start. %d\r\n", test());
    PRINTF("%s %s \r\n",__DATE__, __TIME__);


    for(int i=0; i<1024; i++)
    {
        a[i] = i;
    }
    
    mem_dump(a, 1024*4);
    mem_dump(a, 16);
    mem_dump(a, 12);
    mem_dump(a, 8);
    mem_dump(a, 4);
    while (1)
    {
        ch = GETCHAR();
        PUTCHAR(ch);
    }
}
#else
    #include "fsl_debug_console.h"
#endif

#define mem_dump_printf PRINTF
/*
*   addr: memory address to begin dump
*   len:  in bytes.
*/
void mem_dump_32(void * addr, uint32_t len)
{
    uint32_t * p;
    uint32_t i;
    p = (uint32_t*)addr;
    i = 0;
    
    while(len >= 16)
    {
        mem_dump_printf("0x%08x:  %08x  %08x  %08x  %08x\r\n", &p[i], p[i], p[i+1], p[i+2], p[i+3]);
        i   += 4;
        len -= 16;        
    }
    
    if(len == 12)     mem_dump_printf("0x%x:  %08x  %08x  %08x  --------\r\n",         &p[i], p[i], p[i+1], p[i+2]);
    else if(len == 8) mem_dump_printf("0x%x:  %08x  %08x  --------  --------\r\n",     &p[i], p[i], p[i+1]);
    else if(len == 4) mem_dump_printf("0x%x:  %08x  --------  --------  --------\r\n", &p[i], p[i]);
}

void mem_dump_8(void * addr, uint32_t len) 
{
    uint8_t * p;
    uint32_t i;
    uint32_t left;
    p = (uint8_t*)addr;
    i = 0;
    
    while(len >= 8)
    {
        mem_dump_printf("0x%08x: %02x %02x %02x %02x %02x %02x %02x %02x \r\n", 
                        &p[i], p[i], p[i+1], p[i+2], p[i+3], p[i+4], p[i+5], p[i+6], p[i+7]);
        i   += 8;
        len -= 8;
    }
    
    if(len)
    {
        mem_dump_printf("0x%08x: ");
        left = 8;
        
        while(len)
        {
            mem_dump_printf("%02x ", p[i]);
            len--;
            i++;
            left--;
        }
        
        while(left)
        {
            mem_dump_printf("-- ");
            i++;
            left--;
        }
        
        mem_dump_printf("\r\n");
    }
}






