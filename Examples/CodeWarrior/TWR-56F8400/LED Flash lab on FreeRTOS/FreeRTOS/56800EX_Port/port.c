/*
    FreeRTOS V7.3.0 - Copyright (C) 2012 Real Time Engineers Ltd.

    FEATURES AND PORTS ARE ADDED TO FREERTOS ALL THE TIME.  PLEASE VISIT 
    http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!
    
    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    
    http://www.FreeRTOS.org - Documentation, training, latest versions, license 
    and contact details.  
    
    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool.

    Real Time Engineers ltd license FreeRTOS to High Integrity Systems, who sell 
    the code with commercial support, indemnification, and middleware, under 
    the OpenRTOS brand: http://www.OpenRTOS.com.  High Integrity Systems also
    provide a safety engineered and independently SIL3 certified version under 
    the SafeRTOS brand: http://www.SafeRTOS.com.
*/

/*
	Changes from V4.2.1

	+ Introduced the configKERNEL_INTERRUPT_PRIORITY definition.
*/

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the 56800EX port.
 * Code ported by Richy Ye in Jan. 2013.
 *----------------------------------------------------------*/

/* Port include files. */
#include "PE_Types.h"
#include "IO_Map.h"

/* Scheduler include files. */
#include "FreeRTOS.h"
#include "task.h"

/* Hardware specifics. */
#define portTIMER_PRESCALE 16
#define portINITIAL_SR	0x00UL

/* Defined for backward compatability with project created prior to 
FreeRTOS.org V4.3.0. */
#ifndef configKERNEL_INTERRUPT_PRIORITY
	#define configKERNEL_INTERRUPT_PRIORITY 0
#endif


/* We require the address of the pxCurrentTCB variable, but don't want to know
any details of its type. */
typedef void tskTCB;
extern tskTCB * volatile pxCurrentTCB;

/* Records the nesting depth of calls to portENTER_CRITICAL(). */
unsigned portBASE_TYPE uxCriticalNesting = 0x00;

/*
 * Setup the timer used to generate the tick interrupt.
 */
static void prvSetupTimerInterrupt( void );

void vPortYield(void);

#define portSAVE_CONTEXT()	do {\
		asm(ADDA	#<2,SP);	/* SP is always long aligned */\
		asm(BFTSTL  #$0001,SP);\
		asm(BCC     NOADDASAVE);\
		asm(ADDA    #<1,SP);\
		asm(NOADDASAVE:);\
		asm(MOVE.L	SR, X:(SP)+);\
		asm(BFSET  	#0x0300,SR);	/* Disable interrupt */\
		asm(MOVE.L  R0, X:(SP)+);\
		asm(MOVE.L  R1, X:(SP)+);\
		asm(MOVE.L  R2, X:(SP)+);\
		asm(MOVE.L  R3, X:(SP)+);\
		asm(MOVE.L  R4, X:(SP)+);\
		asm(MOVE.L  R5, X:(SP)+);\
		asm(MOVE.L  N, X:(SP)+);\
		asm(MOVE.L  N3, X:(SP)+);\
		asm(MOVE.L  LC2,X:(SP)+);\
		asm(MOVE.L  LA2,X:(SP)+);\
		asm(MOVE.L  LC,X:(SP)+);\
		asm(MOVE.L  LA,X:(SP)+);\
		asm(MOVE.L  A2,X:(SP)+);\
		asm(MOVE.L  A10,X:(SP)+);\
		asm(MOVE.L  B2,X:(SP)+);\
		asm(MOVE.L  B10,X:(SP)+);\
		asm(MOVE.L  C2,X:(SP)+);\
		asm(MOVE.L  C10,X:(SP)+);\
		asm(MOVE.L  D2,X:(SP)+);\
		asm(MOVE.L  D10,X:(SP)+);\
		asm(MOVE.L  X0,X:(SP)+);\
		asm(MOVE.L  Y,X:(SP)+);\
		asm(MOVE.L  OMR,X:(SP)+);\
		asm(BFCLR	#0x01B0,OMR);	/* Ensure CM = 0 */\
		asm(MOVE.L  M01,X:(SP)+);\
		asm(BFSET	#0xFFFF,M01);	/* Set M01 for linear addressing */\
		asm(MOVE.L  HWS,X:(SP)+);\
		asm(MOVE.L  HWS,X:(SP)+);\
		asm(MOVE.L	uxCriticalNesting,A);\
		asm(MOVE.L	A10,X:(SP)+);	/* Save the critical nesting counter */\
		asm(MOVE.L	pxCurrentTCB,R0);\
		asm(TFRA	SP,R1);	/* Save the new top of stack into the TCB */\
		asm(MOVE.L	R1,X:(R0));\
		}while(0)

#define portRESTORE_CONTEXT()  do {\
		asm(MOVE.L	pxCurrentTCB,R0);	/* Restore the stack pointer for the task */\
		asm(MOVE.L	X:(R0),R1);\
		asm(BFTSTL  #$0001,R1);\
		asm(BCC     NOADDARESTORE);\
		asm(ADDA    #<1,R1);\
		asm(NOADDARESTORE:);\
		asm(TFRA	R1,SP);\
		asm(SUBA	#<2,SP);\
		asm(MOVE.L	X:(SP)-,A);\
		asm(MOVE.L	A10,uxCriticalNesting);	/* Restore the critical nesting counter */\
		asm(MOVE.L  X:(SP)-,HWS);\
		asm(MOVE.L  X:(SP)-,HWS);\
		asm(MOVE.L  X:(SP)-,M01);\
		asm(MOVE.L  X:(SP)-,OMR);\
		asm(MOVE.L  X:(SP)-,Y);\
		asm(MOVE.L  X:(SP)-,X0);\
		asm(MOVE.L  X:(SP)-,D);\
		asm(MOVE.L  X:(SP)-,D2);\
		asm(MOVE.L  X:(SP)-,C);\
		asm(MOVE.L  X:(SP)-,C2);\
		asm(MOVE.L  X:(SP)-,B);\
		asm(MOVE.L  X:(SP)-,B2);\
		asm(MOVE.L  X:(SP)-,A);\
		asm(MOVE.L  X:(SP)-,A2);\
		asm(MOVE.L  X:(SP)-,LA);\
		asm(MOVE.L  X:(SP)-,LC);\
		asm(MOVE.L  X:(SP)-,LA2);\
		asm(MOVE.L  X:(SP)-,LC2);\
		asm(MOVE.L  X:(SP)-,N3);\
		asm(MOVE.L  X:(SP)-,N);\
		asm(MOVE.L  X:(SP)-,R5);\
		asm(MOVE.L  X:(SP)-,R4);\
		asm(MOVE.L  X:(SP)-,R3);\
		asm(MOVE.L  X:(SP)-,R2);\
		asm(MOVE.L  X:(SP)-,R1);\
		asm(MOVE.L  X:(SP)-,R0);\
		asm(MOVE.L  X:(SP)-,SR);\
		}while(0)

/* 
 * See header file for description. 
 */
portSTACK_TYPE *pxPortInitialiseStack( portSTACK_TYPE *pxTopOfStack, pdTASK_CODE pxCode, void *pvParameters )
{
unsigned short usCode;
portBASE_TYPE i;

const portSTACK_TYPE xInitialStack[] = 
{
	0x03030303UL,	/* R3 */
	0x04040404UL, /* R4 */
	0x05050505UL, /* R5 */
	0x06060606UL, /* N */
	0x07070707UL, /* N3 */
	0x08080808UL, /* LC2 */
	0x09090909UL, /* LA2 */
	0x0A0A0A0AUL, /* LC */
	0x0B0B0B0BUL, /* LA */
	0x0E0E0E0EUL, /* A2 */
	0x0F0F0F0FUL, /* A10 */
	0x11111111UL, /* B2 */
	0x22222222UL, /* B10 */
	0x33333333UL, /* C2 */
	0x44444444UL, /* C10 */
	0x55555555UL, /* D2 */
	0x66666666UL, /* D10 */
	0x77777777UL, /* X0 */
	0x88888888UL /* Y */
};

	/* Setup the stack as if a yield had occurred.
	Save the program counter. */
	*pxTopOfStack = ( portSTACK_TYPE ) pxCode;
	pxTopOfStack++;
	

	/* Status register with interrupts enabled. */
	*pxTopOfStack = portINITIAL_SR;
	pxTopOfStack++;
	
	
	/* Address register R0 */
	*pxTopOfStack = 0x0000000UL;
	pxTopOfStack++;
	
	/* Address register R1 */
	*pxTopOfStack = 0x01010101UL;
	pxTopOfStack++;

	/* Parameters are passed in R2. */
	*pxTopOfStack = ( portSTACK_TYPE ) pvParameters;
	pxTopOfStack++;

	for( i = 0; i < ( sizeof( xInitialStack ) / sizeof( portSTACK_TYPE ) ); i++ )
	{
		*pxTopOfStack = xInitialStack[ i ];
		pxTopOfStack++;
	}

	/* Operation mode register */
	asm(MOVE.W OMR,usCode);
	*pxTopOfStack = ( portSTACK_TYPE ) usCode;
	pxTopOfStack++;
	
	/* Modulo register */
	asm(MOVE.W M01,usCode);
	*pxTopOfStack = ( portSTACK_TYPE ) usCode;
	pxTopOfStack++;
	
	/* Hardware stack register 1 */
	*pxTopOfStack = 0x0C0C0C0CUL;
	pxTopOfStack++;
	
	/* Hardware stack register 0 */
	*pxTopOfStack = 0x0D0D0D0DUL;
	pxTopOfStack++;

	/* Finally the critical nesting depth. */
	*pxTopOfStack = 0x00000000UL;
	pxTopOfStack++;

	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

portBASE_TYPE xPortStartScheduler( void )
{
	/* Setup a timer for the tick ISR. */
	prvSetupTimerInterrupt(); 

	/* Restore the context of the first task to run. */
	portRESTORE_CONTEXT();
	
	/* Simulate the end of the yield function. */
	asm (rts);

	/* Should not reach here. */
	return pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the scheduler for the 56800EX port will get stopped
	once running.  If required disable the tick interrupt here, then return 
	to xPortStartScheduler(). */
}
/*-----------------------------------------------------------*/

/*
 * Setup a timer for a regular tick.
 */
static void prvSetupTimerInterrupt( void )
{
const unsigned long ulCompareMatch = ( ( configCPU_CLOCK_HZ / portTIMER_PRESCALE ) / configTICK_RATE_HZ ) - 1;
unsigned int uiPrescalerVal;
unsigned int uiPortTimerPrescale;

	/* Enable PIT0 clock */
	setReg16Bit(SIM_PCE2,PIT0);
	
	/* Setup PIT0 interrupt priority */
	setRegBitGroup(INTC_IPR10,PIT0_ROLLOVR,configKERNEL_INTERRUPT_PRIORITY+1);
	
	/* Reset PIT0 */
	setReg16(PIT0_CTRL,0x00);
	
	/* Enable the interrupt */
	setReg16Bit(PIT0_CTRL,PRIE);
	
	/* Configure the modulo value */
	setReg16(PIT0_MOD,ulCompareMatch);
	
	/* Setup the prescaler value */
	uiPortTimerPrescale = portTIMER_PRESCALE;
	for (uiPrescalerVal=0; uiPortTimerPrescale>0; uiPrescalerVal++)
	{
			uiPortTimerPrescale = uiPortTimerPrescale/2;
	}
	setRegBitGroup(PIT0_CTRL,PRESCALER,uiPrescalerVal-1);
	
	/* Start the timer */
	setRegBit(PIT0_CTRL,CNT_EN);
}
/*-----------------------------------------------------------*/

void vPortEnterCritical( void )
{
	portDISABLE_INTERRUPTS();
	asm(nop);
	asm(nop);
	asm(nop);
	asm(nop);
	asm(nop);
	asm(nop);
	uxCriticalNesting++;
}
/*-----------------------------------------------------------*/

void vPortExitCritical( void )
{
	uxCriticalNesting--;
	if( uxCriticalNesting == 0 )
	{
		portENABLE_INTERRUPTS();
		asm(nop);
		asm(nop);
		asm(nop);
		asm(nop);
		asm(nop);
		asm(nop);
	}
}
/*-----------------------------------------------------------*/

/*
 * Context switch functions.  These are both interrupt service routines.
 */
void vPortYield(void)
{
	portSAVE_CONTEXT();
	vTaskSwitchContext();
	portRESTORE_CONTEXT();
}

/*-----------------------------------------------------------*/

#pragma interrupt alignsp
void vPortTickInterrupt(void)
{
	/* Clear the timer interrupt. */
	clrRegBit(PIT0_CTRL,PRF);
	
	vTaskIncrementTick();

	#if configUSE_PREEMPTION == 1
		portYIELD();
	#endif
}
