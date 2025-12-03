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
uint8_t pos1 = 3;
uint8_t pos2 = 4;
uint8_t right_turn = FALSE;
uint8_t left_turn = FALSE;
uint8_t warning_toggle = FALSE;
uint8_t auto_det_crash = FALSE;
uint8_t cnt_4bit = 0;
// uint8_t cnt_3bit = 0;
// uint8_t cnt_2bit = 0;

float stangle[5];
float stangle_limit = 20.0;
float stangle_can_factor = 0.04;
uint8_t is_turning = FALSE;
uint8_t consecutive_count = 0;

float vel_x[5];
float vel_y[5];
float vel_abs[5];
float min_accel = 7;
float max_accel = 10;
float velx_can_factor = 0.1;
float vely_can_factor = 0.05;


uint8_t can_rx_data[8];
uint32_t can_rx_id;
uint32_t can_rx_id_stangle = 0x00010001; // 0x11
uint32_t can_rx_id_vel = 0x00100010; // 0x22

uint8_t can_rx_extended_id_stangle = FALSE;
uint8_t can_rx_extended_id_velx = FALSE;
uint8_t can_rx_extended_id_vely = FALSE;

uint8_t can_rx_length;
uint8_t can_rx_length_stangle = 16;
uint8_t can_rx_length_velx = 12;
uint8_t can_rx_length_vely = 10;

uint8_t can_msg_received=FALSE;
uint8_t stangle_received=FALSE;
uint8_t vel_x_received=FALSE;
uint8_t vel_y_received=FALSE;

uint8_t can_tx_data[8];


/******************************************************************************
* External Variables
******************************************************************************/


/******************************************************************************
* Local Function Declarations
******************************************************************************/

void port_init(void);
void bal_index(void);
void timer_init(void);
void jobb_index(void);
void CAN_kuldes(void);
void veszvillogo(void);
void detect_crash(void);
void CAN_beolvasas(void);
void blinker_reset(void);
void auto_blinker_off(void);

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
		  switch(can_rx_id)
		  {
			  case 0x11: // kormányszög
			  {
				  for (int i=0;i<5;i++)
				  {
					  float raw_stangle = can_rx_data[1] << 8 | can_rx_data[0];
					  stangle[i] = raw_stangle * stangle_can_factor;
					  break;
				  }			 
			  }
			  case 0x22: // sebesség
			  {
				  for (int j=0;j<5;j++)
				  {
					   float raw_velx = can_rx_data[4] | can_rx_data[1] << 8 | ;
					   vel_x[j] = raw_velx * velx_can_factor;
					   
					   float raw_vely =  ;
					   vel_y[j] = raw_vely * vely_can_factor;
					   break;
				  }
			  }
		  }
		  can_msg_received = 0;
	  }
}

void CAN_kuldes(void)
{
	uint8_t can_tx_data[8];
	if(cnt_4bit == 15) cnt_4bit = 0;
	else cnt_4bit++;
	
	can_tx_data[0]= cnt_4bit<<4 | auto_det_crash<<3 | warning_toggle<<2 |PB0_pushed<<1 | PB2_pushed;
	
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
		if(fabsf(stangle[i]) > stangle_limit && !is_turning) consecutive_count++;
		
		if (consecutive_count>3)
		{
			is_turning = TRUE; 
			consecutive_count = 0;
		}
		
		if (stangle[i] == 0 && is_turning)
		{
			is_turning = FALSE;
			blinker_reset();
			consecutive_count = 0;
		}
	}
	
}

void detect_crash(void)
{
	for(int i = 0;i < 5; i++)
	{
		vel_abs[i]=hypotf(vel_x[i],vel_y[i]);
		
		if( (vel_abs[i+1]-vel_abs[i]) > min_accel && (vel_abs[i+1]-vel_abs[i]) < max_accel)
		{
			warning_toggle = TRUE;
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
	CAN_ReceiveEnableMob(0, can_rx_id_vel, can_rx_extended_id_velx, 8);	// sebesség olvasás 
	CAN_ReceiveEnableMob(0, can_rx_id_vel, can_rx_extended_id_vely, 8);	// sebesség olvasás
	CAN_ReceiveEnableMob(0, can_rx_id_stangle, can_rx_extended_id_stangle, 8);	// kormányszög olvasás	
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
				if( warning_toggle == FALSE ) warning_toggle = TRUE;
				else warning_toggle = FALSE;
				
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
			if (PB0_pushed && !warning_toggle && PB2_re_enable_cnt == BTN_ENA_DELAY) jobb_index();
			
			if (warning_toggle) veszvillogo();
			
			if(PB2_pushed && !warning_toggle && PB0_re_enable_cnt == BTN_ENA_DELAY) bal_index();
			
			
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
ISR(CANIT_vect)
{
	uint8_t i, dlc = 0;

	CANPAGE = 0;  // MOB kiválasztása

	if ((CANSTMOB & (1<<RXOK)) != FALSE)
	{
		dlc = CANCDMOB & 0x0F;

		switch(can_rx_length)
		{
			case can_rx_length_stangle:		// kormányszög olvasás
				for(i=0;i<dlc;i++) stangle[i] = CANMSG;
				can_rx_length_stangle = dlc;
				stangle_received = 1;
				break;

			case can_rx_length_velx:		// x sebesség olvasás			
				for(i=0;i<dlc;i++) vel_x[i] = CANMSG;
				can_rx_length_velx = dlc;
				vel_x_received = 1;
				break;

			case can_rx_length_vely:		// y sebesség olvasás
				for(i=0;i<dlc;i++) vel_y[i] = CANMSG;
				can_rx_length_vely = dlc;
				vel_y_received = 1;
				break;
		}

		CANSTMOB &= ~(1<<RXOK); // flag törlése

		CAN_ReceiveEnableMob(0, can_rx_id_stangle, can_rx_extended_id_stangle, 8); // Következõ fogadás engedélyezése
		CAN_ReceiveEnableMob(0, can_rx_id_vel, can_rx_extended_id_velx, 8); 
		CAN_ReceiveEnableMob(0, can_rx_id_vel, can_rx_extended_id_vely, 8); 
		
	}
}



