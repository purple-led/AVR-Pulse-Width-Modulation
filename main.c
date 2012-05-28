/**
* "Pulse-width modulation v1.1"
* Device change porosity with button click.
* If voltage on PD6 is +5, counting will direct, otherwise will reverse.
* GNU GPLv3 © evil_linuxoid 2012
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define RS PA0 //RS is connected to PA0
#define RW PA1 //RW is connected to PA1
#define EN PA2 //EN is connected to PA2

#define set_true(in, pos) in |= (1 << pos)
#define set_false(in, pos) in &= ~(1 << pos)
#define read_bit(in, pos) ((in & (1 << pos)) ? 1 : 0)

int stack_over = 0;
int step = 0;

/* Counter for clicks */
int count_click = 0;
const int CLICK_MAX = 10;
const uint8_t BUTT_MAX_HIST_VOLT = 5; //length of sample {1, 8}
uint8_t button_voltage_status = 0xff;
uint8_t button_click_up_status = 0;
uint8_t button_click_down_status = 0;
uint8_t button_1_history_voltage = 0xff;
uint8_t button_timer_counter = 0;

/* Input command in LCD */
void lcd_com(unsigned char p)
{
	set_false(PORTA, RS); //RS = 0 (for command)
	set_true(PORTA, EN); //EN = 1 (start to input command in LCD)
	PORTC = p; //input command 
	_delay_us(100); //duration of the signal
	set_false(PORTA, EN); //EN = 0 (finish to input command in LCD)
	
	if(p == 0x01 || p == 0x02) _delay_us(1800); //pause for execution
	else _delay_us(50);
}

/* Input data in LCD */
void lcd_dat(unsigned char p)
{
	set_true(PORTA, RS); //RS = 1(for data)
	set_true(PORTA, EN); //EN = 1 (start to input data in LCD)
	PORTC = p; //input data
	_delay_us(100); //duration of the signal
	set_false(PORTA, EN); //EN = 0 (finish to input data in LCD)
	_delay_us(50); //pause for execution
}

void lcd_write(char * string)
{
	int i = 0, count = strlen(string);
	for(i = 0; i < count; i ++) lcd_dat(string[i]);
}

/* The initialization function LCD */
void lcd_init(void)
{
	lcd_com(0x08); //display off(1640us)
	lcd_com(0x38); //8 bit, 2 lines(40us)
	lcd_com(0x01); //cleaning the display(1640us)
	lcd_com(0x06); //shift the cursor to the right(40us)
	lcd_com(0x0D); //a blinking cursor(40us)
}

ISR(TIMER0_OVF_vect)
{
	if(button_timer_counter > 80)
	{
		button_timer_counter = 0;
		button_1_history_voltage = ((button_1_history_voltage << 1) + read_bit(PIND, PD3)) & ((1 << BUTT_MAX_HIST_VOLT) - 1);

		if(button_1_history_voltage == (1 << BUTT_MAX_HIST_VOLT) - 1)
		{
			if(!read_bit(button_voltage_status, PD3)) set_true(button_click_up_status, PD3);
			set_true(button_voltage_status, PD3);
		}
		else if(button_1_history_voltage == 0)
		{
			if(read_bit(button_voltage_status, PD3)) set_true(button_click_down_status, PD3);
			set_false(button_voltage_status, PD3);
		}
	}
	else button_timer_counter ++;
}

int main()
{ 
	/* For LCD */
	set_true(DDRA, RS); //RC is output
	set_true(DDRA, RW); //RW is output
	set_true(DDRA, EN); //EN is output
	PORTA = 0x00; //zero in output
	DDRC = 0xFF; //PD0-7 are output
	PORTC = 0x00; //PD0-7 are output

	/* For counting clicks */
	DDRB = 15; /* PB0, PB1, PB2, PB3 are output */
	set_false(DDRD, PD3); //button
	set_false(DDRD, PD6); //direction of counting
	set_true(PORTD, PD3); //PD3 gets extra resistance of 100 kOhm
	set_true(PORTD, PD6); //PD6 gets extra resistance of 100 kOhm
	
	/* Timer 0 on */
	TCCR0 = 0x61;
	set_true(TIMSK, TOIE0);
	OCR0 = 0;
	
	/* All interrupts are enable */
	set_true(SREG, 7);
	
	lcd_init();
	
	char * greet_1 = "   \"PWM v1.1\"";
	char * greet_2 = "--___--___--___-";
	
	lcd_com(0x0C);
	lcd_write(greet_1);
	lcd_com(0xC0);
	lcd_write(greet_2);
	_delay_ms(1500);
	
	char * step_1_0 = "Press button";
	char * step_1_1 = "to continue...";
	lcd_com(0x01);
	lcd_com(0x02);
	lcd_write(step_1_0);
	lcd_com(0xC0);
	lcd_write(step_1_1);
	
	while(1)
	{
		_delay_ms(100);
		if(read_bit(button_click_down_status, PD3))
		{
			set_false(button_click_down_status, PD3);
			break;
		}
	}
 
	lcd_com(0x01);
	lcd_com(0x02);

	const char * note_click_1 = "Porosity: ";
	char note_click_2[16];

	sprintf(note_click_2, "Max value: %d", CLICK_MAX);

	lcd_write(note_click_1);
	lcd_com(0xC0);
	lcd_write(note_click_2);	

	step = 1;
	char buf[3];
	char * curs_beg_count = 0x80 + strlen(note_click_1);

	while(1)
	{
		lcd_com(curs_beg_count);
		
		if(read_bit(button_click_down_status, PD3))
		{
			set_false(button_click_down_status, PD3);
			count_click += (read_bit(PIND, PD6) ? 1 : -1);
			
			if(count_click > CLICK_MAX || count_click < 0)
			{
				count_click = count_click > CLICK_MAX ? 0 : CLICK_MAX;
				stack_over = 1;
				
				PORTB |= 15;
			}
			else if(stack_over == 1)
			{
				stack_over = 0;
				
				set_false(PORTB, PB0);
				set_false(PORTB, PB1);
				set_false(PORTB, PB2);
			}
		}
		
		OCR0 = round(count_click * 255.0 / CLICK_MAX);

		if(count_click < 10) sprintf(buf, "  %d", count_click);
		else if(count_click < 100) sprintf(buf, " %d", count_click);
		else if(count_click < 1000) sprintf(buf, "%d", count_click);
		lcd_write(buf);

		_delay_ms(50);
	}

	return 0;
}