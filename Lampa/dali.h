/*=============================================================================
                          DALI FUNCTION PROTOTYPES
=============================================================================*/
void dali(void);
void init_dali(void);
void dali_cmd(unsigned int8 command);
void dali_dap(unsigned int8 value);
void dali_special(unsigned int8 command, value);
void dali_check_bus(void);

/*=============================================================================
                               DALI FUNCTIONS

Short or group address  YAAAAAAS 
Short addresses (64)    0AAAAAAS 
Group addresses (16)    100AAAAS 
Broadcast               1111111S 
Special command         101CCCC1   
Special command         110CCCC1 
 
S:  selector bit:  S = ‘0’  direct arc power level following 
                   S = ‘1’  command following 
Y:  short- or group address:  Y = ‘0’  short address 
                              Y = ‘1’  group address or broadcast 
A:  significant address bit 
C:  significant command bit 
=============================================================================*/
void dali(void)
{//1
  dali_check_bus();
   
  if(bDaliMode != DALI_DAT)                  // new dali receive data available?  
    return;   

  daliflag = 0;                              // erase old flags
  bDaliRcvCmd = make8(wDaliRcv, 0);          // extract command 
  bDaliRcvAdr = make8(wDaliRcv, 1);          // extract address 
  bDaliMode = DALI_STBY;                     // set mode that new data can be achieved  
  bDaliBusTimeout = DALI_TIMEOUT;            // Reset Timeout
  flag.pulse = 0;                            // kein Pulsen bei Dalidaten 

  if((bDaliRcvAdr & 0xE0) == 0xA0 || (bDaliRcvAdr & 0xE0) == 0xC0)  // Special command?
  {//2
    dali_special(bDaliRcvAdr, bDaliRcvCmd);
  }//2
  else
  {//2
    if(bDaliRcvAdr & 0b00000001)             // command following?
    {//3  
      daliflag.cmd = 1;
      if((bDaliRcvCmd >= 0x20) && (bDaliRcvCmd <= 0x80))  // config command?
      {//4
        if(!bCmdCnt)
        {//5
          bLastCmd = bDaliRcvCmd;            // cmd merken
          bCmdCnt++;                         
          return;                            // und raus
        }//5
        if(bCmdCnt)                          // es gab schon ein config cmd
        {//5
          if(bLastCmd == bDaliRcvCmd)        // gleich letztem cmd?
          {//6
             bLastCmd = 0;                   // dann letztes cmd löschen 
          }//6                               // und weiter in der funktion     
          else                               // sonst
          {//6
            bLastCmd = bDaliRcvCmd;          // neues cmd merken
            return;                          // und raus aus funktion 
          }//6   
        }//5  
      }//4
    }//3

    if((bDaliRcvAdr & 0b11111110) == 0xFE)           // broadcast?
      daliflag.broadcast = 1;    
    else
    if(bDaliRcvAdr & 0b10000000)           // group address?
    {//3                                
      bDaliRcvAdr >>= 1;                   // shift left 1 Bit (get rid of selector bit)
      bDaliRcvAdr &= 0x0F;
      if(bDaliRcvAdr <= 7)
      {//4
        if(bDaliGroup0_7 & (1<<bDaliRcvAdr))  // group 0 - 7 ?
          daliflag.group = 1;
      }//4
      else
      {//4 
        bDaliRcvAdr &= 0x07;
        if(bDaliGroup8_15 & (1<<bDaliRcvAdr))  // group 8 - 15 ?
          daliflag.group = 1;
      }//4    
      bDaliRcvAdr = 0xFF;                  // rev1: Kurzadresse löschen wenn Gruppenbefehl sonst wird Gruppenadresse als Kurzadresse ausgewertet. 
    }//3
    else                                   // short address
    {//3
      bDaliRcvAdr >>= 1;                   // shift left 1 Bit (get rid of selector bit)
      bDaliRcvAdr &= 0b00111111;
    }//3

    if(daliflag.broadcast && daliflag.cmd)                 // broadcast & command
      dali_cmd(bDaliRcvCmd);                               // run dali command
    else
    if((bDaliRcvAdr == bDaliShortAdr) && daliflag.cmd)     // short address & command
      dali_cmd(bDaliRcvCmd);                               // run dali command
    else
    if(daliflag.broadcast && !daliflag.cmd)                // broadcast & DAP
      dali_dap(bDaliRcvCmd);                               // set direct arc power
    else
    if((bDaliRcvAdr == bDaliShortAdr) && !daliflag.cmd)    // Short adress ok & DAP
      dali_dap(bDaliRcvCmd);                               // set direct arc power  
    else
    if(daliflag.group && daliflag.cmd)                     // Group & Cmd
      dali_cmd(bDaliRcvCmd);
    else
    if(daliflag.group && !daliflag.cmd)                    // Group & DAP
      dali_dap(bDaliRcvCmd); 
  }//2

//////////////////////////////////////////////// 
//  call after dali_cmd and dali_dap
  if(bDaliPowerLevel < bDaliMaxLevel)              // für Statusmeldung
    flag.dali_dim = 1;
  else
    flag.dali_dim = 0;

  if(daliflag.snd)                                 // soll gesendet werden?
  {//2
    daliflag.snd = 0;
    bDaliMode = DALI_SND;
    bDaliSndCnt = 8;                               // 8 - 0 -> Startbit + 8 Datanbits 
    bDaliSndClk = 0;       
    set_timer1(65535-6000);                        // 6ms warten bis Antwort gesendet wird
    enable_interrupts(int_timer1);
  }//2
//////////////////////////////////////////////// 
}//1

void dali_cmd(unsigned int8 command)
{//1
  unsigned int8 bTmpVar;  

  if(bParam)                    // PARAM Mode
  {//2
    bDaliPowerLevel = bDaliMaxLevel;              // pwm 100%
    switch(command)
    {//3
      case OFF:
        eeprom_write(EE_POTI, bPotiValue);          // save actual poti value 
        eeprom_write(EE_PARAM, 'P');                // mark as parameterized
        bDaliSndBuff = '+';                         // Meldung an Controller 
        daliflag.snd = 1;                           // 
        break;
      case STEP_UP:
        set_led_current(--bPotiValue);              // ! Param: widerstand kleiner = heller !
        break;
      case STEP_DOWN:
        set_led_current(++bPotiValue);
        break;
      case STORE_DTR_AS_SHORT_ADDRESS:
        eeprom_write(EE_ADDRESS, bDaliDTR);
        bDaliSndBuff = '-';                         // Meldung an Controller 
        daliflag.snd = 1;
        break;
      case STORE_THE_DTR_AS_SCENE: 
        eeprom_write(EE_CIRCUITS, bDaliDTR);
        bDaliSndBuff = '-';                         // Meldung an Controller 
        daliflag.snd = 1;
        break;
      case STORE_THE_DTR_AS_POWER_ON_LEVEL:
        eeprom_write(EE_BLINK, bDaliDTR);
        bDaliSndBuff = '-';                         // Meldung an Controller 
        daliflag.snd = 1;
        break;
      case RESET:
        eeprom_write(EE_POTI, 0);                   // Abgleichwert löschen       
        eeprom_write(EE_PARAM, 0);                  // Erststart merker löschen         
        eeprom_write(EE_CIRCUITS, 0);               // Anzahl LED Kreise löschen  
        eeprom_write(EE_ADDRESS, 0xFF);             // kein DALI = 0xFF
        eeprom_write(EE_BLINK, 0);
        bDaliSndBuff = 'R';                         // Meldung an Controller Reset erfolgt
        daliflag.snd = 1;
        break;
      case RECALL_MAX_LEVEL:
        bDaliPowerLevel = bDaliMaxLevel;                // später aus EEPROM!      
        output_low(xConverterOn);                       // lamp on if off
        break;
      case QUERY_MANUFACTURER_SPECIFIC_MODE:           //0xA6
        bDaliSndBuff = 'j';
        daliflag.snd = 1;
        break;
      case QUERY_STATUS:
        bDaliSndBuff = bStatus;
        daliflag.snd = 1;
        break;
     
      default:
        bDaliSndBuff = '-';                         // Meldung an Controller auch wenn Befehl nicht bekannt ist für 
        daliflag.snd = 1;                           // zukünftige Erweiterungen des Parametriervorgangs
        break;
    }//3
  }//2
  else
  {//2
    switch(command)
    {//3
      case OFF:              
        output_high(xConverterOn);
        if(flag.blink)
        {
          flag.blink = 0;
          disable_interrupts(INT_TIMER3);
        }
        bDaliPowerLevel = bDaliMinLevel-1;      // Leuchte ging manchmal nicht an wenn KEINE PWM anliegt?!
                                                // Floatender Feedback Eingang des Converters?
        break;                                  

      case UP:
        break;

      case DOWN:
        break;

      case STEP_UP:
        if(bDaliPowerLevel < bDaliMaxLevel)
          bDaliPowerLevel++;
        break;
        
      case STEP_DOWN:
        if(bDaliPowerLevel > bDaliMinLevel)
          bDaliPowerLevel--;
        break;

      case RECALL_MAX_LEVEL:
        check_blink();
        bDaliPowerLevel = bDaliMaxLevel;                // später aus EEPROM!      
        output_low(xConverterOn);                       // lamp on if off
        break;

      case RECALL_MIN_LEVEL:
        check_blink();
        bDaliPowerLevel = bDaliMinLevel;                // später aus EEPROM!      
        output_low(xConverterOn);                       // lamp on if off
        break;

      case STEP_DOWN_AND_OFF:
        if(bDaliPowerLevel == bDaliMinLevel)  
          output_high(xConverterOn);                    // lamp off
        else
          bDaliPowerLevel--;
        break;

      case ON_AND_STEP_UP:
        if(!(PORTA & 0b10000000))                       // converter off?
        {//4
          bDaliPowerLevel = bDaliMinLevel; 
          output_low(xConverterOn);                     // lamp on
        }//4
        else
          bDaliPowerLevel++;
        break;

      case RESET:                               // Blinksequenz Aus. Laterne bleibt An
        if(flag.blink)
        {//4
          flag.blink = 0;
          disable_interrupts(INT_TIMER3);
          output_low(xConverterOn);             // lamp on      
        }//4
        break;

      case QUERY_STATUS:
        if(daliflag.broadcast || daliflag.group)
          return;
        bDaliSndBuff = bStatus;
        daliflag.snd = 1;
        break;

      case QUERY_MANUFACTURER_SPECIFIC_MODE:           //0xA6 -> für PARAM
        bDaliSndBuff = 'j';
        daliflag.snd = 1;
        break;

      case QUERY_ACTUAL_LEVEL:
        if(PORTA & 0b10000000)              // Converter off?
          bDaliSndBuff = 0;
        else
          bDaliSndBuff = bDaliPowerLevel;
        daliflag.snd = 1;
        break;

      default:  break;
    }//3
    switch(command & 0xF0)
    {//3
      case SCENE:
        bBlinkSeq = command & 0x0F;
        if(bBlinkSeq > (SEQ - 1))                    // blinksequenz größer als hinterlegte Anzahl?
          return;
        flag.blink = 1;
        setup_timer_3(T3_INTERNAL | T3_DIV_BY_8); 
        disable_interrupts(INT_TIMER3); 
        set_timer3(65535-TIMER3);                     //-> Timer3 Overflow 50ms 
        bBlinkPhase = 0;                             // AN Zeit aus Array
        bBlinkCnt = BLINK[bBlinkSeq][bBlinkPhase];
        bBlinkdauer = 0;
//rev1        bDaliPowerLevel = bDaliMaxLevel;
        enable_interrupts(INT_TIMER3);
        output_low(xConverterON);                    // Laterne AN 
        break;
      case ADD_TO_GROUP: 
        if((command & 0x0F) <= 7)
        {//4
          bTmpVar = eeprom_read(EE_GROUP_0_7);
          bTmpVar |= 1<<(command & 0x0F);
          eeprom_write(EE_GROUP_0_7, bTmpVar);
          bDaliGroup0_7 = bTmpVar;  
        }//4
        else
        {//4
          bTmpVar = eeprom_read(EE_GROUP_8_15);
          bTmpVar |= 1<<(command & 0x07);
          eeprom_write(EE_GROUP_8_15, bTmpVar); 
          bDaliGroup8_15 = bTmpVar;
        }//4
        break;
      case REMOVE_FROM_GROUP:
        if((command & 0x0F) <= 7)
        {//4
          bTmpVar = eeprom_read(EE_GROUP_0_7);
          bTmpVar &= ~(1<<(command & 0x0F));
          eeprom_write(EE_GROUP_0_7, bTmpVar);
          bDaliGroup0_7 = bTmpVar;
        }//4
        else
        {//4
          bTmpVar = eeprom_read(EE_GROUP_8_15);
          bTmpVar &= ~(1<<(command & 0x07));
          eeprom_write(EE_GROUP_8_15, bTmpVar);
          bDaliGroup8_15 = bTmpVar;
        }//4      
        break; 
    }//3
  }//2
 
  // hier wird die PWM eingestellt (nur bei Änderung)
  if(bDaliPowerLevel != bLastPowerLevel)
  {//2
//rev1    if(flag.blink)                                                 // keine Änderung im Blinkmodus
//      bDaliPowerLevel = bDaliMaxLevel;
    pwm_set_duty_percent(pwm_out, wPwmDuty[bDaliPowerLevel]);
    bLastPowerLevel = bDaliPowerLevel;                             // neu merken
  }//2
}//1

void dali_dap(unsigned int8 value)
{//1
  if(bParam)                                  // Parametriermodus?
  {//2
    bDaliDTR = value;
    return;
  }//2
  else
  {//2
    if(value == 'P')
    {//3
      bDaliLastDAP = value;
      bParamCnt = 0;
    }//2
    if(value == PARAM[bParamCnt+1] && bDaliLastDAP == PARAM[bParamCnt])
    {//3
      bDaliLastDAP = value;
      bParamCnt++;
    }//3
    else
    {//3
      bParamCnt = 0;
    }//3
    if(bParamCnt == 4)                          // DAP 'P''A''R''A''M' received in series
    {//3 
      bParam = 1;
      bDaliSndBuff = 'P';
      daliflag.snd = 1;
      bDaliPowerLevel = 254; 
      bPotiValue = eeprom_read(EE_POTI);        // Potiwert bleibt auf EEPROM Wert
      flag.param = 1;                           // blinken während der parametrierung unterdrücken
      flag.pulse = 0;                           // kein pulsen während der parametrierung
      output_low(xConverterON);                 // laterne einschalten falls aus
      return;
    }//3
  }//2

  // --- normal operation ---
  //rev1  if(value <= bDaliMaxLevel && value >= bDaliMinLevel && !flag.blink) // und keine Änderung im Blinkmodus
  if(value <= bDaliMaxLevel && value >= bDaliMinLevel)
  {//2
    bDaliPowerLevel = value;                                          // Wert übernehmen
    bLastPowerLevel = bDaliPowerLevel;                                // neu merken
    
    if(!daliflag.broadcast)                            // Nur auf Gruppe oder Kurzadresse reagieren. KEIN Blinkmodus                                                
    {//3
//rev1
      pwm_set_duty_percent(pwm_out, wPwmDuty[bDaliPowerLevel]);     // PWM einstellen
      
      if(flag.blink)                                               // schon im Blinkmodus?
      {//4
        check_blink();                                             // Timer neu setzen -> wegen drift bei blink mit mehreren Laternen
        output_low(xConverterOn);                                  // UND converter AN -> Timer 3 NUR Toggle!
      }//4
      else                                             
      if(PORTA & 0b10000000)                                       // ansonsten wenn converter AUS
      {//4
        check_blink();                                                // wenn feste Blinksequenz hinterlegt Flag setzen und Timer starten
        output_low(xConverterOn);                                     // UND converter AN
      }//4
    }//3                          
  }//2 
}//1

void dali_special(unsigned int8 command, value)
{//1

  if(bParam)
  {//2
    switch(command)
    {//3
      case DTR0:
        bDaliDTR = value;
        bDaliSndBuff = '+';                        // Meldung an Controller 
        daliflag.snd = 1;
        break;
      case TERMINATE:
        reset_cpu();
        break;
      default:break;
    }//3
  }//2
}//1 

void init_dali(void)
{//1
  daliflag = 0;
  bDaliShortAdr = eeprom_read(EE_ADDRESS);
  bDaliBusTimeout = DALI_TIMEOUT;
  bDaliGroup0_7  = eeprom_read(EE_GROUP_0_7);
  bDaliGroup8_15 = eeprom_read(EE_GROUP_8_15);

  bDaliSndBuff = '.';                            // Leuchte meldet sich nach einschalten
  bDaliMode = DALI_SND;
  bDaliSndCnt = 8;                               // 8 - 0. Startbit + 8 Datanbits 
  bDaliSndClk = 0;       
  set_timer1(65535-6000);                        // 6ms warten bis Antwort gesendet wird
  enable_interrupts(int_timer1);

  bDaliMaxLevel = DALI_MAX_LEVEL;  
  bDaliMinLevel = DALI_MIN_LEVEL;    //   
  bDaliPowerLevel = bDaliMaxLevel;   // 
  bLastPowerLevel = bDaliMaxLevel;

}//1

void dali_check_bus(void)
{//1
  if(bParam || !flag.param)   // nicht während des Parametriervorgangs prüfen oder wenn nicht parametriert (sonst Leuchte aus..)
    return;  

//  if(bDaliShortAdr == 0xFF)   // keine DALI ADRESSE?
//    return;                   // dann BUS NICHT prüfen!

  if((bDaliMode == DALI_STBY) && daliflag.ms100)   //  
  {//2
    daliflag.ms100 = 0;
    if(bDaliBusTimeout)
      bDaliBusTimeout--;                      // Reset der Variable wenn Dali daten empfangen wurden dali() 
    if(!bDaliBusTimeout)                      // Timeout abgelaufen?
    {//3
      daliflag.bustimeout = 1;                // KEIN Datenverkehr
      if((PORTB & 0b00010000) && !flag.blink) // Timeout und keine DALI Spg vorhanden
      {//4       
        check_blink();                        // feste Blinksequenz hinterlegt? !! funktioniert NUR mit fester Blinksequenz !!
        pwm_set_duty_percent(pwm_out, 1000);  // volle Helligkeit
        output_low(xConverterOn);             // Leuchte AN
      }//4 
      else                                    // Timeout und DALI Spg vorhanden
      if(!(PORTB & 0b00010000))
      {//4
        flag.blink = 0;
        disable_interrupts(INT_TIMER3);
        output_high(xConverterOn);            // Leuchte AUS
      }//4
    }//3
  }//2
}//1
   
