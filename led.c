#include <avr/io.h>
#include <util/delay.h>

#include "led.h"

#define MLED_PORT		PORTA  // ������� ���
#define MLED_DDR		DDRA
#define MLED_DATA		0x01
#define MLED_SHIFT		0x02
#define MLED_CLEAR		0x04  // PA2�� CLEAR ��Ʈ�� ���
#define MLED_LATCH1		0x10
#define MLED_LATCH2		0x20

// �Ʒ� #if 0���� �����ִ� �κ� ����
void matrix_led(unsigned int data)  // ������ �Ҵ��� �� �������� �о��
{  
	// ��� 1�� ���� �ִ� ���� ����� ���� 0->1�̳� 1->0ó�� �ٲ� �� ��ȭ�� ����  
	
	unsigned int tmp = ~data;  // �о�� �� ������ �ش� ���� ������ ��� ���� 1�� �ٲ�
	int i;

	MLED_PORT |= MLED_CLEAR;  // PA2 1�� ����, �������� ����
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);  // ���½�ȣ ����
	for( i = 0; i < 16; i++ ) {  
		if( tmp & 0x8000 )	MLED_PORT |= MLED_DATA;  // �������̶�� 1111 1111 1111 1110 & 1000 0000 0000 0000 
													 // 0000 0101 �����ִ� �������ͷ� ��ǲ�� �ش�
		else MLED_PORT &= ~MLED_DATA; 

		MLED_PORT |= MLED_SHIFT;  // 0000 0111  Ŭ�� ��ȣ
		tmp <<= 1;  // 1111 1111 1111 1100
		MLED_PORT &= ~MLED_SHIFT;  // 0000 0101  
	}  // for�� ���鼭 if�� �ɸ��� ������ �����ִ�

	MLED_PORT |= (MLED_LATCH1 | MLED_LATCH2);
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2); 
}

// select_station�Լ� �κ� ����
#if 0
void matrix_led(unsigned int data)  // select_station�Լ����� 0x0009�� ����
{
	unsigned int tmp = ~data;  // 1111 1111 1111 0110�� temp�� �����
	int i;

	MLED_PORT |= MLED_CLEAR;  // 0000 0000 | 0000 0100 PA2 1�� ����, �������� ����
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);  // 0000 0100 = 0000 0100 & ~(0001 0000 | 0010 0000) ���½�ȣ ����
	for( i = 0; i < 16; i++ ) {
		if( tmp & 0x8000 )	MLED_PORT |= MLED_DATA;  
		// if(1111 1111 1111 0110 & 1000 0000 0000 0000)  0000 0101 = 0000 0100 | 0000 0001  // PORTA�� ��ǲ�� �ش�
		else MLED_PORT &= ~MLED_DATA;  // 0000 0100 = 0000 0101 & (~0000 0001)  // if�� �����̸� 0000 0101�̳� 0000 0100�� MLED_PORT�� �ִ� 
									   // input��ȣ reset

		// �� if�� ���̶��
		MLED_PORT |= MLED_SHIFT;  // 0000 0111 = 0000 0101 | 0000 0010  // Ŭ�� ��ȣ �߻�
		tmp <<= 1;  // 1111 1111 1111 0110 << 1 -> 1111 1111 1110 1100
		MLED_PORT &= ~MLED_SHIFT;  // 0000 0101 = 0000 0111 & (~0000 0010)  // Ŭ�� ��ȣ ����  
	}  

	MLED_PORT |= (MLED_LATCH1 | MLED_LATCH2);  // 0011 0101 = 0000 0101 | (0001 0000 | 0010 0000)
											   // ����Ʈ ��������ic�� RCLK�� �谡 �Ǿ Shift-register �����Ͱ� ���丮�� �������Ϳ� �����
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);  // 0000 0101 =  0011 0101 & ~(0001 0000 | 0010 0000) ����Ʈ ��������ic�� RCLK ����
	
}
#endif


void matrix_alloff(void)
{
	int i;

	MLED_PORT |= MLED_CLEAR;
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);
	for( i = 0; i < 16; i++ ) {
		MLED_PORT |= MLED_DATA;

		MLED_PORT |= MLED_SHIFT;
		MLED_PORT &= ~MLED_SHIFT;
	}

	MLED_PORT |= (MLED_LATCH1 | MLED_LATCH2);
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);
}

void matrix_allon(void)
{
	MLED_PORT &= ~MLED_CLEAR;
	MLED_PORT |= (MLED_LATCH1 | MLED_LATCH2);
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);
}

void matrix_init(void)
{
	MLED_DDR = 0xFF;
	MLED_PORT = 0x00;

	matrix_alloff();
}

void matrix_alloff(void)
{
	int i;

	MLED_PORT |= MLED_CLEAR;
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);
	for( i = 0; i < 16; i++ ) {
		MLED_PORT |= MLED_DATA;

		MLED_PORT |= MLED_SHIFT;
		MLED_PORT &= ~MLED_SHIFT;
	}

	MLED_PORT |= (MLED_LATCH1 | MLED_LATCH2);
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);
}

void matrix_allon(void)
{
	MLED_PORT &= ~MLED_CLEAR;
	MLED_PORT |= (MLED_LATCH1 | MLED_LATCH2);
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);
}