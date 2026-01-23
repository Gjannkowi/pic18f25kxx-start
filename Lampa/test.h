/***********************************************************************************
Testmode when Jumper is connected
***********************************************************************************/
void testmode(void)
{//1
  init_test();
//  init_dali();

  while(!(PORTC & 0b00100000))       // solange jumper gesteckt
   {//2
//    dali();
    if(flag.ms100)
    {//3
      flag.ms100 = 0;
      controller_ok_led();            // toggle fast at testmode      
      led_monitoring();
      if(!(PORTC & 0b00010000))       // only when pulse is NOT active 
        check_current();  
      check_converter();
      heater_control(0);
		
      bDaliTestCnt--;                 // Testen der Dali Schnittstelle 
      if(!bDaliTestCnt)
      {//4
        bDaliTestCnt = DALI_TEST_CNT;
        output_toggle(xDaliSend);     
      }//4
    }//3
    restart_wdt();
  }//2  
}//1

void init_test(void)
{//1 
  unsigned int8 value_high, value_low;
  setup_wdt(wdt_off);                // watchdog off

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
  ANSELB = 0b00000110;               // POR default digital; PIN Tausch wegen Heizung 
//  ANSELC = 0b01000000;               // no analogs on port c
  setup_adc(ADC_CLOCK_INTERNAL);
  
  IOCB = 0b00010000;                 // RB4 IOC enabled
   
  pwm_set_duty_percent(pwm_out, 1000);   // brightest step
  pwm_on(pwm_out);
 
  pwm_off(heater_half);              
  pwm_off(heater_full);

  // T0: cycle 100ms 
  setup_timer_0(T0_INTERNAL | T0_DIV_4);  // internal clock | Presc. 4 
  set_timer0(15535);                      // -> counts 50000 until 65535

  // T1: DALI timer
  // 748 = 374탎, 914 = 457탎, 1498 = 749탎, 1832 = 916탎 
  setup_timer_1(T1_INTERNAL | T1_DIV_BY_2);    // 

  set_led_current(10);               // Digi Poti einstellen
  delay_ms(150);                     // MUSS drin sein wegen Einschwingen der Schaltung. 
 
  // Get number of LED circuits
  set_adc_channel(0);
  delay_us(100);
  bADC_Ch0 = read_adc();

  set_adc_channel(1);
  delay_us(100);
  bADC_Ch1 = read_adc();
 
  set_adc_channel(2);
  delay_us(100);
  bADC_Ch2 = read_adc();

  value_high = bADC_Ch0 + LED_TEST_TOL;
  value_low  = bADC_Ch0 - LED_TEST_TOL;

  if((bADC_Ch1 <= value_high) && (bADC_Ch1 >= value_low))
    bCircuitNumber = 2;                                      // min. 2 Circuits

  if((bADC_Ch2 <= value_high) && (bADC_Ch2 >= value_low))    // 3 circuits
    bCircuitNumber = 3; 

  if((bCircuitNumber == 3) && (bADC_Ch0 < 102))
    set_led_current(10);                               // -> 

  if((bCircuitNumber == 2) && (bADC_Ch0 < 102))               
    set_led_current(10);                               // -> 

  wLedTestTime = 20;              // 2s Testzeit
  bCount500ms = 5;
  bCount1000ms = 10;
  bDaliTestCnt = DALI_TEST_CNT;
  
  // Init interrupts
  enable_interrupts(INT_TIMER0);   
  enable_interrupts(INT_RB4);     // IOC
  enable_interrupts(GLOBAL);
}//1
