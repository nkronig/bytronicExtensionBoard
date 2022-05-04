#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

volatile uint32_t TCA_t_counter = 0;
volatile uint8_t transmit[] = {0,1};
volatile uint8_t transmitProgress = 0;
volatile uint8_t transmitProgressMax = 5;
volatile int x = 0;
volatile int err = 0;
volatile uint8_t stopped = 1;
volatile uint8_t rec[16] ={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void TWI_Slave_init()
{
	PORTMUX.CTRLB |= (1<<4);
	TWI0.MCTRLA &= ~(1 << TWI_ENABLE_bp);
	TWI0.SADDR = 0x6F;

	//TWI0.SADDR = 0x71;
	TWI0.SCTRLA = 1 << TWI_APIEN_bp    /* Address/Stop Interrupt Enable: enabled */
	| 1 << TWI_DIEN_bp   /* Data Interrupt Enable: enabled */
	| 1 << TWI_ENABLE_bp /* Enable TWI Slave: enabled */
	| 1 << TWI_PIEN_bp   /* Stop Interrupt Enable: enabled */
	| 0 << TWI_PMEN_bp   /* Promiscuous Mode Enable: disabled */
	| 0 << TWI_SMEN_bp;  /* Smart Mode Enable: disabled */
	TWI0.SCTRLA |= TWI_ENABLE_bm;
}	
void I2C_0_send_ack(void)
{
	TWI0.SCTRLB = (0 << TWI_ACKACT_bp) | TWI_SCMD_RESPONSE_gc;
}
//------------------------------------------------
void prepareTransmit(volatile uint16_t v){
	transmitProgressMax=5;
	transmit[1] = (uint8_t)v;
	transmit[0] = (uint8_t)((uint16_t)v >> 8);
}
uint16_t prepareReceive(volatile uint8_t v[]){
	uint16_t b  = (uint16_t)((uint16_t)v[1] << 8) + (uint16_t)v[2];
	return(b);
}
volatile int i =0;
void setServoTime(int servo, int per){
	int time = per * 41 / 255 + 39;
	switch(servo){
		case 1:
		TCA0.SPLIT.LCMP2 = time;
		break;
		case 2:
		TCA0.SPLIT.LCMP1 = time;
		break;
		case 3:
		TCA0.SPLIT.LCMP0 = time;
		break;
		case 4:
		TCA0.SPLIT.HCMP2 = time;
		break;
		case 5:
		TCA0.SPLIT.HCMP1 = time;
		break;
		case 6:
		TCA0.SPLIT.HCMP0 = time;
		break;
	}
}
void setMosfet(uint16_t data){
	uint8_t num = data &0x003F;
	uint8_t reverse_num = 0;
	for (int i = 0; i < 6; i++) {
		if ((num & (1 << i)))
		reverse_num |= 1 << ((6 - 1) - i);
	}
	PORTC.OUT = (reverse_num);
	PORTB.OUT = (PORTB.IN & 0x2F) | (((data &0x0040) << 1) | ((data &0x0080) >> 1) | ((data &0x0100) >> 4));
	PORTA.OUT = (PORTA.IN & 0x3F) | (((data &0x0200) >> 2) | ((data &0x0400) >> 4));
}
ISR(TWI0_TWIS_vect){
	
	// Address received
	
	if ((TWI0.SSTATUS & TWI_APIF_bm) && (TWI0.SSTATUS & TWI_AP_bm)) {
		I2C_0_send_ack();
		if (!stopped)
		{
			
		}
		else{
			for (int i = 0; i<16; i++)
			{
				rec[i] = 0;
			}
			err = 1;
			x=0;
			stopped = 0;
			transmitProgress=0;
		}
	}
	// Data received
	if ((TWI0.SSTATUS & TWI_DIF_bm) && (!(TWI0.SSTATUS & TWI_DIR_bm))) {
		I2C_0_send_ack();
		rec[x] = TWI0_SDATA;
		x++;
	}
	if ((TWI0.SSTATUS & TWI_DIF_bm) && (TWI0.SSTATUS & TWI_DIR_bm)) {
		if (!(transmitProgress==transmitProgressMax))
		{
			uint8_t tmp = transmit[transmitProgress];
			TWI0.SCTRLB = 0b00000011;
			TWI0.SDATA = (int)tmp;
			transmitProgress++;
		}
		else{
			TWI0.SCTRLB = 0b00000011;
			TWI0.SDATA = 0;
		}
		//PORTC.OUT ^= (1<<0);
		err = 200;
	}
	if ((TWI0.SSTATUS & TWI_APIF_bm) && (!(TWI0.SSTATUS & TWI_AP_bm))) {
		
		if (x > 1)
		{
			if (rec[0] >= 1 && rec[0] <= 6)
			{
				if(rec[0] == 2){
					PORTB.OUTTGL = (1<<2);
				}
				setServoTime(rec[0], prepareReceive(rec));
			}
			if (rec[0]==0)
			{
				setMosfet(prepareReceive(rec));
			}
		}
		stopped = 1;
		TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
	}
}    // end ISR


void timerAInit(void)
{
	PORTMUX.CTRLC |= (PORTMUX_TCA02_bm);
	
	TCA0.SPLIT.LPER = 255;
	TCA0.SPLIT.HPER = 255;
	
	for(int i = 1; i<=6; i++){
		setServoTime(i, 128);
	}
	
	TCA0.SPLIT.CTRLD |= (TCA_SPLIT_SPLITM_bm);
	TCA0.SPLIT.CTRLB |= 0x77;

	TCA0.SPLIT.CTRLA |= (TCA_SPLIT_CLKSEL_DIV256_gc) | (TCA_SPLIT_ENABLE_bm);
}
void clkInit(void){
	//CPU_CCP = 0xD8;
	//CLKCTRL.MCLKCTRLA = 0x80;
	CPU_CCP = 0xD8;
	CLKCTRL.MCLKCTRLB = 0x01;
	CPU_CCP = 0xD8;
	CLKCTRL.OSC20MCTRLA = 0x02;
}
//------------------------------------------------


int main(void)
{
	PORTA.DIR |= 0xF8;
	PORTB.DIR |= 0xF7;
	PORTC.DIR |= 0x3F;
	
	TWI_Slave_init();	
	clkInit();
	timerAInit();
	sei(); // Enable global interrupts

	while (1)
	{

	}
}

