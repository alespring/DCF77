/*#######################################################################################
AVR DCF77 Clock 

Copyright (C) 2005 Ulrich Radig

#######################################################################################*/

#include "clock.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include "usart.h"

//Die Uhrzeit seht in folgenden Variablen
volatile unsigned char ss   = 0;   //Globale Variable für die Sekunden
volatile unsigned char mm   = 0;   //Globale Variable für die Minuten
volatile unsigned char hh   = 0;   //Globale Variable für die Stunden
volatile unsigned char day   = 0;   //Globale Variable für den Tag
volatile unsigned char Weekday   = 0;   //Globale Variable für den Wochentag
volatile unsigned char mon   = 0;   //Globale Variable für den Monat
volatile unsigned int year   = 0;   //Globale Variable für das Jahr

//Bitzähler für RX Bit
volatile unsigned char rx_bit_counter = 0;

//64 Bit für DCF77 benötigt werden 59 Bits
volatile unsigned long long dcf_rx_buffer = 0;

//Hilfs Sekunden Counter
volatile unsigned int h_ss = 0;

//Hilfs Variable für Stundenwechsel
volatile unsigned int h_hh = 0;

//############################################################################
//Overflow Interrupt wird ausgelöst bei 59Sekunde oder fehlenden DCF77 Signal 
SIGNAL (SIG_OVERFLOW1)
//############################################################################

{   
   struct  DCF77_Bits *rx_buffer;
   rx_buffer = (struct DCF77_Bits*)(char*)&dcf_rx_buffer;
      
   //Zurücksetzen des Timers
   TCNT1 = 65535 - (SYSCLK / 1024);
   //wurden alle 59 Bits empfangen und sind die Paritys richtig?
   if (rx_bit_counter == 59 && 
      flags.parity_P1 == rx_buffer->P1 && 
      flags.parity_P2 == rx_buffer->P2 &&
      flags.parity_P3 == rx_buffer->P3)
      //Alle 59Bits empfangen stellen der Uhr nach DCF77 Buffer
      {
      //Berechnung der Minuten BCD to HEX
      mm = rx_buffer->Min-((rx_buffer->Min/16)*6);
        
        if (mm != 0){mm--;}else{mm = 59; h_hh = 1;};
      
        //Berechnung der Stunden BCD to HEX
      hh = rx_buffer->Hour-((rx_buffer->Hour/16)*6);

      if (h_hh) {hh--;h_hh = 0;};

      //Berechnung des Tages BCD to HEX
      day= rx_buffer->Day-((rx_buffer->Day/16)*6); 
	  //Berechnung des Wochentages BCD to HEX
      Weekday= rx_buffer->Weekday;
      //Berechnung des Monats BCD to HEX
      mon= rx_buffer->Month-((rx_buffer->Month/16)*6);
      //Berechnung des Jahres BCD to HEX
      year= 2000 + rx_buffer->Year-((rx_buffer->Year/16)*6);
      //Sekunden werden auf 0 zurückgesetzt
      ss = 59;
      flags.dcf_sync = 1;
      }
   else
      //nicht alle 59Bits empfangen bzw kein DCF77 Signal Uhr läuft 
      //manuell weiter
      {
      Add_one_Second();
      flags.dcf_sync = 0;
      }
   //zurücksetzen des RX Bit Counters
   rx_bit_counter = 0;
   //Löschen des Rx Buffers
   dcf_rx_buffer = 0;
};

//############################################################################
//DCF77 Modul empfängt Träger 
SIGNAL (DCF77_INT)
//############################################################################
{
   //Auswertung der Pulseweite 
   if (INT0_CONTROL == INT0_RISING_EDGE)
      {
      flags.dcf_rx ^= 1;
      //Secunden Hilfs Counter berechnen // SYSCLK defined in USART.H
      h_ss = h_ss + TCNT1 - (65535 - (SYSCLK / 1024));
      //Zurücksetzen des Timers
      TCNT1 = 65535 - (SYSCLK / 1024);
      //ist eine Secunde verstrichen // SYSCLK defined in USART.H
      if (h_ss > (SYSCLK / 1024 / 100 * 90)) //90% von 1Sekunde
         {
         //Addiere +1 zu Sekunden
         Add_one_Second();
         //Zurücksetzen des Hilfs Counters
         h_ss = 0;
         };
      //Nächster Interrupt wird ausgelöst bei abfallender Flanke
      INT0_CONTROL = INT0_FALLING_EDGE;
      }
   else
      {
      //Auslesen der Pulsweite von ansteigender Flanke zu abfallender Flanke
      unsigned int pulse_wide = TCNT1;
      //Zurücksetzen des Timers
      TCNT1 = 65535 - (SYSCLK / 1024);
      //Secunden Hilfs Counter berechnen
      h_ss = h_ss + pulse_wide - (65535 - (SYSCLK / 1024));
      //Parity speichern
      //beginn von Bereich P1/P2/P3
      if (rx_bit_counter ==  21 || rx_bit_counter ==  29 || rx_bit_counter ==  36) 
         {
         flags.parity_err = 0;
         };
      //Speichern von P1
      if (rx_bit_counter ==  28) {flags.parity_P1 = flags.parity_err;};
      //Speichern von P2
      if (rx_bit_counter ==  35) {flags.parity_P2 = flags.parity_err;};
      //Speichern von P3
      if (rx_bit_counter ==  58) {flags.parity_P3 = flags.parity_err;};
      //Überprüfen ob eine 0 oder eine 1 empfangen wurde
      //0 = 100ms
      //1 = 200ms
      //Abfrage größer als 150ms (15% von 1Sekund also 150ms)   
      if (pulse_wide > (65535 - (SYSCLK / 1024)/100*85))
         {
         //Schreiben einer 1 im dcf_rx_buffer an der Bitstelle rx_bit_counter
         dcf_rx_buffer = dcf_rx_buffer | ((unsigned long long) 1 << rx_bit_counter);
         //Toggel Hilfs Parity
         flags.parity_err = flags.parity_err ^ 1;
         }
      //Nächster Interrupt wird ausgelöst bei ansteigender Flanke
      INT0_CONTROL = INT0_RISING_EDGE;
      //RX Bit Counter wird um 1 incrementiert
      rx_bit_counter++;
      }
};

//############################################################################
//Addiert 1 Sekunde
void Add_one_Second (void)
//############################################################################
{
   ss++;//Addiere +1 zu Sekunden
   if (ss == 60)
   {
      ss = 0;
      mm++;//Addiere +1 zu Minuten
      if (mm == 60)
      {
         mm = 0;
         hh++;//Addiere +1 zu Stunden
         if (hh == 24)
         {
            hh = 0;
         }
      }
   }
   alex_timer++;
};

//############################################################################
//Diese Routine startet und inizialisiert den Timer
void Start_Clock (void)
//############################################################################
{
   //Interrupt DCF77 einschalten auf ansteigende Flanke
   DCF77_INT_ENABLE();
   INT0_CONTROL = INT0_RISING_EDGE;
      
   //Interrupt Overfolw enable
   TIMSK1 |= (1 << TOIE1);
   //Setzen des Prescaler auf 1024 
   TCCR1B |= (1<<CS10 | 0<<CS11 | 1<<CS12); 
    //SYSCLK defined in USART.H
   TCNT1 = 65535 - (SYSCLK / 1024);
   return;
};



