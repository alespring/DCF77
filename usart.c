/*----------------------------------------------------------------------------
 Copyright:      Radig Ulrich  mailto: mail@ulrichradig.de
 Author:         Radig Ulrich
 Remarks:        
 known Problems: none
 Version:        14.06.2007
 Description:    RS232 Routinen

 Dieses Programm ist freie Software. Sie können es unter den Bedingungen der 
 GNU General Public License, wie von der Free Software Foundation veröffentlicht, 
 weitergeben und/oder modifizieren, entweder gemäß Version 2 der Lizenz oder 
 (nach Ihrer Option) jeder späteren Version. 

 Die Veröffentlichung dieses Programms erfolgt in der Hoffnung, 
 daß es Ihnen von Nutzen sein wird, aber OHNE IRGENDEINE GARANTIE, 
 sogar ohne die implizite Garantie der MARKTREIFE oder der VERWENDBARKEIT 
 FÜR EINEN BESTIMMTEN ZWECK. Details finden Sie in der GNU General Public License. 

 Sie sollten eine Kopie der GNU General Public License zusammen mit diesem 
 Programm erhalten haben. 
 Falls nicht, schreiben Sie an die Free Software Foundation, 
 Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA. 
------------------------------------------------------------------------------*/

#include <stdio.h>
#include "usart.h"

volatile unsigned char buffercounter = 0;
char usart_rx_buffer[BUFFER_SIZE];

//----------------------------------------------------------------------------
//Init serielle Schnittstelle
void usart_init(unsigned int baudrate) 
{ 
  	//Enable TXEN im Register UCR TX-Data Enable
	UCSR0B =(1 << TXEN0);
	//Teiler wird gesetzt 
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
	//UBRR0=(SYSCLK / (baudrate * 16L) - 1);
	UBRR0=51;

}

//----------------------------------------------------------------------------
//Routine für die Serielle Ausgabe eines Zeichens
void usart_write_char(unsigned char c)
{
	if (c == '\n')
		usart_write_char('\r');
	//Warten solange bis Zeichen gesendet wurde
	while(!(UCSR0A & (1<<UDRE0)));
	//Ausgabe des Zeichens
	//UDR0 = toupper(c);
	UDR0 = c;
	for(long a=0;a<2000;a++){asm("nop");}
	return;
}

//------------------------------------------------------------------------------
void usart_write_P (const char *Buffer,...)
{
	va_list ap;
	va_start (ap, Buffer);	
	
	int format_flag;
	char str_buffer[10];
	char str_null_buffer[10];
	char move = 0;
	char Base = 0;
	int tmp = 0;
	char by;
	char *ptr;
		
	//Ausgabe der Zeichen
    for(;;)
	{
		by = pgm_read_byte(Buffer++);
		if(by==0) break; // end of format string
            
		if (by == '%')
		{
            by = pgm_read_byte(Buffer++);
			if (isdigit(by)>0)
				{
                                 
 				str_null_buffer[0] = by;
				str_null_buffer[1] = '\0';
				move = atoi(str_null_buffer);
                by = pgm_read_byte(Buffer++);
				}

			switch (by)
				{
                case 's':
                    ptr = va_arg(ap,char *);
                    while(*ptr) { usart_write_char(*ptr++); }
                    break;
				case 'b':
					Base = 2;
					goto ConversionLoop;
				case 'c':
					//Int to char
					format_flag = va_arg(ap,int);
					usart_write_char (format_flag++);
					break;
				case 'i':
					Base = 10;
					goto ConversionLoop;
				case 'o':
					Base = 8;
					goto ConversionLoop;
				case 'x':
					Base = 16;
					//****************************
					ConversionLoop:
					//****************************
					itoa(va_arg(ap,int),str_buffer,Base);
					int b=0;
					while (str_buffer[b++] != 0){};
					b--;
					if (b<move)
						{
						move -=b;
						for (tmp = 0;tmp<move;tmp++)
							{
							str_null_buffer[tmp] = '0';
							}
						//tmp ++;
						str_null_buffer[tmp] = '\0';
						strcat(str_null_buffer,str_buffer);
						strcpy(str_buffer,str_null_buffer);
						}
					usart_write_str (str_buffer);
					move =0;
					break;
				}
			
			}	
		else
		{
			usart_write_char ( by );	
		}
	}
	va_end(ap);
}

//----------------------------------------------------------------------------
//Ausgabe eines Strings
void usart_write_str(char *str)
{
	while (*str)
	{
		usart_write_char(*str++);
	}
}





