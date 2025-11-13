/******************************************************************************
 * Created: 2025.11.06.
 * Author : Borbiró Dávid, Ádám Barnabás, Maczkó Márk Soma
******************************************************************************/
 /******************************************************************************
* Include files
******************************************************************************/
#include <avr/io.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#define F_CPU 8000000UL
#include <avr/delay.h>
#include <stdio.h>
#include "peripherals.h"
/******************************************************************************
* Macros
******************************************************************************/
#define TRUE 1
#define FALSE 0
#define BTN_ENA_DELAY 60
/******************************************************************************
* Constants
******************************************************************************/


/******************************************************************************
* Global Variables
******************************************************************************/
uint16_t timer_cnt=0;
uint16_t cnt=0;

uint8_t timer_task_10ms = FALSE, timer_task_50ms = FALSE, timer_task_100ms = FALSE, timer_task_500ms = FALSE, timer_task_1s = FALSE;

uint8_t PB0_pushed = FALSE, PB0_re_enable_cnt = FALSE;
uint8_t PB1_pushed = FALSE, PB1_re_enable_cnt = FALSE;
uint8_t PB2_pushed = FALSE, PB2_re_enable_cnt = FALSE;

uint8_t enable_cnt = 0;
// uint16_t meghivas =0;

/******************************************************************************
* External Variables
******************************************************************************/


/******************************************************************************
* Local Function Declarations
******************************************************************************/
void timer_init(void);
void port_init(void);

/******************************************************************************
* Local Function Definitions
******************************************************************************/
void port_init(void)
{
      DDRA = 0xff;
      DDRD = 0xff;
      
	
	DDRB = (0<<PB0) | (0<<PB1) | (0 << PB2);
	PORTB = (1<<PB0) | (1<<PB1) | (1 << PB2);
}

void jobb_index()
{	
		for (int i=0;i<2;i++)
		{			
			if ((cnt % 5) ==0)
				{
					for (int j=0;j<3;j++)
					{
						if (j==0) PORTA = 8;
						if (j==1) PORTA = 4;
						if (j==2) PORTA = 2;
						if (j==3) PORTA = 1;						
					}
				}
				
		}		
}

void bal_index()
{
	
}
/******************************************************************************
* Function:         int main(void)
* Description:      main function
* Input:            
* Output:           
* Notes:            
******************************************************************************/
int main(void)
{
	port_init();
	timer_init();
	sei();
	/* Replace with your application code */
	while(1)
	{
		if(timer_task_10ms)
		{
		
			timer_task_10ms=FALSE;
		}
		
		if(timer_task_50ms)
		{

				//PB0
			if (PB0_pushed)
			{
			cnt++;					
			jobb_index();
					
					
			if((PINB & (1<<PB0)) == 0 && PB0_pushed == FALSE && PB0_re_enable_cnt==BTN_ENA_DELAY)
			{
						
				PB0_pushed = TRUE;
				PB0_re_enable_cnt = 0;
			}
			if((PINB & (1<<PB0)) == (1<<PB0) && PB0_pushed == TRUE && PB0_re_enable_cnt==BTN_ENA_DELAY)
			{
				PB0_pushed=FALSE;
				PB0_re_enable_cnt = 0;
			}
										
					
					
				// PB1
				if((PINB & (1<<PB1)) == 0 && PB1_pushed == FALSE)
				{
					
					PB1_pushed = TRUE;
				}
				
				if((PINB & (1<<PB1)) == (1<<PB1) && PB1_pushed == TRUE) PB1_pushed=FALSE;
				// PB2
				if((PINB & (1<<PB2)) == 0 && PB2_pushed == FALSE)
				{
					bal_index();
					PB2_pushed = TRUE;
				}
				
				if((PINB & (1<<PB2)) == (1<<PB2) && PB2_pushed == TRUE) PB2_pushed=FALSE;
			
			timer_task_50ms=FALSE;
		}
		
		if(timer_task_100ms)
		{
			timer_task_100ms=FALSE;
		}
		
		if(timer_task_500ms)
		{
			
			timer_task_500ms=FALSE;
		}
		if(timer_task_1s)
		{
			
			timer_task_1s=FALSE;
		}
	}
	
}


/******************************************************************************
* Interrupt Routines
******************************************************************************/
ISR(TIMER0_COMP_vect)
{
	timer_cnt++;
	if((timer_cnt % 1) == 0) timer_task_10ms = TRUE;
	if((timer_cnt % 5) == 0) timer_task_50ms = TRUE;
 	if((timer_cnt % 10) == 0) timer_task_100ms = TRUE;
 	if((timer_cnt % 50) == 0) timer_task_500ms = TRUE;
	if((timer_cnt % 100) == 0) timer_task_1s = TRUE;
}





