#include <avr/io.h>
#include <util/delay.h>

#include "led.h"

#define MLED_PORT		PORTA  // 출력으로 사용
#define MLED_DDR		DDRA
#define MLED_DATA		0x01
#define MLED_SHIFT		0x02
#define MLED_CLEAR		0x04  // PA2를 CLEAR 비트로 사용
#define MLED_LATCH1		0x10
#define MLED_LATCH2		0x20

// 아래 #if 0으로 묶여있는 부분 참고
void matrix_led(unsigned int data)  // 역마다 할당이 된 고유값을 읽어옴
{  
	// 계속 1이 들어가고 있는 것은 상관이 없다 0->1이나 1->0처럼 바뀔 때 변화가 생김  
	
	unsigned int tmp = ~data;  // 읽어온 역 정보를 해당 역을 제외한 모든 역을 1로 바꿈
	int i;

	MLED_PORT |= MLED_CLEAR;  // PA2 1로 만듦, 레지스터 리셋
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);  // 리셋신호 유지
	for( i = 0; i < 16; i++ ) {  
		if( tmp & 0x8000 )	MLED_PORT |= MLED_DATA;  // 강남역이라면 1111 1111 1111 1110 & 1000 0000 0000 0000 
													 // 0000 0101 위에있는 레지스터로 인풋을 준다
		else MLED_PORT &= ~MLED_DATA; 

		MLED_PORT |= MLED_SHIFT;  // 0000 0111  클락 신호
		tmp <<= 1;  // 1111 1111 1111 1100
		MLED_PORT &= ~MLED_SHIFT;  // 0000 0101  
	}  // for를 돌면서 if에 걸리면 강남만 켜져있다

	MLED_PORT |= (MLED_LATCH1 | MLED_LATCH2);
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2); 
}

// select_station함수 부분 설명
#if 0
void matrix_led(unsigned int data)  // select_station함수에서 0x0009이 전달
{
	unsigned int tmp = ~data;  // 1111 1111 1111 0110이 temp에 저장됨
	int i;

	MLED_PORT |= MLED_CLEAR;  // 0000 0000 | 0000 0100 PA2 1로 만듦, 레지스터 리셋
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);  // 0000 0100 = 0000 0100 & ~(0001 0000 | 0010 0000) 리셋신호 유지
	for( i = 0; i < 16; i++ ) {
		if( tmp & 0x8000 )	MLED_PORT |= MLED_DATA;  
		// if(1111 1111 1111 0110 & 1000 0000 0000 0000)  0000 0101 = 0000 0100 | 0000 0001  // PORTA로 인풋을 준다
		else MLED_PORT &= ~MLED_DATA;  // 0000 0100 = 0000 0101 & (~0000 0001)  // if가 거짓이면 0000 0101이나 0000 0100이 MLED_PORT에 있다 
									   // input신호 reset

		// 위 if가 참이라면
		MLED_PORT |= MLED_SHIFT;  // 0000 0111 = 0000 0101 | 0000 0010  // 클락 신호 발생
		tmp <<= 1;  // 1111 1111 1111 0110 << 1 -> 1111 1111 1110 1100
		MLED_PORT &= ~MLED_SHIFT;  // 0000 0101 = 0000 0111 & (~0000 0010)  // 클락 신호 리셋  
	}  

	MLED_PORT |= (MLED_LATCH1 | MLED_LATCH2);  // 0011 0101 = 0000 0101 | (0001 0000 | 0010 0000)
											   // 시프트 레지스터ic의 RCLK가 ↑가 되어서 Shift-register 데이터가 스토리지 레지스터에 저장됨
	MLED_PORT &= ~(MLED_LATCH1 | MLED_LATCH2);  // 0000 0101 =  0011 0101 & ~(0001 0000 | 0010 0000) 시프트 레지스터ic의 RCLK 리셋
	
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