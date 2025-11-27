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
#include "can.h"
#include <math.h>
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
int8_t pos1 = 3;
uint8_t pos2 = 4;
uint8_t jobb_ind = FALSE;
uint8_t bal_ind = FALSE;
uint8_t vesz_toggle = FALSE;
uint8_t auto_det_crash = FALSE;
uint8_t cnt_4bit = 0;

float kormanyszog[5];
uint8_t kanyarodas = FALSE;
uint8_t kanyar_count = 0;

float sebesseg_x[5];
float sebesseg_y[5];
float sebesseg_abs[5];
float min_lassulas = 7;
float max_lassulas = 10;

uint8_t can_rx_data[8];
uint32_t can_rx_id = 0x00000011;
uint8_t can_rx_extended_id = FALSE;
uint8_t can_rx_length;
uint8_t can_msg_received=FALSE;

uint8_t can_tx_data[8];


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
void CAN_beolvasas(void);
void CAN_kuldes(void);
void blinker_reset(void);
void auto_blinker_off(void);
void detect_crash(void);

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
	if(auto_det_crash) auto_det_crash = FALSE;
}

void CAN_beolvasas(void)
{
	if(can_msg_received)
	{
		sprintf(string_for_write_can, "ID: 0x%lX, Length: %d, DATA:", can_rx_id, can_rx_length);
		
		for(uint8_t i=0; i<can_rx_length;i++)
		{
			sprintf(string_for_write_can, " 0x%X", can_rx_data[i]);
			
		}
		
		can_msg_received=0;
	}
	
	kormanyszog[0] = 0; //kormanyszog beolvasas
	for(int i = 4; i > 0;i--)
	{
		sebesseg_x[i] = sebesseg_x[i-1];
		sebesseg_y[i] = sebesseg_y[i-1];
	}
	
	sebesseg_x[0] = 0; //sebessegbeolvasasok
	sebesseg_y[0] = 0;
}

void CAN_kuldes(void)
{
	uint8_t can_tx_data[8];
	if(cnt_4bit == 15) cnt_4bit = 0;
	else cnt_4bit++;
	
	can_tx_data[0]= cnt_4bit<<4 | auto_det_crash<<3 | vesz_toggle<<2 |PB0_pushed<<1 | PB2_pushed;
	
	CAN_SendMob(0,0x000001FD,FALSE,1,can_tx_data);
}

void blinker_reset(void)
{
	PORTA = 0;
	PORTD = 0;
	pos1 = 3;
	pos2 = 4;
}

void auto_blinker_off(void)
{
	for (int i=0;i<5;i++)
	{
		if(sqrt((kormanyszog[i])^2) > 20 && kormanyszog[i-1] > 20 && !kanyarodas) kanyar_count++;
		
		if (kanyar_count>3)
		{
			kanyarodas = TRUE; // ennek lehet, hogy így nincs értelme, csak azt akarom, hogy csak abban az esetben tekintse kanyarodásnak, ha több ideig el van tekerve a kormány
			kanyar_count = 0;
		}
		
		if (kormanyszog[i] == 0 && kanyarodas)
		{
			kanyarodas = FALSE;
			blinker_reset();
			kanyar_count = 0;
		}
	}
	
}

void detect_crash(void)
{
	for(int i = 0;i < 5; i++)
	{
		sebesseg_abs[i]=hypotf(sebesseg_x[i],sebesseg_y[i]);
		
		if( (sebesseg_abs[i+1]-sebesseg_abs[i]) > min_lassulas && (sebesseg_abs[i+1]-sebesseg_abs[i]) < max_lassulas)
		{
			vesz_toggle = TRUE;
			auto_det_crash = TRUE;
		}
	}
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
	can_init();
	CAN_ReceiveEnableMob(0, can_rx_id, can_rx_extended_id, 8);	// enable reception on mob 0
	sei();
	
	while(1)
	{
		//*****************************************************************************************************************
		//10 ms timer
		
		if(timer_task_10ms)
		{
		
			timer_task_10ms=FALSE;
		}
		//*****************************************************************************************************************
		//50 ms timer
		
		if(timer_task_50ms)
		{
			//************************************************************************************************************
			//Automatikus függvények 
			
			CAN_beolvasas();
			CAN_kuldes();
			detect_crash();
			auto_blinker_off();
			
			
			
			//*****************************************************************************************************************
			//PB0		
			
			if((PINB & (1<<PB0)) == 0 && PB0_pushed == FALSE && PB0_re_enable_cnt==BTN_ENA_DELAY)
			{
						
				PB0_pushed = TRUE;
				PB0_re_enable_cnt = 0;
			}
			if((PINB & (1<<PB0)) == (1<<PB0) && PB0_pushed == TRUE && PB0_re_enable_cnt==BTN_ENA_DELAY)
			{
				PB0_pushed=FALSE;
				blinker_reset();
				
				PB0_re_enable_cnt = 40;
			}
			//*****************************************************************************************************************							
			// PB1
			
			if((PINB & (1<<PB1)) == 0 && PB1_pushed == FALSE && PB1_re_enable_cnt == PB1_ENA_DELAY)
			{
				if( vesz_toggle == FALSE ) vesz_toggle = TRUE;
				else vesz_toggle = FALSE;
				
				blinker_reset(); 
				PB1_re_enable_cnt = 0;
				PB1_pushed = TRUE;
				
			}
				
			if((PINB & (1<<PB1)) == (1<<PB1) && PB1_pushed == TRUE && PB1_re_enable_cnt == PB1_ENA_DELAY)
			{
				blinker_reset();
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
				blinker_reset();
				
				PB2_pushed=FALSE;
				PB2_re_enable_cnt = 40;
			}
			//*****************************************************************************************************************		
			//Gomb tiltasok
			
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
			timer_task_450ms=FALSE;
		}
		
		//*****************************************************************************************************************		
		//1000 ms timer	
		
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
 	if((timer_cnt % 15) == 0) timer_task_150ms = TRUE;
 	if((timer_cnt % 45) == 0) timer_task_450ms = TRUE;
	if((timer_cnt % 100) == 0) timer_task_1s = TRUE;
}
ISR(CANIT_vect) //CAN megszakítás
{
	uint8_t i, dlc = 0;
	

	CANPAGE = 0;	// select MOb0, reset FIFO index

	if ( (CANSTMOB & (1<<RXOK)) != FALSE)	// Receive Complete
	{
		
		dlc = CANCDMOB & 0x0F;
		
		for (i=0; i<dlc; i++) can_rx_data[i] = CANMSG;
		
		CANSTMOB &= ~(1<<RXOK);	// clear RXOK flag
		CAN_ReceiveEnableMob(0, can_rx_id, can_rx_extended_id, 8);	// enable next reception  on mob 0
	}
	can_rx_length=dlc;
	can_msg_received=1;
	
}




