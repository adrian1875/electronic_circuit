	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include <avr/eeprom.h>
	#include <avr/wdt.h>
	#include <util/delay.h>

	#include <stdio.h>

	#include "key.h"
	#include "fnd.h"
	#include "var.h"
	#include "lcd.h"
	#include "serial.h"
	#include "led.h"

	#define ON		1
	#define OFF	0

	#define FORWARD		0
	#define REWARD		1


	enum {
		NONE,
		ADULT,
		KID
	};

	enum {
		SELECT_TICKET_TYPE,
		SELECT_START_STATION,
		SELECT_END_STATION,
		CALCULATE_FEE,
		TRAIN_GO
	};

	extern volatile char key_flag;
	extern volatile char fnd_flag;
	extern volatile unsigned char fnd_buf[2];
	extern volatile int money;

	unsigned int led_order[16] = {
		0x0001, 0x0002, 0x0004, 0x0008, 0x0080, 0x0800, 0x8000, 0x4000,
		0x0400, 0x0040, 0x0020, 0x0200, 0x2000, 0x1000, 0x0100, 0x0010
	};
	volatile char state;
	volatile char ticket_type, direction;
	volatile int start_station, end_station, distance;
	volatile int fee;
	int fee_tbl[2] = { 300, 500 };


	void mcu_init(void)
	{
		key_init();
		fnd_init();
		var_init();
		lcd_init();
		serial_init(B9600);
		matrix_init();
	}

	void variable_init(void)
	{
		fnd_off();

		ticket_type = NONE;
		start_station = NONE;
		end_station = NONE;

		distance = 0;
		fee = 0;
	}

	void main_screen(void)
	{
		fnd_off();
		matrix_alloff();

		lcd_gotoxy(0, 0);
		lcd_string(" Select Button! ");
		lcd_gotoxy(0, 1);
		lcd_string("S1:Adult  S2:Kid");
	}

	void start_screen(void)
	{
		int i;

		lcd_string("   Welcome To   ");
		lcd_gotoxy(0, 1);
		lcd_string("Gangnam Station!");

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//	[문제 1]	LED의 순차적 점멸
		//			나. LED : 다음 순서대로 0.3초 간격으로 점멸하게 하시오.
		//				LED1→LED2→LED3→LED4→LED8→LED12→LED16→LED15
		//				→LED11→LED7→LED6→LED10→LED14→LED13→LED9→LED5
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		for( i = 0; i < 16; i++ ) {
			matrix_led(led_order[i]);
			_delay_ms(300);
		}

		main_screen();
	}

	void select_station(char keyin)
	{
		char key = keyin;
		int station;

		_delay_ms(500);

		start_station = 1;
		station = start_station;

		distance = 0;
		fee = 0;

		lcd_gotoxy(0, 0);
		lcd_string(" Start Station  ");
		lcd_gotoxy(0, 1);
		lcd_string("Station Num : ");

		while( 1 ) {
			key = getkey(key);
			if( key_flag ) {
				switch( key ) {
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					//	[문제 2] 스위치를 이용하여 LED를 정방향 또는 역방향으로 이동
					//		나. LED
					//			SW1를 1번 누를 때마다 1정거장씩 정방향으로 이동되고,
					//			SW2을 1번 누를 때마다 1정거장씩 역방향으로 이동되게 하시오.
					////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					case KEY_1 :
					station++;
					if( station == 17 )	station = 1;
					break;

					case KEY_2 :
					station--;
					if( station == 0 )	station = 16;
					break;

					case KEY_3 :
					if( state == SELECT_START_STATION ) {
						start_station = station;
						end_station = 1;
						station = end_station;

						lcd_gotoxy(0, 0);
						lcd_string(" End   Station  ");

						state = SELECT_END_STATION;
					}
					else if( state == SELECT_END_STATION ) {
						end_station = station;

						lcd_gotoxy(0, 0);
						printf("Start:%02d Station", start_station);
						lcd_gotoxy(0, 1);
						printf("End  :%02d Station", end_station);

						matrix_led(led_order[start_station - 1] | led_order[end_station - 1]);

						_delay_ms(500);

						if( start_station == end_station ) {
							lcd_gotoxy(0, 0);
							lcd_string("    Error!!     ");
							lcd_gotoxy(0, 1);
							lcd_string("Return To First!");
							_delay_ms(500);

							start_station = NONE;
							end_station = NONE;

							main_screen();

							state = SELECT_TICKET_TYPE;
						}
						else {
							////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							//	[문제 3] 출발역과 도착역 선택 후 최단거리와 요금을 계산하여 LCD에 표시
							//		가. LCD
							//			거리 : 출발역과 도착역 간의 최단 거리를 계산하여 표시
							//			요금 : Adult = 500 X 거리, Kid = 300 X 거리
							////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
							distance = end_station - start_station;
							if( distance < 0 ) {
								distance *= (-1);
								if( distance < 8 ) {
									direction = REWARD;
								}
								else {
									direction = FORWARD;
									distance = 16 - distance;
								}
							}
							else {
								if( distance > 8 ) {
									direction = REWARD;
									distance = 16 - distance;
								}
								else {
									direction = FORWARD;
								}
							}

							lcd_gotoxy(0, 0);
							printf("Distance:%d      ", distance);

							fee = fee_tbl[ticket_type % 2] * distance;
							lcd_gotoxy(0, 1);
							printf("Fee : %04d Won  ", fee);

							_delay_ms(1000);

							lcd_gotoxy(0, 0);
							lcd_string("Insert Money!!  ");
							lcd_gotoxy(0, 1);
							printf("Fee:%04d-0000   ", fee);

							state = CALCULATE_FEE;
						}

						return;
					}
					break;

					case KEY_5 :
					wdt_enable(WDTO_30MS);
					while( 1 );
					break;
				}
			}

			matrix_led(led_order[station - 1]);
			lcd_gotoxy(14, 1);
			printf("%02d", station);
		}
	}

	void train_go(void)
	{
		int i, pos = start_station - 1;
		char str[30];

		lcd_gotoxy(0, 0);
		lcd_string("Train Is Going..");

		fnd_flag = ON;
		for( i = 0; i <= distance; i++ ) {
			matrix_led(led_order[pos]);

			if( i < distance ) {
				lcd_gotoxy(0, 1);
				if( direction == FORWARD )	printf("----> : %d Minutes", distance - i);
				else					printf("<---- : %d Minutes", distance - i);
			}
			else {
				lcd_gotoxy(0, 0);
				lcd_string("Train Is Arrived");
				lcd_gotoxy(0, 1);
				lcd_string(" Thanks A Lot!  ");
			}

			fnd_buf[0] = i;
			fnd_buf[1] = distance - i;

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//	[문제 5] 통신
			//		출력 양식에 맞춰서 실시간으로 PC 터미널에 출력하시오.
			//		이동시간, 남은거리, 출발역, 도착역, 현재역, 요금을 터미널에 출력
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			serial_transmit('\f');
			sprintf(str, "Time/m        : %d\r\n", distance - i);
			serial_string(str);
			sprintf(str, "Distance      : %d\r\n", i);
			serial_string(str);
			sprintf(str, "Start Station : %02d\r\n", start_station);
			serial_string(str);
			sprintf(str, "End Station   : %02d\r\n", end_station);
			serial_string(str);
			sprintf(str, "Now Station   : %02d\r\n", pos + 1);
			serial_string(str);
			sprintf(str, "Money         : %04d\r\n", fee);
			serial_string(str);

			if( direction == FORWARD ) {
				if( pos < 15 )	pos++;
				else			pos = 0;
			}
			else {
				if( pos )	pos--;
				else		pos = 15;
			}

			_delay_ms(1000);
		}
		_delay_ms(1000);

		ticket_type = NONE;
		start_station = NONE;
		end_station = NONE;

		distance = 0;
		fee = 0;

		main_screen();

		state = SELECT_TICKET_TYPE;
	}

	int main(void)
	{
		unsigned char key = 0xFF;

		mcu_init();
		variable_init();

		sei();

		fdevopen((void *)lcd_data_write, 0);

		start_screen();
		state = SELECT_TICKET_TYPE;

		var_start();

		while( 1 ) {
			key = getkey(key);
			if( key_flag ) {
				switch( key ) {
					case KEY_1 :
					case KEY_2 :
					if( state == SELECT_TICKET_TYPE ) {
						state = SELECT_START_STATION;

						lcd_command_write(0x01);
						lcd_gotoxy(0, 0);
						if( key == KEY_1 )		lcd_string("You Select Adult");
						else if( key == KEY_2 )	lcd_string("You Select Kid  ");

						ticket_type = key;
						select_station(key);
					}
					break;

					case KEY_3 :
					if( state == CALCULATE_FEE ) {
						if( money >= fee ) {
							if( money > fee ) {
								lcd_gotoxy(0, 0);
								lcd_string("Here Is Change. ");
								lcd_gotoxy(0, 1);
								printf("Change : %04d   ", money - fee);
							}
							_delay_ms(1000);

							lcd_gotoxy(0, 0);
							lcd_string("  [S4] Go!!     ");
							lcd_gotoxy(0, 1);
							printf("  Distance : %d  ", distance);

							state = TRAIN_GO;
						}
					}
					break;

					case KEY_4 :
					if( state == TRAIN_GO )	train_go();
					break;

					case KEY_5 :
					wdt_enable(WDTO_30MS);
					while( 1 );
					break;
				}
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			//	[문제 4] 가변저항을 이용하여 지불 요금  표시
			//		1. VR1의 ADC 입력값을 지불 금액으로 LCD에 표시ㅣ
			//		2. 입력된 금액을 요금과 비교하여 크거나 같으면 'OK'라고 표시하고
			//			그 전에는 공백이 되게 하시오.
			////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if( state == CALCULATE_FEE ) {
				lcd_gotoxy(9, 1);
				printf("%04d   ", money);

				if( money >= fee ) {
					lcd_gotoxy(14, 1);
					lcd_string("OK");
				}
			}
		}

		return 0;
	}
