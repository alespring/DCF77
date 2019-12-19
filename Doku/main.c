/*#######################################################################################
AVR DCF77 Clock 

Copyright (C) 2005 Ulrich Radig

#######################################################################################*/

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdint.h>
#include <avr/wdt.h>

#include "clock.h"
#include "usart.h"
#include "lcd.h"

//Schalter
#define TST_UP 		(!(PINC&(1<<PC0)))
#define TST_DOWN 	(!(PINC&(1<<PC1)))
#define TST_main 	(!(PINC&(1<<PC2)))
#define TST_LEFT 	(!(PINC&(1<<PC3)))
#define TST_RIGHT	(!(PINC&(1<<PC4)))
#define TST_SNOOZE	(!(PINC&(1<<PC5)))

//Piepser
#define PIEPSER_ON	PORTA |= (1<<PD7);
#define PIEPSER_OFF	PORTA &= ~(1<<PD7);

unsigned char edgepos;
unsigned char edgeneg;

volatile char piepser_on=0;
volatile unsigned char taktart = 0;
volatile unsigned char takt = 30;
volatile unsigned int alex_timer;


/*
** constant definitions
*/
static const PROGMEM unsigned char copyRightChar[] =
{
	//0x07, 0x08, 0x13, 0x14, 0x14, 0x13, 0x08, 0x07,
	0x05, 0x0a, 0x14, 0x14, 0x14, 0x0a, 0x05, 0x00,
	0x00, 0x04, 0x0e, 0x0e, 0x1f, 0x1f, 0x04, 0x00,
	0x14, 0x0a, 0x05, 0x05, 0x05, 0x0a, 0x14, 0x00,
	0x08, 0x04, 0x02, 0x01, 0x01, 0x02, 0x04, 0x08,
	0x00, 0x00, 0x15, 0x15, 0xff, 0xff, 0x00, 0x00
	//0xFF, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0xFF
};

//static const PROGMEM unsigned char *day_name[]=
 unsigned char *weekday_name[]=
{
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday",
  "Sunday"
};

//static const PROGMEM unsigned char *month_name[]=
 unsigned char *month_name[]=
{
  "Jan",
  "Feb",
  "Mar",
  "Apr",
  "Mai",
  "Jun",
  "Jul",
  "Aug",
  "Sep",
  "Okt",
  "Nov",
  "Dez"
};

unsigned char *einstellungen[]=
{
  "zurück",
  "Alarm",
  "zurück",
  "zeiteinstellung"
};

struct time_anzeige
{
	int mm2;
	int hh2;
	int alarmset;
};

FILE suart_stream = FDEV_SETUP_STREAM(usart_write_char, NULL, _FDEV_SETUP_WRITE );

//FILE suart_stream = FDEV_SETUP_STREAM(lcd_putc, NULL, _FDEV_SETUP_WRITE );
static void stdio_start( void ) 
{
	stdout = &suart_stream;
	//	printf_P( PSTR("This output done with printf_P\n") );
}


unsigned char act = 0;
unsigned char alt = 0;
	
	
void edgetriggerung()
{
	
	unsigned char edge;
	
	
	act = ~PINC;
	edge = act ^ alt;
	edgepos = act & edge;
	edgeneg = edge & !act;
	alt = act;
}

unsigned char e= 0;

//Weckzeit einstellung
void wecker_einstelungen(struct time_anzeige *ts)
{
	char string_wecker[18];
	unsigned char tim = 1;
	unsigned char s1 = 0;
	unsigned char s2 = 0;
	unsigned char m2 = ts->mm2%10;
	unsigned char m1 = ts->mm2/10;
	unsigned char h2 = ts->hh2%10;
	unsigned char h1 = ts->hh2/10;
	unsigned char ss2 = 0;
	unsigned char mm2 = 0;
	unsigned char hh2 = 0;
	
while(1)
{
	wdt_reset();
	switch(tim)
	{
		case 1://einerzahlen minuten eingabe
			lcd_gotoxy(8,2);
			lcd_putc('v');
			
			if(TST_UP 	&& (edgepos  & 0x1)){lcd_clrscr();m2+=1;}//raufzählen
			if(TST_DOWN && (edgepos  & 0x2)){lcd_clrscr();m2-=1;}//runterzählen
			if(TST_LEFT && (edgepos & 0x8)){lcd_clrscr();tim = 2;}//zehnerzahlen minuten einstellen
			
			if(m2 > 9){m2 = 0;}
			
			sprintf(string_wecker, "%01d%01d:%01d%01d", h1,h2,m1,m2);
			lcd_gotoxy(4,3);
			lcd_puts(string_wecker);
			
			if(TST_main && (edgepos  & 0x4)){lcd_clrscr();tim = 5;}//Weckzeit bestätigen
		break;
		case 2://zehnerzahlen minuten eingabe
			lcd_gotoxy(7,2);
			lcd_putc('v');
			
			if(TST_UP 	&& (edgepos  & 0x1)){lcd_clrscr();m1+=1;}//raufzählen
			if(TST_DOWN && (edgepos  & 0x2)){lcd_clrscr();m1-=1;}//runterzählen
			if(TST_LEFT && (edgepos & 0x8)){lcd_clrscr();tim = 3;}//einerzahlen stunden einstellen
			if(TST_RIGHT && (edgepos & 0x10)){lcd_clrscr();tim = 1;}//einerzahlen minuten einstellen
			
			if(m1 > 5){m1 = 0;}
			
			sprintf(string_wecker, "%01d%01d:%01d%01d", h1,h2,m1,m2);
			lcd_gotoxy(4,3);
			lcd_puts(string_wecker);
			
			if(TST_main && (edgepos  & 0x4)){lcd_clrscr();tim = 5;}//Weckzeit bestätigen
		break;
		case 3://einerzahlen stunden eingabe
			lcd_gotoxy(5,2);
			lcd_putc('v');
			
			if(TST_UP 	&& (edgepos  & 0x1)){lcd_clrscr();h2+=1;}//raufzählen
			if(TST_DOWN && (edgepos  & 0x2)){lcd_clrscr();h2-=1;}//runterzählen
			if(TST_LEFT && (edgepos & 0x8)){lcd_clrscr();tim = 4;}//zehnerzahlen stunden einstellen
			if(TST_RIGHT && (edgepos & 0x10)){lcd_clrscr();tim = 2;}//zehnerzahlen minuten einstellen
			
			if(h2 > 9){h2 = 0;}
			
			sprintf(string_wecker, "%01d%01d:%01d%01d", h1,h2,m1,m2);
			lcd_gotoxy(4,3);
			lcd_puts(string_wecker);
			
			if(TST_main && (edgepos  & 0x4)){lcd_clrscr();tim = 5;}//Weckzeit bestätigen
		break;
		case 4://zehnerzahlen stunden eingabe
			lcd_gotoxy(4,2);
			lcd_putc('v');
			
			if(TST_UP 	&& (edgepos  & 0x1)){lcd_clrscr();h1+=1;}//raufzählen
			if(TST_DOWN && (edgepos  & 0x2)){lcd_clrscr();h1-=1;}//runterzählen
			if(TST_RIGHT && (edgepos & 0x10)){lcd_clrscr();tim = 3;}//einerzahlen stunden einstellen
			
			if(h1 > 5){h1 = 0;}
			
			sprintf(string_wecker, "%01d%01d:%01d%01d", h1,h2,m1,m2);
			lcd_gotoxy(4,3);
			lcd_puts(string_wecker);
			
			if(TST_main && (edgepos  & 0x4)){lcd_clrscr();tim = 5;}//Weckzeit bestätigen
		break;
		case 5://zeit umwandlung und definitive bestaetigung
			//ts->ss2 = (s1*10)+s2;
			ts->mm2 = (m1*10)+m2;
			ts->hh2 = (h1*10)+h2;
			sprintf(string_wecker, "%02d:%02d",ts->hh2,ts->mm2);
			lcd_gotoxy(4,3);
			lcd_puts(string_wecker);
			
			
			if((TST_SNOOZE && (edgepos  & 0x20))){lcd_clrscr(); e=0;tim = 1; eeprom_write_block( ts,0x10 ,sizeof(struct time_anzeige)) ; return; }//lcd_gotoxy(16,0);putc(2);
		break;
	}
	edgetriggerung();
  }
}

/*void month()
{
	switch(mon)
	{
		case 01:
			m = Jan;
		break;
		case 02:
			m = Feb;
		break;
	}
}*/


volatile static char piepser_cnt=0;

SIGNAL(SIG_OUTPUT_COMPARE0A)
{
	//PORTA |= (1<<PD7)
	



 if(piepser_cnt >= takt) {
  if(piepser_on == 1)	
  PORTA^=(1<<PD7);
   else 
    PIEPSER_OFF 
   piepser_cnt=0;
  }
  else
  piepser_cnt++;
 
	
}

//############################################################################
//Hauptprogramm
int main (void)
//############################################################################
{
	DDRC = 0x00; // alle Port_D auf input gesetzt 
 	PINC = 0xFF; // alle Port_c auf high gesetzt

	DDRD=0x02;
	//Inizialisierung der Seriellen Schnittstelle
	usart_init(9600);
	DDRA = 0xFF;
	PORTA = 0x00;
	
	TCCR0A = 0x02;
	TCCR0B = 0x05;
	OCR0A = 200;
	TIMSK0 = 0x02;
	alex_timer=0;
	piepser_cnt=0;
	wdt_enable(WDTO_8S);
	
/* initialize display, cursor off */
    lcd_init(LCD_DISP_ON);

                            /* loop forever */
        /* 
         * Test 1:  write text to display
         */

        /* clear display and home cursor */
        lcd_clrscr();

	stdio_start();
	//Globale Interrupts einschalten
	sei();

	//Starten der DCF77 Uhr
	Start_Clock();
	//LCD_Clear();
	usart_write ("\nDCF 77 Clock by Ulrich Radig\n\n");
	
	printf_P(PSTR("Hallo Echo\n"));
    
	//Ausgabe der Zeit auf der Seriellen Schnittstelle in einer Endlosschleife
	
	
	 lcd_clrscr();   /* clear display home cursor */
       
      // lcd_puts("Copyright: ");
       
       /*
        * load two userdefined characters from program memory
        * into LCD controller CG RAM location 0 and 1
        */
     lcd_command(_BV(LCD_CGRAM));  /* set CG RAM start address 0 */
			for(int i=0; i<64; i++)
			{
				lcd_data(pgm_read_byte_near(&copyRightChar[i]));
			}
			
	struct time_anzeige wecker1_ts;
	char string_time[18];
	char string_date[18];
	char string_weekday[18];
	char string_einstellung[5];
	char string_wecker[5];
	unsigned char a = 0;
	unsigned char p1 = 0;
	unsigned char p2 = 0;
	unsigned char t = 0;
	unsigned char x = 0;
	int ax = 0;
	unsigned char stufe = 0;
	unsigned char tempo = 0;
	
	eeprom_read_block(&wecker1_ts,0x10,sizeof(struct time_anzeige));
	
	while (1)
	{
		wdt_reset();
		edgetriggerung();
		switch(e)
		{
			case 0://Hauptbildschirm
				sprintf(string_weekday, "%s", weekday_name[Weekday -1]);//Wochentag
				lcd_gotoxy(0,0);
				lcd_puts(string_weekday);
				
				sprintf(string_date, "%02d. %s %04d", day,month_name[mon -1],year);//Datum
				lcd_gotoxy(2,1);
				lcd_puts(string_date);
				
				
				sprintf(string_time, "%02d:%02d:%02d", hh,mm,ss);//Zeit
				lcd_gotoxy(4,3);
				lcd_puts(string_time);
				
				/*USART
				//printf_P(PSTR(" %s:%d e: %d  TST_main %x  Endgepos %x %x\r"),__FUNCTION__,__LINE__,e,(int) TST_main,(int)edgepos,(int)PINC) ;
				*/
				
				if((day == 12)&&((mon) == 8))	//Geburtstag
				{
					lcd_gotoxy(7,2);
					lcd_putc(4);
				}
				
				if((wecker1_ts.alarmset == 1)||(wecker1_ts.alarmset == 2))	//Alarm
				{
					lcd_gotoxy(13,0);
					lcd_putc(1);
				}
				
				//Menuauswahl
				if((wecker1_ts.alarmset == 2)&&(alex_timer == 120 )){lcd_clrscr();if(taktart > 3){taktart = 3;}taktart +=1;e=6;}
				if((mm == wecker1_ts.mm2)&&(hh == wecker1_ts.hh2)&&(wecker1_ts.alarmset == 1)){lcd_clrscr();e=6;}
				
				if(TST_main && (edgepos  & 0x4)){lcd_clrscr(); e = 1;a=0;p1 = 0;}
				
				/*USART
				//printf_P(PSTR(" %s:%d e: %d  \r"),__FUNCTION__,__LINE__,e) ;
				*/
				
			break;
			
			case 1://Alarm menu
				
				printf_P(PSTR(" %s:%d e: %d  \r"),__FUNCTION__,__LINE__,e) ;
				lcd_gotoxy(1,0);
				lcd_puts("Zuruck");
				
				lcd_gotoxy(1,1);
				lcd_puts("Alarm");
				
				lcd_gotoxy(0,p1);
				lcd_putc('>');
				
				//Menuauswahl
				if(TST_UP  && (edgepos  & 0x1)){lcd_clrscr(); a = 0; p1 = 0;}
				if(TST_DOWN && (edgepos  & 0x2)){lcd_clrscr(); a = 2; p1 = 1;}
				if(TST_main && (edgepos  & 0x4)){lcd_clrscr(); e = a;a=0;p1 = 0;}
				/*USART
				//printf_P(PSTR(" %s:%d e: %d  TST_main %x  Endgepos %x %x\r"),__FUNCTION__,__LINE__,e,(int) TST_main,(int)edgepos,(int)PINC) ;
				*/
				
			break;
			
			case 2://Alarm einstellungen
				
				lcd_gotoxy(1,0);
				lcd_puts("Zuruck");
				
				lcd_gotoxy(1,1);
				lcd_puts("zeiteinstellung");
				
				lcd_gotoxy(1,2);
				lcd_puts("Wecker aktivieren");
				
				lcd_gotoxy(1,3);
				lcd_puts("Wecker deaktivieren");
				
				lcd_gotoxy(0,p1);
				lcd_putc('>');
				
				//Menuauswahl
				if(p1>3){lcd_clrscr();p1 = 0;}
				if(TST_UP 	&& (edgepos  & 0x1)){lcd_clrscr();p1 -= 1;}
				if(TST_DOWN && (edgepos  & 0x2)){lcd_clrscr();p1 += 1;}
				if(TST_main && (edgepos  & 0x4))
				{
					lcd_clrscr();
					if(p1 == 0)a=1;
					if(p1== 1)a=3;
					if(p1== 2)a=4;
					if(p1==3)a=5;
					e=a;
					p1 = 0;
				}
			break;
			
			case 3://weckerzeiteinstellung
				wecker_einstelungen(&wecker1_ts);
			break;
			
			case 4://Alarm aktivierung
				sprintf(string_wecker, "%01d", wecker1_ts.alarmset);
				lcd_gotoxy(4,3);
				lcd_puts(string_wecker);
				
				//Menuauswahl
				if(TST_main && (edgepos  & 0x4)){wecker1_ts.alarmset = 1;eeprom_write_block( &wecker1_ts,0x10 ,sizeof(struct time_anzeige)) ;}
				if(TST_SNOOZE && (edgepos  & 0x20))e=0;
			break;
			
			case 5://Alarm deaktivierung
				sprintf(string_wecker, "%01d", wecker1_ts.alarmset);
				lcd_gotoxy(4,3);
				lcd_puts(string_wecker);
				taktart = 1;
				
				//Menuauswahl
				if(TST_main && (edgepos  & 0x4)){wecker1_ts.alarmset = 0;eeprom_write_block( &wecker1_ts,0x10 ,sizeof(struct time_anzeige)) ;}
				if(TST_SNOOZE && (edgepos  & 0x20))e=0;
			break;
			
			case 6://Wecker
				switch((ss/10) )	//Alarmtakt
						{
							case 0:		takt=30;
									break;
												
							case 1: 	takt=15;
									break;
	
							case 2: 	takt=10;
									break;
	
							case 3: 	takt=5;
									break;

							case 4:     	takt=1;
									break;

							default:	break;
						}
				
				//Happy Birthday
				if((day == 13)&&((mon) == 11))
				{
					lcd_gotoxy(2,2);
					lcd_puts("Happy Birthday");
				}else{//Alarm
					lcd_gotoxy(5,2);
					lcd_puts("Alarm");
				}
				
				piepser_on=1;				//Alarm	
				alex_timer=0;				//Timer zurücksetzen
				wecker1_ts.alarmset = 2;	//Alarm wiederholen Befehl
				if(TST_SNOOZE && (edgepos  & 0x20)){piepser_on=0;e = 0;}
			break;
		}
		
		printf_P(PSTR(" %s:%d e: %d  TST_main %x  Endgepos %x %x a %d\r"),__FUNCTION__,__LINE__,e,(int) TST_main,(int)edgepos,(int)PINC,a) ;
	    
		/*USART
		//printf_P(PSTR("%i-%i-%i Time: %i:%i:%i Sync: %i  Rx: %i    RX_CNT: %d   \r"),
		//		day,mon,year,hh,mm,ss,flags.dcf_sync,flags.dcf_rx, rx_bit_counter);
		*/
		//Wait a schort time
		if(x >= 100)
		{
			x = 0;
			lcd_clrscr();
		}else{
			x++;
		}
		for (long i=0;i<20000;i++){};
	}
return (1);
}

