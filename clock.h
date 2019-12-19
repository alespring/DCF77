/*#######################################################################################
AVR DCF77 Clock

Copyright (C) 2005 Ulrich Radig

#######################################################################################*/

#ifndef _CLOCK_H
 #define _CLOCK_H

volatile unsigned char ss;	//Globale Variable für Sekunden
volatile unsigned char mm;	//Globale Variable für Minuten
volatile unsigned char hh;	//Globale Variable für Stunden
volatile unsigned char day;	//Globale Variable für den Tag
volatile unsigned char Weekday;	//Globale Variable für den Wochentag
volatile unsigned char mon;	//Globale Variable für den Monat
volatile unsigned int year;	//Globale Variable für den Jahr

extern void Start_Clock (void); //Startet die DCF77 Uhr
extern void Add_one_Second (void);

//64 Bit für DCF77 benötigt werden 59 Bits
volatile unsigned long long dcf_rx_buffer;
//RX Pointer (Counter)
volatile extern unsigned char rx_bit_counter;
//Hilfs Sekunden Counter
volatile unsigned int h_ss;


extern volatile unsigned int alex_timer;

#if defined (__AVR_ATmega128__)
	//Interrupt an dem das DCF77 Modul hängt hier INT0
	#define DCF77_INT_ENABLE()	EIMSK |= (1<<INT0);
	#define DCF77_INT			SIG_INTERRUPT0
	#define INT0_CONTROL		EICRA
	#define INT0_FALLING_EDGE	0x02
	#define INT0_RISING_EDGE	0x03
	#define TIMSK1 				TIMSK
#endif

#if defined (__AVR_ATmega32__)
	//Interrupt an dem das DCF77 Modul hängt hier INT0
	#define DCF77_INT_ENABLE()	GICR |= (1<<INT0);
	#define DCF77_INT			SIG_INTERRUPT0
	#define INT0_CONTROL		MCUCR
	#define INT0_FALLING_EDGE	0x02
	#define INT0_RISING_EDGE	0x03
	#define TIMSK1 				TIMSK
#endif

#if defined (__AVR_ATmega324P__)
	//Interrupt an dem das DCF77 Modul hängt hier INT0
	#define DCF77_INT_ENABLE()	EIMSK |= (1<<INT0);
	#define DCF77_INT			SIG_INTERRUPT0
	#define INT0_CONTROL		EICRA
	#define INT0_FALLING_EDGE	0x02
	#define INT0_RISING_EDGE	0x03
	#define TIMSK1 				TIMSK
#endif

#if defined (__AVR_ATmega8__)
	//Interrupt an dem das DCF77 Modul hängt hier INT0
	#define DCF77_INT_ENABLE()	GICR |= (1<<INT0);
	#define DCF77_INT			SIG_INTERRUPT0
	#define INT0_CONTROL		MCUCR
	#define INT0_FALLING_EDGE	0x02
	#define INT0_RISING_EDGE	0x03
    #define TIMSK1              TIMSK
#endif

#if defined (__AVR_ATmega88__)
	//Interrupt an dem das DCF77 Modul hängt hier INT0
	#define DCF77_INT_ENABLE()	EIMSK |= (1<<INT0);
	#define DCF77_INT			SIG_INTERRUPT0
	#define INT0_CONTROL		EICRA
	#define INT0_FALLING_EDGE	0x02
	#define INT0_RISING_EDGE	0x03
#endif

//Structur des dcf_rx_buffer
struct  DCF77_Bits {
	unsigned char M			    :1	;
	unsigned char O1			:1	;
	unsigned char O2			:1	;
	unsigned char O3			:1	;
	unsigned char O4			:1	;
	unsigned char O5			:1	;
	unsigned char O6			:1	;
	unsigned char O7			:1	;
	unsigned char O8			:1	;
	unsigned char O9			:1	;
	unsigned char O10			:1	;
	unsigned char O11			:1	;
	unsigned char O12			:1	;
	unsigned char O13			:1	;
	unsigned char O14			:1	;
	unsigned char R			    :1	;
	unsigned char A1			:1	;
	unsigned char Z1			:1	;
	unsigned char Z2			:1	;
	unsigned char A2			:1	;
	unsigned char S			    :1	;
	unsigned char Min			:7	;//7 Bits für die Minuten
	unsigned char P1			:1	;//Parity Minuten
	unsigned char Hour			:6	;//6 Bits für die Stunden
	unsigned char P2			:1	;//Parity Stunden
	unsigned char Day			:6	;//6 Bits für den Tag
	unsigned char Weekday		:3	;//3 Bits für den Wochentag 
	unsigned char Month		    :5	;//3 Bits für den Monat
	unsigned char Year		    :8	;//8 Bits für das Jahr **eine 5 für das Jahr 2005**
	unsigned char P3			:1	;//Parity von P2
	};
	
struct 
	{
	volatile char parity_err					:1	;//Hilfs Parity
	volatile char parity_P1					:1	;//Berechnetes Parity P1
	volatile char parity_P2					:1	;//Berechnetes Parity P2
	volatile char parity_P3					:1	;//Berechnetes Parity P3
	volatile char dcf_rx					:1	;//Es wurde ein Impuls empfangen
	volatile char dcf_sync					:1	;//In der letzten Minuten wurde die Uhr syncronisiert
	}flags;
	
#endif //_CLOCK_H
