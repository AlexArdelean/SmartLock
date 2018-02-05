#include <stdint.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <string.h> 
#include <math.h> 
#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <avr/eeprom.h> 
#include <avr/portpins.h> 
#include <avr/pgmspace.h> 
 
//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h" 
#include <math.h>
#include "C:\Users\Test\Documents\Atmel Studio\includes\usart.h"
#include "C:\Users\Test\Documents\Atmel Studio\includes\rcc522.h"
#include "C:\Users\Test\Documents\Atmel Studio\includes\keypad.h"
#include "C:\Users\Test\Documents\Atmel Studio\includes\lcd.h"
#include "C:\Users\Test\Documents\Atmel Studio\includes\servo.h"

#define horizontal 0
#define vertical 1
#define clockwise 2
#define counterclockwise 3
#define lock 4
#define open 5

unsigned char* determineDoorPositions(unsigned char rotation, unsigned int degrees, unsigned char userstart, unsigned char userend);

char test = 0x00, i = 0;
unsigned char receivedData = 0x00;
unsigned char button1 = 0, button2 = 0, button3 = 0, button4 = 0;
unsigned char currentState = lock;
unsigned char pb; //port b
unsigned char motionDetected = 0x00;
unsigned char tmpmotion = 0x00;
unsigned int detectcount = 0;
char password[4] = {'1', '2', '3', '4'};
unsigned int degrees		= 135;                    //135
unsigned int clockdirection = clockwise;             //clockwise
char keypadCount = 0;

enum LEDState {INIT,Wait_1, Reset_1} Test_state;
void Test_Init(){
	Test_state = INIT;
}
enum Keypad {Keypad_INIT, Keypad_Password, Keypad_Wait, Keypad_Change, Keypad_Degree, Keypad_Clock} Keypad_state;
void InitLock_Init(){
	Keypad_state = Keypad_INIT;
}
enum LCD_Display {Display_INIT} LCD_state;
void LCD_Init(){
	LCD_state = Display_INIT;
}
enum PIR_Sensor {PIR_INIT} PIR_state, PIR_state2, PIR_state3;
void PIR_Init(){
	PIR_state = PIR_INIT;
}
void PIR_Detect_Init(){
	PIR_state2 = PIR_INIT;
}
void PIR_Detect_Tick(){
	tmpmotion += PINB & 0x04;
	detectcount++;
	if (detectcount >= 100){
		motionDetected = tmpmotion;
		tmpmotion = 0;
		detectcount = 0;
	}
}
void PIR_Tick(){
	if (motionDetected)
		UART_Send_BlueTooth("Y",0);
	else if (!motionDetected)
		UART_Send_BlueTooth("N",0);
	motionDetected = 0x00;
}

char attempt[4] = "";
int newDegree[3] = {0,0,0};
unsigned char invalid = 0x00;
void InitLockTask_Tick(){
	unsigned char currentKey = GetKeypadKey();

	switch(Keypad_state){
		case Keypad_INIT:
			Keypad_state = Keypad_Wait;
		break;
		case Keypad_Wait:
			if (currentKey){
				if (keypadCount == 0)
					LCD_ClearScreen();
				if (currentKey == '#'){
					Keypad_state = Keypad_Change;
					LCD_WriteData('#');
				}
				else{
					attempt[keypadCount] = currentKey;
					LCD_WriteData('*');
					Keypad_state = Keypad_Password;
				}
			}
			while (GetKeypadKey()){asm("nop");}
		break;
		case Keypad_Password:
			if (currentKey){
				keypadCount++;
				LCD_WriteData('*');
				attempt[keypadCount] = currentKey;
				if (keypadCount >= 3){
					if (attempt[0] == password[0] && attempt[1] == password[1] 
					&& attempt[2] == password[2] && attempt[3] == password[3]){
						if (currentState == lock){
							servo(open, clockdirection, degrees);
						}
						delay_ms(100);
						currentState = open;
					}
					LCD_ClearScreen();
					keypadCount = 0;
					Keypad_state = Keypad_Wait;
					memset(attempt, 0, strlen(attempt));
				}
			while (GetKeypadKey()){asm("nop");}
			}
		break;
		case Keypad_Change:
			if(currentKey){
				LCD_WriteData(currentKey);
				attempt[keypadCount] = currentKey;
				keypadCount++;
				if (keypadCount == 4){
					if (attempt[0] == password[0] && attempt[1] == password[1]
					&& attempt[2] == password[2] && attempt[3] == password[3]){
						LCD_WriteData('#');
					}
					else {
						keypadCount = 0;
						Keypad_state = Keypad_Wait;
						memset(attempt, 0, strlen(attempt));
						LCD_ClearScreen();
					}
				}
				if (keypadCount == 5 && currentKey != 'D' && currentKey != 'C')
					password[keypadCount-5] = currentKey;
				if (keypadCount >= 6)
					password[keypadCount-5] = currentKey;
				if (keypadCount == 5 && currentKey == 'D')
					Keypad_state = Keypad_Degree;
				if (keypadCount == 5 && currentKey == 'C')
					Keypad_state = Keypad_Clock;			
				if (keypadCount == 8){
					keypadCount = 0;
					Keypad_state = Keypad_Wait;
					LCD_ClearScreen();
				}
				while(GetKeypadKey()){asm("nop");}
			}
		break;
		case Keypad_Clock:
			if (currentKey){
				LCD_ClearScreen();
				if (currentKey == '1'){
					clockdirection = clockwise;
					LCD_DisplayString(1, "Clockwise");
				}
				if (currentKey == '2'){
					clockdirection = counterclockwise;
					LCD_DisplayString(1, "CounterClockwise");
				}
				keypadCount = 0;
				Keypad_state = Keypad_Wait;
				while(GetKeypadKey()){asm("nop");}
			}
		break;
		case Keypad_Degree:
			if (currentKey){
				LCD_WriteData(currentKey);
				keypadCount++; //at 6 on start
				if (keypadCount == 6)
					newDegree[2] = currentKey - '0';
				if (keypadCount == 7)
					newDegree[1] = currentKey - '0';
				if (keypadCount == 8){
					newDegree[0] = currentKey - '0';
					unsigned int tmp = 0;
					for (int i = 0; i < 3;i++){
						tmp += newDegree[i] * pow(10, i);
						if (newDegree[i] < 0 || newDegree[i] > 9)
							invalid = 1;
					}
					if (tmp > 260)
						invalid = 1;
					LCD_ClearScreen();
					if (!invalid){
						degrees = tmp;
						LCD_WriteData(newDegree[2] + '0');
						LCD_WriteData(newDegree[1] + '0');
						LCD_WriteData(newDegree[0] + '0');
					}
					invalid = 0;
					keypadCount = 0;
					Keypad_state = Keypad_Wait;
				}		
				while(GetKeypadKey()){asm("nop");}		
			}
		break;
		default:
		break;
	}
}
//open and lock door using 2 different buttons, in final design open button will not be implemented.
void Test_Tick(){
	
	/*//testing UART connection with RFID/NFC chip
	unsigned char* string;
	string = NULL;
	string = "Hello";
	for (int i = 0; i < 3; i++){
		while (USART_IsSendReady(1) == 1) {}
		USART_Send(0x01, 1);
		while(USART_HasTransmitted(1)==1){}
		while (USART_IsSendReady(1) == 1) {}
		USART_Send(0x02, 1);
		while(USART_HasTransmitted(1)==1){}
	}
		
	while (USART_IsSendReady(1) == 1) {}
	USART_Send(0x1A, 1);
	while(USART_HasTransmitted(1)==1){}
		
	while(!USART_HasReceived(1)){}
	unsigned char* temp = USART_Receive(1);
	
	LCD_DisplayString(1, temp);
	*/
	button1 = ~PINB & 0x01;
	button2 = ~PINB & 0x02;
	//Transitions
	switch(Test_state){
		case INIT:
				Test_state = Wait_1;  
		case Wait_1:
			if (button1){
				if (currentState != open)
					servo(open, clockdirection, degrees);
				currentState = open;
				Test_state = Reset_1;
				pb = 0x01;			
			}
			else if (button2){
				servo(lock, clockdirection, degrees);
				currentState = lock;
				Test_state = Reset_1;
				pb = 0x02;
			}
		break;
		case Reset_1:
			if((!button1 && !button2)){
				Test_state = Wait_1;
				pb = 0x00;	
			}
		break;
	}
	//PORTB = pb;
	return Test_state;
}

void TestSecTask()
{
	Test_Init();
   for(;;) 
   { 	
	Test_Tick();
	vTaskDelay(10); 
   } 
}
void InitLockTask()
{
	InitLock_Init();
	for(;;)
	{
		InitLockTask_Tick();
		vTaskDelay(25);
	}
}
void PIRTask()
{
	PIR_Init();
	for(;;)
	{
		PIR_Tick();
		vTaskDelay(1000);
	}
}
void PIR_Detect_Task()
{
	PIR_Detect_Init();
	for(;;)
	{
		PIR_Detect_Tick();
		vTaskDelay(10);
	}
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(TestSecTask, (signed portCHAR *)"TestSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(PIR_Detect_Task, (signed portCHAR *)"PIR_Detect_Task", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(PIRTask, (signed portCHAR *)"PIRTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(InitLockTask, (signed portCHAR *)"InitLockTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	
 
 
int main(void) 
{ 
	DDRA = 0xF0;	PORTA = 0x0F;   //keypad
	DDRB = 0x00;	PORTB = 0xFF;	//output
	DDRC = 0xFF;	PORTC = 0x00;	//output LCD
	DDRD = 0xE3;	PORTD = 0x1C;	//output for bluetooth 0-1, input 2-4, output servo 5 LCD 67
	
	//SPI_MasterInit();
	//initBluetooth();
	initUSART(0);
	initUSART(1);
	initServo();
	LCD_init();	
	
   //Start Tasks  
   StartSecPulse(1);
    //RunSchedular 
   vTaskStartScheduler(); 
 
   return 0; 
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initBluetooth(){
		delay_ms(7000);
		char* command = NULL;
		
		//setup both bluetooth modules on 1 atmega using both UART ports
		command = "AT+ROLE=M";
		UART_Send_BlueTooth(command, 0);
		command = "AT+NAME=AlexTest";
		UART_Send_BlueTooth(command, 0);
		command = "AT+PIN8888";
		UART_Send_BlueTooth(command, 0);
		
		command = "AT+ROLE=S";
		UART_Send_BlueTooth(command, 1);
		command = "AT+NAME=AlexTest";
		UART_Send_BlueTooth(command, 1);
		command = "AT+PIN8888";
		UART_Send_BlueTooth(command, 1);
}
//old code, from previous way to determine lock positions unused, possible future use maybe not tho
unsigned char* determineDoorPositions(unsigned char rotation, unsigned int degrees, unsigned char userstart, unsigned char userend){
	static unsigned char pos[2];
	if (rotation == clockwise){
		if (userstart == horizontal && userend == horizontal){
			pos[0] = 0;
			pos[1] = 2;
		}
		else if (userstart == horizontal && userend == vertical){
			if (degrees == 90){
				pos[0] = 0;
				pos[1] = 1;
			}
			else if (degrees == 270){
				pos[0] = 0;
				pos[1] = 3;
			}
			else {
				pos[0] = 0xff;
				pos[1] = 0xff;
			}
		}
		else if (userstart == vertical && userend == vertical){
			pos[0] = 1;
			pos[1] = 3;
		}
		else if (userstart == vertical && userend == horizontal){
			if (degrees == 90){
				pos[0] = 1;
				pos[1] = 2;
			}
			else if (degrees == 270){
				pos[0] = 0;
				pos[1] = 0;
			}
			else {
				pos[0] = 0xff;
				pos[1] = 0xff;
			}
		}
	}
	else if (rotation == counterclockwise){
		if (userstart == horizontal && userend == horizontal){
			pos[0] = 2;
			pos[1] = 0;
		}
		else if (userstart == horizontal && userend == vertical){
			if (degrees == 90){
				pos[0] = 2;
				pos[1] = 1;
			}
			else if (degrees == 270){
				pos[0] = 0;
				pos[1] = 0;
			}
			else {
				pos[0] = 0xff;
				pos[1] = 0xff;
			}
		}
		else if (userstart == vertical && userend == vertical){
			pos[0] = 3;
			pos[1] = 1;
		}
		else if (userstart == vertical && userend == horizontal){
			if (degrees == 90){
				pos[0] = 1;
				pos[1] = 0;
			}
			else if (degrees == 270){
				pos[0] = 3;
				pos[1] = 0;
			}
			else {
				pos[0] = 0xff;
				pos[1] = 0xff;
			}
		}
	}
	return pos;
}

//I never got SPI working to the raspberry pi or to the NFC chip, here for future reference
 unsigned char SPI_Transmit(unsigned char readorwrite, unsigned char address){
	 address = 1<<address;
	 if (readorwrite == 'r')
	 address |= 0x80;
	 //PORTB = PORTB & 0xEF;
	 SPDR = 0x00;//address;
	 while( !(SPSR & (1<<SPIF) ));
	 //PORTB = PORTB | 0x10;
	 return SPDR;
 }
 void SPI_MasterInit(void) {
	 DDRB = (1<<DDB4) | (1<<DDB5) | (1<<DDB7);
	 SPCR = (1<<SPE) |  (1<<MSTR) | (1<<SPR0);// | (1<<SPIE); //| (1<<CPOL);
 }