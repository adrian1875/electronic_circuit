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

#define ON	1
#define OFF	0

#define FORWARD	0
#define REWARD	1


enum {  // 열거형: 자주 사용하는 변수를 나열하여 사용한다, 나열 된 변수들은 컴파일러가 정수로 인식한다
	// 변수만 선언하고 아무 숫자도 할당하지 않으면 0부터 순차적으로 할당한다
	// 0->1->2이렇게 할당하다 중간에 100을 할당하면 다시 100->101->102순으로 할당한다
	NONE = 0,
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
	0x0001, 0x0002, 0x0004, 0x0008, 
	0x0080, 0x0800, 0x8000, 0x4000,
	0x0400, 0x0040, 0x0020, 0x0200,
	0x2000, 0x1000, 0x0100, 0x0010
}; // 1부터 17까지 역마다 고유값 할당
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
	serial_init(B9600);  // B9600는 103으로 정의됨
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

	lcd_gotoxy(0, 0);  //  해당 위치의 lcd에 하단의 정보를 표기
	lcd_string(" Select Button! ");  // 정보를 표기한다
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
	//
	//					matrix_led함수 사용
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	for( i = 0; i < 16; i++ ) {
		matrix_led(led_order[i]);  // 역 정보를 순차적으로 하나씩 읽어옴
		_delay_ms(300);
	}

	main_screen();
}

void select_station(char keyin)  // SW1이 눌려있어 1이 들어옴
{
	char key = keyin;  // key에 1저장
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
		key = getkey(key);  // 키는 1
		if( key_flag ) {  // 1
			switch( key ) {  // 1
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	[문제 2] 스위치를 이용하여 LED를 정방향 또는 역방향으로 이동
	//		나. LED
	//			SW1를 1번 누를 때마다 1정거장씩 정방향으로 이동되고,
	//			SW2을 1번 누를 때마다 1정거장씩 역방향으로 이동되게 하시오.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					case KEY_1 :  // KEY_1==1, key==1
					station++;  // station의 기본 값은 1이라 SW1이 눌리면 한칸씩 전진함
					if( station == 17 )	station = 1;  // 끝까지 가면 첫 역으로 이동
					break;  // while이 아니라 switch case를 빠져나감

					case KEY_2 :  // SW2가 눌리면
					station--;   // 한 칸씩 후퇴함
					if( station == 0 )	station = 16;  // 첫 역에 도착하면 마지막 역으로 감
					break;  // while이 아니라 switch case를 빠져나감

					case KEY_3 :  // select_station함수가 실행 중일 때 SW3이 눌리면 
					if( state == SELECT_START_STATION ) {  // SELECT_START_STATION의 기본 값은 0, state는 main에서 1로 초기화
						start_station = station;  // 처음 SW3을 누르면 그때의 station값이 시작값이 된다 
						end_station = 1;  // end_station을 1로 초기화
						station = end_station;  // station을 1로 초기화

						lcd_gotoxy(0, 0);
						lcd_string(" End   Station  ");

						state = SELECT_END_STATION;  // state는 2
					}
					else if( state == SELECT_END_STATION ) {  // 바로 위의 코드에 의해서 2==2가 된다 
						end_station = station;  // 그 때의 역을 도착역으로 지정

						// lcd에 시작역과 도착역을 표시
						lcd_gotoxy(0, 0);   
						printf("Start:%02d Station", start_station);
						lcd_gotoxy(0, 1);
						printf("End  :%02d Station", end_station);

						// 강남과 삼성이 각각 시작과 도착으로 선택되었다고 가정
						matrix_led(led_order[start_station - 1] | led_order[end_station - 1]);
						/*
							led_order의 0번(start_station이 1이므로 1-1) 인덱스의 값과 4번(end_station이 5이므로 5-1) 인덱스의 값이 |(or)연산되어 matrix_led에 전달된다
							0x0001 | 0x0008  -> 0x0009이 전달됨
							시프트 레지스터의 값을 스토리지에 저장한다
						*/
						_delay_ms(500);

						if( start_station == end_station ) {  // 시작과 끝이 같게 입력되면 에러를 표시하고 lcd초기화
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
								// 끝이 시작보다 뒤에 있는 경우 
								if( distance < 0 ) {  // 시작과 끝의 위치 파악
									distance *= (-1);  
									if( distance < 8 ) {  // 사이의 역이 8정거장을 넘어가지 않는다면 역방향으로 가는 것을 타고
										direction = REWARD;  // reward:0
									}
									else {  // 8정거장을 넘어간다면 정방향으로 계속 간다
										direction = FORWARD;  // forward:1
										distance = 16 - distance;
									}
								}
								
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								// 끝이 시작보다 앞에 있는 경우
								else {
									if( distance > 8 ) {  // 사이의 역이 8정거장보다 크면 역방향으로 가고
										direction = REWARD;
										distance = 16 - distance;
									}
									else {  // 8정거장보다 작다면 정방향으로 계속 간다
										direction = FORWARD;
									}
								}

								lcd_gotoxy(0, 0);  // 시작역과 종작역의 거리를 표시
								printf("Distance:%d      ", distance);

								fee = fee_tbl[ticket_type % 2] * distance;  // 요금계산 성인or어린이 요금 * 거리
								// fee_tbld에서 0번 인덱스가 성인 1번 인덱스가 어린이이므로 홀수 짝수로 구별함 
								
								lcd_gotoxy(0, 1);  // 요금을 표시
								printf("Fee : %04d Won  ", fee);

								_delay_ms(1000);

								lcd_gotoxy(0, 0);  // 
								lcd_string("Insert Money!!  ");
								lcd_gotoxy(0, 1);
								printf("Fee:%04d-0000   ", fee);  // 요금 - 0000 (?), 저항에 따라 100원 씩 증감한다

								state = CALCULATE_FEE;  // state애 3을 저장
							}

							return;
						}
						break;

					case KEY_5 :
						wdt_enable(WDTO_30MS);  // WDTO_30MS는 1로 정의되어 있음
						while( 1 );
						break;
				}
			}
		
			matrix_led(led_order[station - 1]);  // station에는 종착역이 저장 되어있기 때문에 종착역을 메모리에 저장
			lcd_gotoxy(14, 1);
			printf("%02d", station);  // 종착역을 lcd에 표시
		}
	}

	void train_go(void)
	{
		int i, pos = start_station - 1;  // start_station이 1(강남역)이면 pos에 0이 저장
		char str[30];

		lcd_gotoxy(0, 0);
		lcd_string("Train Is Going..");

		fnd_flag = ON;
		for( i = 0; i <= distance; i++ ) {  // 거리만큼 반복함
			matrix_led(led_order[pos]);  // 시작역에만 불이 들어오고, 도착지를 향해서 한 칸씩 이동함
			if( i < distance ) {  // 거리가 3이라면 이곳엔 3번 들어옴 
				lcd_gotoxy(0, 1);
				if( direction == FORWARD )	printf("----> : %d Minutes", distance - i);  // FORWARD는 0이고, direction은 4라고 가정하면 else로 빠진다 
				else					printf("<---- : %d Minutes", distance - i);
			}
			else {
				lcd_gotoxy(0, 0);  // 도착 했음을 알림
				lcd_string("Train Is Arrived");
				lcd_gotoxy(0, 1);
				lcd_string(" Thanks A Lot!  ");
			}

			fnd_buf[0] = i;  // FND_1에 몇 개의 역을 지나왔는지 표시됨
			fnd_buf[1] = distance - i;  // FND_2에 남은 시간이 표시됨  
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	[문제 5] 통신
	//		출력 양식에 맞춰서 실시간으로 PC 터미널에 출력하시오.
	//		이동시간, 남은거리, 출발역, 도착역, 현재역, 요금을 터미널에 출력
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			serial_transmit('\f');  // '\f'는 아스키 코드로 0x0C(10진수 : 12)
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

			if( direction == FORWARD ) {  // direction은 4라고 가정하면 FORWARD가 0이라 else로 빠짐
				if( pos < 15 )	pos++;
				else			pos = 0;
			}
			else {  
				if( pos )	pos--;  // 시작역이 강남역이면 pos는 0이라 처음에는 else로 빠짐, 이후 pos를 하나씩 감소시킴
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

		mcu_init();  //  mcu를 초기화 함
		variable_init();  //변수들을 초기화

		sei();

		fdevopen((void *)lcd_data_write, 0);

		start_screen();  // 문제 1의 상황을 만들어줌
		state = SELECT_TICKET_TYPE; // state에 0을 저장

		var_start();

		while( 1 ) {
			key = getkey(key);
			if( key_flag ) {  // 1
				switch( key ) {
					case KEY_1 :  // SW1이 눌리거나 SW2가 눌리면 break가 없어 아래로 순차적으로 실행이 됨
					case KEY_2 : 
					if( state == SELECT_TICKET_TYPE ) {  // SELECT_TICKET_TYPE과 state는 초기값이 0
						state = SELECT_START_STATION;  // SELECT_START_STATION은 초기값이 1 state 1을 넣음

						lcd_command_write(0x01);
						lcd_gotoxy(0, 0);
						if( key == KEY_1 )		lcd_string("You Select Adult");  // SW1을 눌렀다면 성인이다
						else if( key == KEY_2 )	lcd_string("You Select Kid  ");  // SW2을 눌렀다면 어린이다

						ticket_type = key;  // 성인과 어린이 요금을 선택함
						select_station(key);  // 성인(SW1)이라고 가정, 출발역을 선택하기 전에는 select_station의 while을 빠져나가지 못함
					}
					break;

					case KEY_3 :  // SW3이 눌렸다
					if( state == CALCULATE_FEE ) {  // select_station함수에서 최종적으로 state에 3이 저장되어서 True가 된다
						if( money >= fee ) {  // 저항을 돌려주면 money에 잔고가 차게된다
							if( money > fee ) {  // 잔고가 요금보다 크다면 잔액을 표시
								lcd_gotoxy(0, 0);
								lcd_string("Here Is Change. ");
								lcd_gotoxy(0, 1);
								printf("Change : %04d   ", money - fee);
							}
							_delay_ms(1000);
							// 잔액 표시 후 SW4를 누르면 기차가 간다는 것을 표시함, 도착역까지의 거리도 함께 표시
							lcd_gotoxy(0, 0);
							lcd_string("  [S4] Go!!     ");
							lcd_gotoxy(0, 1);
							printf("  Distance : %d  ", distance);  

							state = TRAIN_GO;  // 출발하면 state를 4로 초기화
						}
					}
					break;

					case KEY_4 :  // SW4가 누르면 기차가 간다
					if( state == TRAIN_GO )	train_go();  // case KEY_3에 의해서 True가 된다
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
			if( state == CALCULATE_FEE ) {  // 저항을 돌려 잔액을 증가시키는 것은 select_station함수가 실행되고 있을 때 이므로 state와 CALCULATE_FEE는 모두 3이다
				lcd_gotoxy(9, 1);
				printf("%04d   ", money);  // lcd에 잔고표시

				if( money >= fee ) {  // 잔고 >= 요금이면 lcd에 ok표시
					lcd_gotoxy(14, 1);
					lcd_string("OK");
				}
			}
		}

		return 0;
	}
