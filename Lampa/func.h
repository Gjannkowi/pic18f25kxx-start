/*=============================================================================
                                FUNKTIONEN
=============================================================================*/
void init_lantern(void);
void lantern(void);
void check_current(void);
void controller_ok_led(void);
void check_converter(void);
void testmode(void);
void kill_fuse(void);
void heater_control(char mode);
void operating_hours(void);
void led_monitoring(void);
void check_param(void);
void set_led_current(unsigned int8 poti_val);
void check_status(void);
void testmode(void);
void init_test(void);
void check_blink(void);

void eeprom_write(char address, value);
unsigned int8 eeprom_read(unsigned char address);
boolean poti_ack(unsigned int8 adr);
void write_i2c(unsigned int8 adr, unsigned int8 ch, unsigned int8 data);
unsigned int8 read_i2c(unsigned int8 adr);

//--------------------------------------------------------------------------------
void eeprom_write(char address, value)
{//1
  write_eeprom(address, value);
}//1

unsigned int8 eeprom_read(unsigned char address)
{//1
  return(read_eeprom(address));
}//1

void init_lantern(void)
{//1
  unsigned int32 dLastValue=0, dActValue=0;
  unsigned int8 i;

  //set in/out direction
  set_tris_a(0b00001111);
  set_tris_b(0b00010110);     // B4=Dali_rcv IOC
  set_tris_c(0b00100000);     // c5 = Testmode Jumper
  set_tris_e(0);

  output_a(0);       
  output_b(0);
  output_c(0);
  output_e(0);

  flag = 0;  

  // setup analog pins
  ANSELA = 0b00001111;               // POR default analog
  ANSELB = 0b00000110;               // POR default digital  
//  ANSELC = 0b00000000;               // 

  setup_adc(ADC_CLOCK_INTERNAL);
  
  IOCB = 0b00010000;                 // RB4 IOC enabled
    
  pwm_set_duty_percent(pwm_out, 1000);   // brightest step
  pwm_on(pwm_out);

  pwm_off(heater_half);
  pwm_off(heater_full);

  // watchdog
  setup_wdt(WDT_2S);                 // 2s watchdog
  restart_wdt();

  // T0: cycle 100ms 
  setup_timer_0(T0_INTERNAL | T0_DIV_4);  // internal clock | Presc. 4 
  set_timer0(15535);                      // -> counts 50000 until 65535

  // T1: DALI timer
  // 748 = 374µs, 914 = 457µs, 1498 = 749µs, 1832 = 916µs 
  setup_timer_1(T1_INTERNAL | T1_DIV_BY_2);    // 

  check_param();                    // Laterne parametriert?
 
//  set_led_current(20);              // wegen Anlaufschwierigkeiten der Schaltung
//  delay_ms(50);          

  set_led_current(eeprom_read(EE_POTI));     // set the digi potis
  bCircuitNumber = eeprom_read(EE_CIRCUITS); 
  bStatus = eeprom_read(EE_STATUS);
 
  if(bStatus & ST_EOL)                       // ist der Status EOL und die Sicherung wurde NICHT gekillt
  {//2
    output_high(xConverterOn);               // converter off
    delay_ms(500);
    reset_cpu();                             // dann dauerhaft neu starten
  }//2
  
  //Laterne mit fester Blinkfrequenz?
  check_blink();

  // get operating hours counter  
  for(i=0; i<5; i++)
  {//2
    dActValue = make32(eeprom_read(EE_OHC_START+bOHoffset),
                       eeprom_read(EE_OHC_START+bOHoffset+1),
                       eeprom_read(EE_OHC_START+bOHoffset+2),
                       eeprom_read(EE_OHC_START+bOHoffset+3));   

    if(dActValue > dLastValue)
    {//3
      dLastValue = dActValue;
    }//3
    else
    if(dActValue < dLastValue)
    {//3
      dOHC = dLastValue;
      break;
    }//3
    bOHoffset += 4;    
  }//2 
                                          
  // Init counter
  bCount500ms = 5;
  bCount1000ms = 10;
  bCountMin = MINUTE;
  bCountEOL = EOL_CNT;
  wLedTestTime = LED_TEST_TIME; 

  bHeaterHalfCnt = 0;
  bHeaterFullCnt = 0;

  // Init interrupts
  enable_interrupts(INT_TIMER0);   
  enable_interrupts(INT_RB4);     // IOC
  enable_interrupts(GLOBAL);
 
}//1

void lantern(void) 
{//1
  check_status();                 // 

  if(flag.ms100 && flag.param && !bParam) // every 100ms wenn paramertiert ist und kein Parametriermodus
  {//2
    flag.ms100 = 0;
    if(!(PORTA & 0b10000000))       // converter on?
    {//3      
      led_monitoring();
      if(!(PORTC & 0b00010000))     // only when pulse is NOT active 
        check_current();            // Eingangstrom checken wegen pulsen
      check_converter();            // nach check current prüfen wegen pulsen
    }//3
    if(flag.heat_half || flag.heat_full)   // 
      heater_control(1);                   // Heizung PWM einstellen
  }//2

  if(flag.ms500)                     // every 500ms
  {//2
    flag.ms500 = 0;
    controller_ok_led();            // led µc is running
    if(!flag.param)                 // keine Parametrierung?
    {//3
      output_toggle(xConverterON);  // dann blinken
    }//3 
  }//2

  if(flag.ms1000)                   // every second
  {//2
    flag.ms1000 = 0;
    bCountMin--;
    if(!bCountMin)                  // every minute
    {//3
      bCountMin = MINUTE;
      if(!(PORTA & 0b10000000))       // converter on?
        operating_hours();            // nur wenn converter an ist betriebstunden zählen
    }//3
    heater_control(0);  
  }//2
}//1

void check_current(void)
{//1
  // 100mA -> 0,5V am Analogeingang
  // Pulsen ab <250mA -> 63dec 
  
  delay_ms(10);                                             // wegen Trägheit des Analogeingangs
  //read analog input
  set_adc_channel(3);
  delay_us(100);
  bADC_Ch3 = read_adc();

  if(bADC_Ch3 < 63 && flag.param && !(PORTA & 0b10000000))  // < 1.25V && parametrierung vorhanden? && converter ON ~250mA
    flag.pulse = 1;                                         // current pulse on
  if(bADC_Ch3 < 63 && !(PORTC & 0b00100000))                // < 1.25V && testmode?
    flag.pulse = 1;                                         // current pulse on
//  if((bADC_Ch3 > 70) || (bDaliShortAdr != 0xFF))            // > 1.36V || dali adresse vorhanden  ~270mA
  if((bADC_Ch3 > 70) || !daliflag.bustimeout)               // > 1.36V || DALI Datenverkehr vorhanden
    flag.pulse = 0;                                         // current pulse off                                              
}//1 

void check_converter(void)
{//1
  // voltage output converter 
  set_adc_channel(10);                 
  delay_us(100);
  bADC_Ch10 = read_adc();

  // Ausgangsspannung kleiner 2,5V?  
  if(bADC_Ch10 < 26)                         // Wandler AN und Ausgangsspannung < 2,5V? (AD Eingang ~0,5V == 26dec)
  {//2
    bStatus |= OUT_FAIL;                     // status converter output failure
    flag.pulse = 0;                          // nicht pulsen wenn converter nicht arbeitet (Eingangsspannung < 19V)
    if(!flag.blink)                          // rev1: wenn KEIN Blinkmodus
    {//3
      output_high(xConverterOn);               // Wandler abschalten
      delay_ms(50);
      output_low(xConverterOn);                // Wandler neu einschalten
    }//3
  }//2

  // Ausgangsspannung größer 20V (4V AD)
  if((bADC_Ch10 >= 205)) 
  {//2
    bStatus |= OUT_FAIL;                     // status converter output failure
    if(PORTC & 0b00100000)                   // Nur wenn kein Testmodus EEPROM schreiben
      eeprom_write(EE_STATUS, bStatus);         
    kill_fuse();                             // Bei zu hoher Ausgangsspannung Sicherung killen (28.11.2017, Ser) 
  }//2
}//1

void kill_fuse(void)
{//1
  output_high(xKill24V);       // mosfet ansteuern und 24V Sicherung zerstören
}//1 

void led_monitoring(void)
{//1
  // Spannungen zwischen den einzelnen Kreisen messen.
  // Wenn der Unterschied zwischen den Kreisen größer 0,3V ist liegt ein Fehler vor.
  // 0,3V entsprechen 0,1V am Analog Eingang -> +/- 5 Digits 
  unsigned int8 i, j, tol, aValues[3];   
                                
  set_adc_channel(0);
  delay_us(100);
  bADC_Ch0 = read_adc();

  set_adc_channel(1);
  delay_us(100);
  bADC_Ch1 = read_adc();
 
  set_adc_channel(2);
  delay_us(100);
  bADC_Ch2 = read_adc();

  aValues[0] = bADC_Ch0;
  aValues[1] = bADC_Ch1;
  aValues[2] = bADC_Ch2;
 
  i=j=0;                     // init

  if(bCircuitNumber == 2)    //  
  {//2
//    i=0;
    if(aValues[0] < aValues[1])
      j = aValues[1] - aValues[0];
    if(aValues[0] > aValues[1])
      j = aValues[0] - aValues[1];
    if(aValues[0] == aValues[1])
      j=0;
  }//2
  if(bCircuitNumber == 3)    //  
  {//2
    if(aValues[0] < aValues[1])
      j = aValues[1] - aValues[0];
    if(aValues[0] > aValues[1])
      j = aValues[0] - aValues[1];
    if(aValues[0] == aValues[1])
      j=0;

    if(aValues[1] < aValues[2])
      i = aValues[2] - aValues[1];
    if(aValues[1] > aValues[2])
      i = aValues[1] - aValues[2];
    if(aValues[1] == aValues[2])
      i=0;
  }//2
  tol = LED_TEST_TOL;                 // direkt mit konstante klappt der Vergleich nicht!? wegen Typ?
  if(j > tol || i > tol)              // greater than test tolerance?
  {//2
    if(wLedTestTime)
      wLedTestTime--;
    else
    {//3
      // error? -> destroy fuse
      bStatus |= LED_FAIL;
      if(PORTC & 0b00100000)                 // Nur wenn kein Testmodus EEPROM schreiben
        eeprom_write(EE_STATUS, bStatus);    // status 1 (LED failure) 
      kill_fuse();               
    }//3
  }//2
  else
  {//2
    wLedTestTime = LED_TEST_TIME;
    if(!(PORTC & 0b00100000))             // testmode?
      wLedTestTime = 20;
  }//2
}//1
 
void operating_hours(void)
{//1  
  bCountEOL--;              
  if(!bCountEOL)
  {//2    
    bCountEOL = EOL_CNT;                     // set counter for next 20min
    dOHC += EOL_CNT;                         // add 20min to operating hour counter
    if(dOHC >= EOL)// && !(bStatus & ST_EOL))   // >6000000min und status ist noch nicht EOL?
    {//3  
      bStatus |= ST_EOL;                     // status EOL
      eeprom_write(EE_STATUS, bStatus);
      kill_fuse(); 
    }//3
    else                                     // save
    {//3
      if(bOHoffset >= 20)
        bOHoffset = 0;

      eeprom_write(EE_OHC_START+bOHoffset+3, make8(dOHC, 0));
      eeprom_write(EE_OHC_START+bOHoffset+2, make8(dOHC, 1));
      eeprom_write(EE_OHC_START+bOHoffset+1, make8(dOHC, 2));
      eeprom_write(EE_OHC_START+bOHoffset  , make8(dOHC, 3));

      bOHoffset += 4;
    }//3
  }//2
}//1

void controller_ok_led(void)
{//1
  output_toggle(xLedOut);
}//1

void heater_control(char mode)
{//1
  /*-------------------------------------------------------------------
°C   -50 -40 -30 -20 -10   0  10  20  25  30  40  50  60  70  80
ADC  189 185 180 175 171 165 160 156 153 151 146 141 137 132 128
ADC Werte berechnet
-------------------------------------------------------------------*/

  switch(mode)
  {//2
    case 0:  //Temperatur checken
             set_adc_channel(8);
             delay_us(100);
             bADC_Ch8 = read_adc();

             if(bADC_Ch8>=160 && !flag.heat_half)                       
             {//3
               flag.heat_half = 1;
               bHeaterHalfCnt = 0;
               if(!(PORTC & 0b00100000))          // Testmodus?
                 bHeaterHalfCnt = PWM_STEP - 1;   // -> max PWM  
               pwm_set_duty_percent(heater_half, HEATER_PWM[bHeaterHalfCnt]);
               pwm_on(heater_half);
             }//3
             else if(bADC_Ch8<=157)
             {//3
               pwm_off(heater_half);
               flag.heat_half = 0;
             }//3

             if(bADC_Ch8>=164 && !flag.heat_full)                       
             {//3
               flag.heat_full = 1;
               bHeaterFullCnt = 0;
               if(!(PORTC & 0b00100000))          // Testmodus?
                 bHeaterFullCnt = PWM_STEP - 1;   // -> max PWM 
               pwm_set_duty_percent(heater_full, HEATER_PWM[bHeaterFullCnt]);
               pwm_on(heater_full);
             }//3
             else if(bADC_Ch8<=160)           
             {//3               
               pwm_off(heater_full);
               flag.heat_full = 0;
             }//3
             break;

    case 1:  //PWM erhöhen
             if(flag.heat_half)
             {//3
               if(HEATER_PWM[bHeaterHalfCnt] < PWM_MAX)
                 pwm_set_duty_percent(heater_half, HEATER_PWM[++bHeaterHalfCnt]);                       
             }//3    
             if(flag.heat_full)
             {//3
               if(HEATER_PWM[bHeaterFullCnt] < PWM_MAX)
                 pwm_set_duty_percent(heater_full, HEATER_PWM[++bHeaterFullCnt]);                
             }//3   
             break;
  }//2
}//1

void check_param(void)
{//1
  if(eeprom_read(EE_PARAM) == 'P')        // parametrierung vorhanden?
  {//2                                    // yes 
    flag.param = 1;     
  }//2
}//1

void set_led_current(unsigned int8 poti_val)
{//1
//     
//  0-200k Reg A
    write_i2c(ADR_POTI20K, POTI_CH_A, 0);              // ein Poti reicht aus
//  0-200k Reg B
    write_i2c(ADR_POTI20K, POTI_CH_B, poti_val);
}//1

void check_status (void)
{//1
  bStatus = 0;

  bStatus |= POWER;                         // Lantern is powered and working

  if(flag.blink)                            // rev1: Blinkmodus durch Sequenz?
     bStatus |= ON_OFF;                     // dann status AN auch in Blinkpause
  else
  if(!(PORTA & 0b10000000))                 // converter ON?
  {//2
    bStatus |= ON_OFF;  

    if(bADC_Ch10 < 25)                         // Wandler AN und Ausgangsspannung < 1V?
      bStatus |= OUT_FAIL;                     // status converter output failure    
  }//2

  if(bDaliPowerLevel < bDaliMaxLevel)       // lantern dimmed?
    bStatus |= DIMM;

  if(dOHC >= EOL)                           // end of life
    bStatus |= ST_EOL;          
}//1

void check_blink()
{//1
  bBlinkSeq = eeprom_read(EE_BLINK);         // 1-16, 0=keine
  if(bBlinkSeq)
  {//2
    setup_timer_3(T3_INTERNAL | T3_DIV_BY_8); 
    enable_interrupts(INT_TIMER3); 
    set_timer3(65535-TIMER3);                    //-> Timer3 Overflow 50ms     
    flag.blink = 1;                              // Blinkflag 
    bBlinkSeq -= 1;                              // -1 wegen Sequenz aus EEPROM. 0 = kein Blink. nicht nötig bei DALI
    bBlinkPhase = 0;                             // AN Zeit aus Array
    bBlinkCnt = BLINK[bBlinkSeq][bBlinkPhase];
    bBlinkdauer = 0;
  }//2
}//1

boolean poti_ack(unsigned int8 adr)
{//1
  int1 ack;
  i2c_start();            // If the write command is acknowledged,
  ack = i2c_write(adr);  // then the device is ready.
  i2c_stop();
  return !ack;
}//1

void write_i2c(unsigned int8 adr, unsigned int8 ch, unsigned int8 data)
{//1
  while(!poti_ack(adr));
  i2c_start();
  i2c_write(poti, adr);
  i2c_write(poti, ch);
  i2c_write(poti, data);
  i2c_stop();
}//1

unsigned int8 read_i2c(unsigned int8 adr)
{//1
  unsigned int8 dat;
  adr |= 0b00000001;            // LSB = 1 = read mode
  while(!poti_ack(adr));
  i2c_start();
  dat = i2c_read(poti,0);
  i2c_stop();
  return(dat); 
}//1



 
  
