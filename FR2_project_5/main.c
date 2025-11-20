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
#define BTN_ENA_DELAY 50
#define PB1_ENA_DELAY 5
/******************************************************************************
* Constants
******************************************************************************/


/******************************************************************************
* Global Variables
******************************************************************************/
uint16_t timer_cnt=0;
uint16_t cnt=0;

uint8_t timer_task_10ms = FALSE, timer_task_50ms = FALSE, timer_task_150ms = FALSE, timer_task_450ms = FALSE, timer_task_1s = FALSE;

uint8_t PB0_pushed = FALSE, PB0_re_enable_cnt = BTN_ENA_DELAY;
uint8_t PB1_pushed = FALSE, PB1_re_enable_cnt = PB1_ENA_DELAY;
uint8_t PB2_pushed = FALSE, PB2_re_enable_cnt = BTN_ENA_DELAY;

uint8_t enable_cnt = 0;
uint8_t ciklus = 0;
int8_t pos1 = 3;
uint8_t pos2 = 4;
uint8_t jobb_ind = FALSE;
uint8_t bal_ind = FALSE;
uint8_t vesz_toggle = FALSE;

/******************************************************************************
* External Variables
******************************************************************************/


/******************************************************************************
* Local Function Declarations
******************************************************************************/
void timer_init(void);
void port_init(void);
void jobb_index(void);
void bal_index(void);
void veszvillogo(void);

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

void jobb_index(void)
{	
	
	if (pos1 >= 0 ) PORTA = PORTA | (1<<pos1);
	if (pos1 == -1) PORTA = PORTA & 0xf0;
	
	if (pos1 >= 0 ) PORTD = PORTD | (1<<pos1);
	if (pos1 == -1) PORTD = PORTD & 0xf0;
	
	if (pos1 == -2) pos1 = 4;
	pos1--;
	
}

void bal_index(void)
{
	if (pos2 <= 7 ) PORTA = PORTA | (1<<pos2);
	if (pos2 == 8) PORTA = PORTA & 0x0f;
	
	if (pos2 <= 7 ) PORTD = PORTD | (1<<pos2);
	if (pos2 == 8) PORTD = PORTD & 0x0f;
	
	if (pos2 == 9) pos2 = 3;
	pos2++;
}

void veszvillogo(void)
{
	bal_index();
	jobb_index();
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
	
	while(1)
	{
		if(timer_task_10ms)
		{
		
			timer_task_10ms=FALSE;
		}
		
		if(timer_task_50ms)
		{

			//PB0	***********************************************************************************************************	
			if((PINB & (1<<PB0)) == 0 && PB0_pushed == FALSE && PB0_re_enable_cnt==BTN_ENA_DELAY)
			{
						
				PB0_pushed = TRUE;
				PB0_re_enable_cnt = 0;
			}
			if((PINB & (1<<PB0)) == (1<<PB0) && PB0_pushed == TRUE && PB0_re_enable_cnt==BTN_ENA_DELAY)
			{
				PB0_pushed=FALSE;
				PORTA = 0;
				PORTD = 0;
				pos1 = 3;
				pos2 = 4;
				
				PB0_re_enable_cnt = 40;
			}
			//*****************************************************************************************************************							
					
			// PB1
			if((PINB & (1<<PB1)) == 0 && PB1_pushed == FALSE && PB1_re_enable_cnt == PB1_ENA_DELAY)
			{
				if( vesz_toggle == FALSE )
				{
					vesz_toggle = TRUE;
				}
				
				else
				{
					vesz_toggle = FALSE;
				}
				
				PORTA = 0;
				PORTD = 0;
				pos1 = 3;
				pos2 = 4;
				PB1_re_enable_cnt = 0;
				PB1_pushed = TRUE;
				
			}
				
			if((PINB & (1<<PB1)) == (1<<PB1) && PB1_pushed == TRUE && PB1_re_enable_cnt == PB1_ENA_DELAY)
			{
				PORTA = 0;
				PORTD = 0;
				pos1 = 3;
				pos2 = 4;
				PB1_re_enable_cnt = 0;
				PB1_pushed=FALSE;
			}
			
			//*****************************************************************************************************************		
			// PB2
			if((PINB & (1<<PB2)) == 0 && PB2_pushed == FALSE && PB2_re_enable_cnt==BTN_ENA_DELAY)
			{
				
				PB2_pushed = TRUE;
				PB2_re_enable_cnt = 0;
			}
			if((PINB & (1<<PB2)) == (1<<PB2) && PB2_pushed == TRUE && PB2_re_enable_cnt==BTN_ENA_DELAY)
			{
				PORTA = 0;
				PORTD = 0;
				pos1 = 3;
				pos2 = 4;
				
				PB2_pushed=FALSE;
				PB2_re_enable_cnt = 40;
			}
			//*****************************************************************************************************************		
			
			//gomb tiltasok
			if(PB0_re_enable_cnt<BTN_ENA_DELAY) PB0_re_enable_cnt += 1;
			if(PB1_re_enable_cnt<PB1_ENA_DELAY) PB1_re_enable_cnt += 1;
			if(PB2_re_enable_cnt<BTN_ENA_DELAY) PB2_re_enable_cnt += 1;
			
			timer_task_50ms=FALSE;
			}
		
	//*****************************************************************************************************************		
			
			//150 ms timer	
		if(timer_task_150ms)
		{
			if (PB0_pushed && !vesz_toggle && PB2_re_enable_cnt == BTN_ENA_DELAY) jobb_index();
			
			if (vesz_toggle) veszvillogo();
			
			if(PB2_pushed && !vesz_toggle && PB0_re_enable_cnt == BTN_ENA_DELAY) bal_index();
			
			
			timer_task_150ms=FALSE;
		}
//*****************************************************************************************************************		
			
			//450 ms timer			
		if(timer_task_450ms)
		{
			if(PB0_pushed || PB1_pushed || PB2_pushed)
			{
				ciklus++;
			}
			
			if(ciklus == 1) ciklus = 0;
			timer_task_450ms=FALSE;
		}
		
//*****************************************************************************************************************		
			
			//1000 ms timer	
		if(timer_task_1s)
		{
			if(PB0_pushed || PB1_pushed || PB2_pushed)
			{
				ciklus++;
			}
			
			if(ciklus == 1) ciklus = 0;
			
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
 	if((timer_cnt % 15) == 0) timer_task_150ms = TRUE;
 	if((timer_cnt % 45) == 0) timer_task_450ms = TRUE;
	if((timer_cnt % 100) == 0) timer_task_1s = TRUE;
}





