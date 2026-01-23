/********************************************************************************
Navigationslaterne S60, S61, S66 
- Dimmbar über DALI

FIRMWARE     : Navigationslaterne S60, S61, S66... - Dimmbar über DALI                                         
FIRMWARE-Nr. : 7366500802                                       
Rev          : 1, xxxxx, ÄM 24195                                                 
ORT+DATUM    : Bremen, 18.05.2021                                        
AUTOR        : Brand                                
COMPILER     : PIC C-Compiler CCS V5.096

---------------------------------------------------------------------------------
Revision:  ursprünglich 7366500800rev4
			  
rev0 :     Brand, 28.01.2019
                     
           Blinkfunktionen :  als feste szene im EEPROM hinterlegen und 
                              dann vom controller entsprechend starten oder parametriert
                              -> es soll KEINE DIMMUNG bei Blinkfunktion möglich sein!!!! -> MOMs PG S66N GL
										
           Heizungsteuerung : 1. Temperaturmessungen an der Elektronik wenn Gehäuse
                                 mit Heizung vorhanden
                              2. Das Einschalten der Heizung soll durch PWM erfolgen für 2s 
                                 "Langsames einschalten" (Serfass)
                                 Dafür wird CCP2 und CCP4 benutzt. Statt AN12 wird AN10 benutzt.

rev1 :     Brand, 29.06.2021

           - Dimmung nun auch bei fester Sequenz/Blinkfunktion möglich 
             (Änderung wegen AOPS). Ein "springen" der Intensität(5ms) sowie Aussetzer
             im Blinktakt sind weiterhin nicht auszuschliessen! 
             
           - Fehler bei Auswertung Kurz- und Gruppenadresse behoben
             (Kurzadresse hat auch auf Gruppenadresse reagiert)
            
           - Statusantwort meldet im Blinkmodus durch Sequenz immer AN (flag.blink),
             anstatt abhängig von Converter AN/AUS
---------------------------------------------------------------------------------

PIC18F25K22

I/O's:
RA0   Analog IN 0 -> Kreis 1
RA1   Analog IN 1 -> Kreis 2
RA2   Analog IN 2 -> Kreis 3
RA3   Analog IN 3 -> Netzstrommessung
RA4   Digi Out    -> LED Controller läuft 
RA5   Digi Out    -> 24v Fuse Killer
RA6   
RA7   Digi Out    -> Wandler Ein/Aus

RB0   CCP4 -> Heizung halbe Leistung                                 
RB1   Analog IN 10 -> Ausgangsspannung Wandler (Leerlauferkennung)   
RB2   Analog IN 8  -> Temperatursensor
RB3   Digi Out -> DALI SND
RB4   Digi In  -> DALI RCV    (Interrupt on change)
RB5   Digi Out -> Pulsen 
RB6   PGC
RB7   PGD

RC0   Digi Out -> 
RC1   CCP2 -> Heizung volle Leistung                                 
RC2   Digi Out -> SDA  (I²C) Digitalpoti
RC3   Digi Out -> SCL  (I²C) Digitalpoti
RC4   Digi OUT -> Pulsen 
RC5   Digi IN -> Testmodus
RC6   CCP3 / PWM Out 0-5V 
RC7   

RE3   MCLR

EEPROM:  0x00 - 0x0F   Version_No
         0x10          First start?
         0x11          Number of circuits
         0x12          Status       Bit 0: Laterne AN/AUS, 1: Gedimmter Zustand, 2: EOL, 
                                        3: LED failure (kill), 4: circuit failure
         0x13          Digi Poti Value / Parameter
         0x14          DALI Address
         0x15          DALI Group 0-7
         0x16          DALI Group 8-15
         0x30 - 0x43   5x 4Byte for Operation hour counting (due to byte endurance of EEPROM)
                          
Timer0:  100ms Takt
Timer1:  Timing für DALI
Timer2:  von use pwm verwendet -> Dimmung
Timer3:  Blinktakt erzeugen
Timer4:  von use pwm verwendet -> Heizung halbe Leistung
Timer5:  n.v.
Timer6:  von use pwm verwendet -> Heizung volle Leistung
********************************************************************************/
#define VERSION_NO "7366500802rev1"

#include <18F25K22.h>

#fuses INTRC_IO,WDT,PUT,BROWNOUT,BORV29,noLVP,noCPB,noCPD,noWRT,noWRTB,noWRTC,noWRTD,noPLLEN,CCP3C6
#ifdef DEBUG   
  #device ICD=TRUE                // ICD im Debug Modus aktivieren, -> Fuses nicht aktiv!  
#endif

#device ADC=8                     // 8-Bit ADC

#use delay(internal=8M)           // µC interner Takt / Zeitbasis für Delay (ohne WDT-Reset !)
#use fast_IO(ALL)                 // TRIS-Register manuell verwalten !

#use pwm(stream=pwm_out, output=pin_c6, timer=2, frequency=500, duty=100, disable_level=high)   // Pin C6 -> CCP3

#use pwm(stream=heater_half, output=pin_b0, timer=4, frequency=500, duty=100, disable_level=high) // Pin B0 -> CCP4
#use pwm(stream=heater_full, output=pin_c1, timer=6, frequency=500, duty=100, disable_level=high) // Pin C1 -> CCP2

#use I2C(master, stream=poti, sda=PIN_C2, scl=PIN_C3, fast=200000)        // fast=200000

#rom getenv("EEPROM_ADDRESS") = {VERSION_NO}                              // ROM erste zeile     
#rom getenv("EEPROM_ADDRESS") + 0x10 = {                                  
                                        0,0,0,0,0xFF,0,0,0,0,0,0,0,0,0,0,0,     
                                        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  //  0x00,0x5B,0x8D,0x6C,0,0,0,0,0,0,0,0,0,0,0,0, //EOL in 20min
                                        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
                                       }
                                                                                          
#hexcomment\Version: VERSION_NO

#priority int_timer1, int_rb, int_timer0

#include "defvar.h"                            // defines and variables
#include "func.h"                              // functions and prototypes
#include "dali_table.h" 
#include "dalicmd.h"                           // dali defines
#include "dali.h"                              // dali functions, variables and prototypes
#include "test.h"
/*=============================================================================
                                INTERRUPTS
=============================================================================*/
#INT_TIMER0
void timer0_isr(void)
{//1
  set_timer0(15535);                           // start timer 100ms
  flag.ms100 = 1;                              // 100ms IR flag
  daliflag.ms100 = 1;
  bCount500ms--;
  bCount1000ms--;
  if(bCountPulse)
    bCountPulse--;
  if(!bCountPulse)
    output_low(xCurrentPulse);       // switch current pulse OFF
  if(!bCount500ms)
  {//2
    bCount500ms = 5;
    flag.ms500 = 1;  
  }//2
  if(!bCount1000ms)
  {//2
    bCount1000ms = 10;
    flag.ms1000 = 1;
    if(flag.pulse)
    {
      output_high(xCurrentPulse);       // switch current pulse ON
      bCountPulse = PULSE_TIME;
    }
  }//2
}//1 

#INT_TIMER1
void timer1_isr(void)
{//1 
  if(bDaliMode == DALI_RCV) 
    set_timer1(65535-806);                      // -27µs dann passt der timer!?
  if(bDaliMode == DALI_SND)
  {//2  
    set_timer1(65535-380);                      // reset timer
    bDaliSndClk ^= 1<<0;                        // Toggle bit every interrupt
  }//2

  if(bDaliMode == DALI_RCV && daliflag.ioc)     // war vor timer ablauf ein ioc?  
  {//2                                          // ja, alles OK                                                  
    bDaliBit = PORTB & 0x10;                    // get actual port value    
    daliflag.ioc = 0;   
    bDaliBitCnt--;                          
    if(bDaliBit)
      bit_set(wDaliRcv, bDaliBitCnt); 
    if(bDaliBitCnt == 0)                        // komplett Empfangen?
    {//3
      bDaliMode = DALI_DAT;                     // daten komplett vorhanden     
      disable_interrupts(INT_TIMER1);
      enable_interrupts(int_timer0);
      output_low(pin_b3);
    }//3
  }//2
  else 
  if(bDaliMode == DALI_RCV)
  {//2                                             
    bDaliMode = DALI_STBY;                       
    disable_interrupts(INT_TIMER1);
    enable_interrupts(int_timer0);
  }//2 

  if(bDaliMode == DALI_SND)               
  {//2
    if(bDaliSndCnt == 255)
    {//3
      output_low(pin_b3);
      bDaliMode = DALI_STBY;
      disable_interrupts(INT_TIMER1);
      enable_interrupts(int_timer0);
    }//3
    if(bDaliSndCnt == 8)
    {//3
      if(bDaliSndClk)
        output_high(pin_b3);
      else
      {//4
        output_low(pin_b3);
        bDaliSndCnt--; 
      }//4
    }//3
    else
    if(bDaliSndCnt < 8)
    {//3
      if(bDaliSndClk && bit_test(bDaliSndBuff, bDaliSndCnt))
        output_high(pin_b3);
      else
      if(bDaliSndClk)
        output_low(pin_b3);
      
      if(!bDaliSndClk)
      {//4
        output_toggle(pin_b3);
        bDaliSndCnt--;
      }//4     
    }//3
  }//2
}//1

#INT_TIMER3
void timer3_isr(void)
{//1 
  set_timer3(65535-TIMER3);             // Timer neu starten 50ms
  bBlinkdauer++;
  if(bBlinkdauer == bBlinkCnt)
  {//2
    if(flag.blink)
      output_toggle(xConverterON);
    bBlinkdauer = 0;
    bBlinkPhase++;
    if(BLINK[bBlinkSeq][bBlinkPhase] == 0)
      bBlinkPhase = 0;
    bBlinkCnt = BLINK[bBlinkSeq][bBlinkPhase];
    
  }//2
}//1

#INT_RB
void rb4_isr(void)
{//1
  // ioc on rb4
  bReadIOC = PORTB & 0x10;              // read portb -> reset interrupt      
  if(bDaliMode == DALI_STBY) 
  {//2
    set_timer1(65535-1014);                        // -27µs dann passt der timer!?
    disable_interrupts(int_timer0);
    clear_interrupt(int_timer1);                 
    enable_interrupts(INT_TIMER1);
    daliflag.ioc = 0;
    bDaliMode = DALI_RCV;
    wDaliRcv = 0;  
    bDaliBitCnt = 16;       
  }//2
  else
    daliflag.ioc = 1;
}//1

/*#############################################################################
                                  M A I N
#############################################################################*/
void main(void) 
{//1  
  if(!(PORTC & 0b00100000))       // testmode when jumper is connected
    testmode();
  
  init_lantern();                 // init 
  init_dali();
  
  while(TRUE)
  {//2 
    dali();                       // dali based functions
    lantern();                    // lantern based functions    
    restart_wdt();
  }//2
}//1
