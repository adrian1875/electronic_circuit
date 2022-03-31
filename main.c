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


enum {  // ������: ���� ����ϴ� ������ �����Ͽ� ����Ѵ�, ���� �� �������� �����Ϸ��� ������ �ν��Ѵ�
	// ������ �����ϰ� �ƹ� ���ڵ� �Ҵ����� ������ 0���� ���������� �Ҵ��Ѵ�
	// 0->1->2�̷��� �Ҵ��ϴ� �߰��� 100�� �Ҵ��ϸ� �ٽ� 100->101->102������ �Ҵ��Ѵ�
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
}; // 1���� 17���� ������ ������ �Ҵ�
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
	serial_init(B9600);  // B9600�� 103���� ���ǵ�
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

	lcd_gotoxy(0, 0);  //  �ش� ��ġ�� lcd�� �ϴ��� ������ ǥ��
	lcd_string(" Select Button! ");  // ������ ǥ���Ѵ�
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
	//	[���� 1]	LED�� ������ ����
	//			��. LED : ���� ������� 0.3�� �������� �����ϰ� �Ͻÿ�.
	//				LED1��LED2��LED3��LED4��LED8��LED12��LED16��LED15
	//				��LED11��LED7��LED6��LED10��LED14��LED13��LED9��LED5
	//
	//					matrix_led�Լ� ���
	//
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	for( i = 0; i < 16; i++ ) {
		matrix_led(led_order[i]);  // �� ������ ���������� �ϳ��� �о��
		_delay_ms(300);
	}

	main_screen();
}

void select_station(char keyin)  // SW1�� �����־� 1�� ����
{
	char key = keyin;  // key�� 1����
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
		key = getkey(key);  // Ű�� 1
		if( key_flag ) {  // 1
			switch( key ) {  // 1
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	[���� 2] ����ġ�� �̿��Ͽ� LED�� ������ �Ǵ� ���������� �̵�
	//		��. LED
	//			SW1�� 1�� ���� ������ 1�����徿 ���������� �̵��ǰ�,
	//			SW2�� 1�� ���� ������ 1�����徿 ���������� �̵��ǰ� �Ͻÿ�.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
					case KEY_1 :  // KEY_1==1, key==1
					station++;  // station�� �⺻ ���� 1�̶� SW1�� ������ ��ĭ�� ������
					if( station == 17 )	station = 1;  // ������ ���� ù ������ �̵�
					break;  // while�� �ƴ϶� switch case�� ��������

					case KEY_2 :  // SW2�� ������
					station--;   // �� ĭ�� ������
					if( station == 0 )	station = 16;  // ù ���� �����ϸ� ������ ������ ��
					break;  // while�� �ƴ϶� switch case�� ��������

					case KEY_3 :  // select_station�Լ��� ���� ���� �� SW3�� ������ 
					if( state == SELECT_START_STATION ) {  // SELECT_START_STATION�� �⺻ ���� 0, state�� main���� 1�� �ʱ�ȭ
						start_station = station;  // ó�� SW3�� ������ �׶��� station���� ���۰��� �ȴ� 
						end_station = 1;  // end_station�� 1�� �ʱ�ȭ
						station = end_station;  // station�� 1�� �ʱ�ȭ

						lcd_gotoxy(0, 0);
						lcd_string(" End   Station  ");

						state = SELECT_END_STATION;  // state�� 2
					}
					else if( state == SELECT_END_STATION ) {  // �ٷ� ���� �ڵ忡 ���ؼ� 2==2�� �ȴ� 
						end_station = station;  // �� ���� ���� ���������� ����

						// lcd�� ���ۿ��� �������� ǥ��
						lcd_gotoxy(0, 0);   
						printf("Start:%02d Station", start_station);
						lcd_gotoxy(0, 1);
						printf("End  :%02d Station", end_station);

						// ������ �Ｚ�� ���� ���۰� �������� ���õǾ��ٰ� ����
						matrix_led(led_order[start_station - 1] | led_order[end_station - 1]);
						/*
							led_order�� 0��(start_station�� 1�̹Ƿ� 1-1) �ε����� ���� 4��(end_station�� 5�̹Ƿ� 5-1) �ε����� ���� |(or)����Ǿ� matrix_led�� ���޵ȴ�
							0x0001 | 0x0008  -> 0x0009�� ���޵�
							����Ʈ ���������� ���� ���丮���� �����Ѵ�
						*/
						_delay_ms(500);

						if( start_station == end_station ) {  // ���۰� ���� ���� �ԷµǸ� ������ ǥ���ϰ� lcd�ʱ�ȭ
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
	//	[���� 3] ��߿��� ������ ���� �� �ִܰŸ��� ����� ����Ͽ� LCD�� ǥ��
	//		��. LCD
	//			�Ÿ� : ��߿��� ������ ���� �ִ� �Ÿ��� ����Ͽ� ǥ��
	//			��� : Adult = 500 X �Ÿ�, Kid = 300 X �Ÿ�
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								distance = end_station - start_station;
								// ���� ���ۺ��� �ڿ� �ִ� ��� 
								if( distance < 0 ) {  // ���۰� ���� ��ġ �ľ�
									distance *= (-1);  
									if( distance < 8 ) {  // ������ ���� 8�������� �Ѿ�� �ʴ´ٸ� ���������� ���� ���� Ÿ��
										direction = REWARD;  // reward:0
									}
									else {  // 8�������� �Ѿ�ٸ� ���������� ��� ����
										direction = FORWARD;  // forward:1
										distance = 16 - distance;
									}
								}
								
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
								// ���� ���ۺ��� �տ� �ִ� ���
								else {
									if( distance > 8 ) {  // ������ ���� 8�����庸�� ũ�� ���������� ����
										direction = REWARD;
										distance = 16 - distance;
									}
									else {  // 8�����庸�� �۴ٸ� ���������� ��� ����
										direction = FORWARD;
									}
								}

								lcd_gotoxy(0, 0);  // ���ۿ��� ���ۿ��� �Ÿ��� ǥ��
								printf("Distance:%d      ", distance);

								fee = fee_tbl[ticket_type % 2] * distance;  // ��ݰ�� ����or��� ��� * �Ÿ�
								// fee_tbld���� 0�� �ε����� ���� 1�� �ε����� ����̹Ƿ� Ȧ�� ¦���� ������ 
								
								lcd_gotoxy(0, 1);  // ����� ǥ��
								printf("Fee : %04d Won  ", fee);

								_delay_ms(1000);

								lcd_gotoxy(0, 0);  // 
								lcd_string("Insert Money!!  ");
								lcd_gotoxy(0, 1);
								printf("Fee:%04d-0000   ", fee);  // ��� - 0000 (?), ���׿� ���� 100�� �� �����Ѵ�

								state = CALCULATE_FEE;  // state�� 3�� ����
							}

							return;
						}
						break;

					case KEY_5 :
						wdt_enable(WDTO_30MS);  // WDTO_30MS�� 1�� ���ǵǾ� ����
						while( 1 );
						break;
				}
			}
		
			matrix_led(led_order[station - 1]);  // station���� �������� ���� �Ǿ��ֱ� ������ �������� �޸𸮿� ����
			lcd_gotoxy(14, 1);
			printf("%02d", station);  // �������� lcd�� ǥ��
		}
	}

	void train_go(void)
	{
		int i, pos = start_station - 1;  // start_station�� 1(������)�̸� pos�� 0�� ����
		char str[30];

		lcd_gotoxy(0, 0);
		lcd_string("Train Is Going..");

		fnd_flag = ON;
		for( i = 0; i <= distance; i++ ) {  // �Ÿ���ŭ �ݺ���
			matrix_led(led_order[pos]);  // ���ۿ����� ���� ������, �������� ���ؼ� �� ĭ�� �̵���
			if( i < distance ) {  // �Ÿ��� 3�̶�� �̰��� 3�� ���� 
				lcd_gotoxy(0, 1);
				if( direction == FORWARD )	printf("----> : %d Minutes", distance - i);  // FORWARD�� 0�̰�, direction�� 4��� �����ϸ� else�� ������ 
				else					printf("<---- : %d Minutes", distance - i);
			}
			else {
				lcd_gotoxy(0, 0);  // ���� ������ �˸�
				lcd_string("Train Is Arrived");
				lcd_gotoxy(0, 1);
				lcd_string(" Thanks A Lot!  ");
			}

			fnd_buf[0] = i;  // FND_1�� �� ���� ���� �����Դ��� ǥ�õ�
			fnd_buf[1] = distance - i;  // FND_2�� ���� �ð��� ǥ�õ�  
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	[���� 5] ���
	//		��� ��Ŀ� ���缭 �ǽð����� PC �͹̳ο� ����Ͻÿ�.
	//		�̵��ð�, �����Ÿ�, ��߿�, ������, ���翪, ����� �͹̳ο� ���
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			serial_transmit('\f');  // '\f'�� �ƽ�Ű �ڵ�� 0x0C(10���� : 12)
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

			if( direction == FORWARD ) {  // direction�� 4��� �����ϸ� FORWARD�� 0�̶� else�� ����
				if( pos < 15 )	pos++;
				else			pos = 0;
			}
			else {  
				if( pos )	pos--;  // ���ۿ��� �������̸� pos�� 0�̶� ó������ else�� ����, ���� pos�� �ϳ��� ���ҽ�Ŵ
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

		mcu_init();  //  mcu�� �ʱ�ȭ ��
		variable_init();  //�������� �ʱ�ȭ

		sei();

		fdevopen((void *)lcd_data_write, 0);

		start_screen();  // ���� 1�� ��Ȳ�� �������
		state = SELECT_TICKET_TYPE; // state�� 0�� ����

		var_start();

		while( 1 ) {
			key = getkey(key);
			if( key_flag ) {  // 1
				switch( key ) {
					case KEY_1 :  // SW1�� �����ų� SW2�� ������ break�� ���� �Ʒ��� ���������� ������ ��
					case KEY_2 : 
					if( state == SELECT_TICKET_TYPE ) {  // SELECT_TICKET_TYPE�� state�� �ʱⰪ�� 0
						state = SELECT_START_STATION;  // SELECT_START_STATION�� �ʱⰪ�� 1 state 1�� ����

						lcd_command_write(0x01);
						lcd_gotoxy(0, 0);
						if( key == KEY_1 )		lcd_string("You Select Adult");  // SW1�� �����ٸ� �����̴�
						else if( key == KEY_2 )	lcd_string("You Select Kid  ");  // SW2�� �����ٸ� ��̴�

						ticket_type = key;  // ���ΰ� ��� ����� ������
						select_station(key);  // ����(SW1)�̶�� ����, ��߿��� �����ϱ� ������ select_station�� while�� ���������� ����
					}
					break;

					case KEY_3 :  // SW3�� ���ȴ�
					if( state == CALCULATE_FEE ) {  // select_station�Լ����� ���������� state�� 3�� ����Ǿ True�� �ȴ�
						if( money >= fee ) {  // ������ �����ָ� money�� �ܰ� ���Եȴ�
							if( money > fee ) {  // �ܰ� ��ݺ��� ũ�ٸ� �ܾ��� ǥ��
								lcd_gotoxy(0, 0);
								lcd_string("Here Is Change. ");
								lcd_gotoxy(0, 1);
								printf("Change : %04d   ", money - fee);
							}
							_delay_ms(1000);
							// �ܾ� ǥ�� �� SW4�� ������ ������ ���ٴ� ���� ǥ����, ������������ �Ÿ��� �Բ� ǥ��
							lcd_gotoxy(0, 0);
							lcd_string("  [S4] Go!!     ");
							lcd_gotoxy(0, 1);
							printf("  Distance : %d  ", distance);  

							state = TRAIN_GO;  // ����ϸ� state�� 4�� �ʱ�ȭ
						}
					}
					break;

					case KEY_4 :  // SW4�� ������ ������ ����
					if( state == TRAIN_GO )	train_go();  // case KEY_3�� ���ؼ� True�� �ȴ�
					break;

					case KEY_5 :
					wdt_enable(WDTO_30MS);
					while( 1 );
					break;
				}
			}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//	[���� 4] ���������� �̿��Ͽ� ���� ���  ǥ��
	//		1. VR1�� ADC �Է°��� ���� �ݾ����� LCD�� ǥ�ä�
	//		2. �Էµ� �ݾ��� ��ݰ� ���Ͽ� ũ�ų� ������ 'OK'��� ǥ���ϰ�
	//			�� ������ ������ �ǰ� �Ͻÿ�.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			if( state == CALCULATE_FEE ) {  // ������ ���� �ܾ��� ������Ű�� ���� select_station�Լ��� ����ǰ� ���� �� �̹Ƿ� state�� CALCULATE_FEE�� ��� 3�̴�
				lcd_gotoxy(9, 1);
				printf("%04d   ", money);  // lcd�� �ܰ�ǥ��

				if( money >= fee ) {  // �ܰ� >= ����̸� lcd�� okǥ��
					lcd_gotoxy(14, 1);
					lcd_string("OK");
				}
			}
		}

		return 0;
	}
