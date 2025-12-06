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
#include <util/delay.h>
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
//Perifériák változói
uint16_t timer_cnt=0;


uint8_t timer_task_10ms = FALSE, timer_task_50ms = FALSE, timer_task_150ms = FALSE, timer_task_450ms = FALSE, timer_task_1s = FALSE;
uint8_t PB0_pushed = FALSE, PB0_re_enable_cnt = BTN_ENA_DELAY;
uint8_t PB1_pushed = FALSE, PB1_re_enable_cnt = PB1_ENA_DELAY;
uint8_t PB2_pushed = FALSE, PB2_re_enable_cnt = BTN_ENA_DELAY;

//Index változók
uint8_t pos1 = 3;
uint8_t pos2 = 4;
uint8_t warning_toggle = FALSE;
uint8_t auto_det_crash = FALSE;

//Beolvasott nyers CAN adatok
uint16_t raw_st_angle = 0;
uint16_t raw_vel_x= 0;
uint16_t raw_vel_y= 0;
uint8_t cnt_2bit = 0; //0x22 üzenethez tartozó olvasott counter
uint8_t cnt_3bit = 0; //0x11 üzenethez tartozó olvasott counter
uint8_t cnt_2bit_local = 0; //0x22 üzenethez tartozó lokális counter
uint8_t cnt_3bit_local = 0; //0x11 üzenethez tartozó lokális counter

//Automatikus index visszaállításhoz használt változók
float st_angle = 0;					//deg
float st_angle_limit = 20.0;		//deg
float st_angle_can_factor = 0.04;
uint8_t is_turning = FALSE;
uint8_t turn_counter = 0;

//Gyorsulás számításhoz használt változók
float vel_x = 0;       // m/s
float vel_y = 0;       // m/s
float vel_abs[5];      // m/s
float min_accel = 7;   // m/s2
float max_accel = 10;  // m/s2 
float velx_can_factor = 0.1;
float vely_can_factor = 0.05;

//CAN változók
uint8_t cnt_4bit = 0;  //kiküldött timer counter
uint8_t can_rx_data[8];
uint8_t can_rx_data2[8];
const uint8_t can_rx_length_st_angle = 16;
const uint8_t can_rx_length_velx = 12;
const uint8_t can_rx_length_vely = 10;
uint8_t can_msg_received=FALSE;
uint8_t can_tx_data[1];
uint8_t can_tx_error[1];

/******************************************************************************
* External Variables
******************************************************************************/


/******************************************************************************
* Local Function Declarations
******************************************************************************/

void port_init(void);
void left_index(void);
void right_index(void);
void CAN_send(void);
void emergency_lights(void);
void detect_crash(void);
void CAN_read(void);
void CAN_sendError(void);
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

void right_index(void)
{	
	
	if (pos1 >= 0 ) PORTA = PORTA | (1<<pos1);
	if (pos1 == -1) PORTA = PORTA & 0xf0;
	
	if (pos1 >= 0 ) PORTD = PORTD | (1<<pos1);
	if (pos1 == -1) PORTD = PORTD & 0xf0;
	
	if (pos1 == -2) pos1 = 4;
	pos1--;
	
}

void left_index(void)
{
	if (pos2 <= 7 ) PORTA = PORTA | (1<<pos2);
	if (pos2 == 8) PORTA = PORTA & 0x0f;
	
	if (pos2 <= 7 ) PORTD = PORTD | (1<<pos2);
	if (pos2 == 8) PORTD = PORTD & 0x0f;
	
	if (pos2 == 9) pos2 = 3;
	pos2++;
}

void emergency_lights(void)
{
	left_index();
	right_index();
}

void CAN_read(void)
{
	//Vehicle dynamincs üzenetbõl használt adatok beolvasása
	raw_vel_x = can_rx_data[3];
	raw_vel_x = raw_vel_x & ((can_rx_data[4] & 0x0F)<<8);
			
	raw_vel_y = ((can_rx_data[4] & 0xF0)>>4);
	raw_vel_y = raw_vel_y & (8<<(can_rx_data[5] & 0b00111111)); 
			
	cnt_2bit = ((can_rx_data[7] & 0b11000000) >> 6 ); //Itt a maszkolás nem biztos, hogy kell, ez kérdés
			
			 
	//Vehicle controls üzenetbõl használt adatok beolvasása  
	raw_st_angle = can_rx_data2[0] & (can_rx_data2[1]<<8);
	
	cnt_3bit = 	((can_rx_data[4] & 0b11100000) >> 5 ); //Itt a maszkolás nem biztos, hogy kell, ez kérdés	
	
	//Adatok konvertálása
	st_angle = raw_st_angle * st_angle_can_factor - 1310.72;
	vel_x = raw_vel_x * velx_can_factor;
	vel_y = raw_vel_y * vely_can_factor - 25.6;	
	
	//Counterek biztonsági ellenõrzése
	if(cnt_2bit_local == 3) cnt_2bit_local = 0;
	else cnt_2bit_local++;
	if(cnt_2bit != cnt_2bit_local) CAN_sendError();  
	
	if(cnt_3bit_local == 7) cnt_3bit_local = 0;
	else cnt_3bit_local++;
	if(cnt_3bit != cnt_3bit_local) CAN_sendError();  

	can_msg_received = FALSE;		
}

void CAN_send(void)
{
	if(cnt_4bit == 15) cnt_4bit = 0;
	else cnt_4bit++;
	
	can_tx_data[0]= cnt_4bit<<4 | auto_det_crash<<3 | warning_toggle<<2 |PB0_pushed<<1 | PB2_pushed;
	
	CAN_SendMob(0,0x1FD,FALSE,1,can_tx_data);
}

void CAN_sendError(void)
{
	can_tx_error[0] = 0x00; //Ez még kérdés, hogy mit küldünk!
	
    CAN_SendMob(2, 0x100, FALSE, 1, can_tx_error);
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
		if((fabsf(st_angle) > st_angle_limit) && !is_turning && (turn_counter != 5)) turn_counter++;
		
		if (turn_counter == 5)
		{
			is_turning = TRUE; 		
		}
		
		if ((fabsf(st_angle) < 5 ) && is_turning)
		{
			is_turning = FALSE;
			blinker_reset();
			turn_counter = 0;
		}	
}

void detect_crash(void)
{
	for (int i=0;i<4;i++)
	{
		vel_abs[i] = vel_abs[i+1];
	}
	vel_abs[4]=hypotf(vel_x,vel_y);
	
	
	if ( (((vel_abs[4]-vel_abs[0]) / (0.20))  > min_accel) && (((vel_abs[4]-vel_abs[0]) / (0.20)) < max_accel) )
	{
		warning_toggle = TRUE;
		auto_det_crash = TRUE;
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
	CAN_ReceiveEnableMob(0, 0x22, FALSE, 8);	 // sebesség olvasás engedélyezése
	CAN_ReceiveEnableMob(1, 0x11, FALSE, 5);	// kormányszög olvasás engedélyezése
	
	sei();
	
	while(1)
	{
		//*****************************************************************************************************************
		//10 ms timer
		
		if(timer_task_10ms)
		{
		    if(can_msg_received) CAN_read();

			timer_task_10ms=FALSE;
		}
		//*****************************************************************************************************************
		//50 ms timer
		
		if(timer_task_50ms)
		{
			//************************************************************************************************************
			//Automatikus függvények 

			CAN_send();
			detect_crash();
			auto_blinker_off();		
			
			//*****************************************************************************************************************
			//PB0	(Jobb Index)	
			
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
			// PB1   (Vészvillogó)
			
			if((PINB & (1<<PB1)) == 0 && PB1_pushed == FALSE && PB1_re_enable_cnt == PB1_ENA_DELAY)
			{
				if( warning_toggle == FALSE ) warning_toggle = TRUE;
				else warning_toggle = FALSE;
				if(auto_det_crash) auto_det_crash = FALSE;
				
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
			// PB2     (Bal index)
			
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
			if (PB0_pushed && !warning_toggle && PB2_re_enable_cnt == BTN_ENA_DELAY) right_index();
			
			if (warning_toggle) emergency_lights();
			
			if(PB2_pushed && !warning_toggle && PB0_re_enable_cnt == BTN_ENA_DELAY) left_index();
			
			
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

	CANPAGE = 0;  // MOB 0 kiválasztása

	if ((CANSTMOB & (1<<RXOK)) != FALSE)
	{
		dlc = CANCDMOB & 0x0F;
		for (i=0; i<dlc; i++) can_rx_data[i] = CANMSG;
		
	}
			
	CANPAGE = 1;  // MOB 1 kiválasztása

	if ((CANSTMOB & (1<<RXOK)) != FALSE)
	{
		dlc = CANCDMOB & 0x0F;
		for (i=0; i<dlc; i++) can_rx_data2[i] = CANMSG;

	}
	
	CANSTMOB &= ~(1<<RXOK); // flag törlése
	
	
	can_msg_received = TRUE;
	CAN_ReceiveEnableMob(0, 0x22, FALSE, 8);	 // sebesség olvasás engedélyezése
	CAN_ReceiveEnableMob(1, 0x11, FALSE, 5);	// kormányszög olvasás engedélyezése
}



