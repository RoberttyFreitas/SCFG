#include <avr/io.h>
#include <avr/interrupt.h>
#include "utils.h"
#include "spi.h"
#include "mfrc522.h"
#include "mfrc522_cmd.h"
#include "mfrc522_reg.h"

#define F_CPU 1000000UL
#define FOSC 16000000 // Clock Speed
#define BAUD 9600
#define USART_BAUDRATE FOSC/16/BAUD-1

#define Sensor_C0	0
#define Sensor_C1	1
#define Botao_Cad	2
#define Botao_Des	3
#define Botao_Cos	4
#define State_IDLE	-1

#define COD_ENTRADA		'1'
#define COD_SAIDA		'2'
#define COD_LEITURA		'3'
#define COD_CADASTRA	'4'
#define COD_DESC		'5'
#define COD_ERRO		'6'

int flag_action = 0;
int estado_anterior = State_IDLE, estado = State_IDLE;
char transmitir = 0x00;

uint8_t byte;
uint8_t  str[MAX_LEN];
uint8_t SelfTestBuffer[64];


void USART_init(unsigned int ubrr){
	//UART init
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void USART_Transmit( unsigned char data ){
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)));
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

ISR(PCINT1_vect){
	
	if((~PINC) & (1 << 0)){
		estado = Sensor_C0;
	}else if((~PINC) & (1 << 1)){
		estado = Sensor_C1;
	}else if(PINC & (1 << 2)){
		estado = Botao_Cad;
	}else if(PINC & (1 << 3)){
		estado = Botao_Des;
	}else if(PINC & (1 << 4)){
		estado = Botao_Cos;
	}
}

void setup_uc(){
	
	spi_init();
	USART_init(USART_BAUDRATE);
	mfrc522_init();
	
	byte = mfrc522_read(ComIEnReg);
	mfrc522_write(ComIEnReg,byte|0x20);
	byte = mfrc522_read(DivIEnReg);
	mfrc522_write(DivIEnReg,byte|0x80);
	
	// Configura o RX e TX
	DDRD = 0b01100010;
	PORTD = 0x00;
	
	//SET all PORTC as input
	DDRC  = 0x00;
	PORTC = 0x03;
	
	//Enable Pin Change INT[2]
	PCICR = 0b00000010;

	/*Pin Change Interrupt MASK*/
	/*bits 23 to 16*/
	PCMSK2 = 0x00;
	/*bits 14 to 08*//*Doesnt exist PCINT15*/
	PCMSK1 = 0x1F;
	/*bits 07 to 00*/
	PCMSK0 = 0x00;
}

void read_tag(){
	
	flag_action = 1;
	while(flag_action){
		byte = mfrc522_request(PICC_REQALL,str);
		
		if(byte == CARD_FOUND){
			byte = mfrc522_get_card_serial(str);
			if(byte ==  CARD_FOUND){
				USART_Transmit(transmitir);
				for(int i =0; i<4; i++){
					uint8_t charHIGH = (str[i]>>4);
					uint8_t charLOW = str[i] & 0x0F;
					
					if(charHIGH >= 10){
						charHIGH += 55;
					}
					else charHIGH += 48;
					
					if(charLOW >= 10){
						charLOW += 55;
					}
					else charLOW += 48;
					USART_Transmit(charHIGH);
					USART_Transmit(charLOW);
				}
				flag_action = 0;
			}
		}
	}
	
}

void machine_state(){
	
	if(estado != State_IDLE){
		estado_anterior = estado;
		if(estado_anterior == Sensor_C0)
			transmitir = COD_ENTRADA;
		else if(estado_anterior == Sensor_C1)
			transmitir = COD_SAIDA;
		else if(estado_anterior == Botao_Cad)
			transmitir = COD_CADASTRA;
		else if(estado_anterior == Botao_Des)
			transmitir = COD_DESC;
		else if(estado_anterior == Botao_Cos)
			transmitir = COD_LEITURA;
		read_tag();
		estado = State_IDLE;
	}
}

int main(){

	estado = State_IDLE;
	estado_anterior = State_IDLE;
	
	setup_uc();
	sei();
	
	while(1){
		machine_state();
	}
}
