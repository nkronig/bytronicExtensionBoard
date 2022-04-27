#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>

volatile uint32_t TCA_t_counter = 0;

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
	PORTB.DIR |= 0xF3;
	PORTC.DIR |= 0x3F;
	
	clkInit();
	timerAInit();
	sei(); // Enable global interrupts
	setMosfet(0b0101001100111011);

	while (1)
	{
		for(int i = 0; i<=255; i++){
			setServoTime(1, i);
			_delay_ms(5);
		}
		_delay_ms(1000);
		for(int i = 255; i>=0; i--){
			setServoTime(1, i);
			_delay_ms(5);
		}
		_delay_ms(1000);
	}
}

