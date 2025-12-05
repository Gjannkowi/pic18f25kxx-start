/*################# FUNKCJE PODSTAWOWE #####################################*/

#INT_TIMER1			// Timer-1: Wywo?anie co (40) 100ms
void int_T1(void)
{ //1
 set_timer1(K_T1_SET);		// Restart Timer1
 xT1_FLAG=1;				// Ustaw flag? T1
} //1

#INT_RDA
void int_receive(void)		// DALI-1 Odpowied? S66
{//1						  
 bDALI_RX_Dat=bU1RXB;		// RX 1 bajt danych
 xDALI_RX_OK=1;				// Flaga RX
// ?? clear_interrupt(INT_RDA);
}//1

// Wy?lij adres i komend? zgodnie z protoko?em DALI
// Adres to YAAAAAAS, tutaj zawsze S=1
// Pauza mi?dzy znakami tutaj 100ms, patrz main())
void TX_DALI(int8 adr, int8 cmd)// 1x wykonaj komend? wysy?ania DALI
{//1							// DALI1: tylko 16 bitów (bajt adr + bajt danych)
 disable_interrupts(INT_RDA);	// RX na razie WY?
 bU1TXB=adr;					// 1.Wy?lij bajt adr, od razu dalej do TSR
 bU1TXB=cmd;					// 2.Wy?lij bajt CMD/DAP (DALI 1), od razu do bufora
 while (!bit_test(bU1ERRIR,7));// Bit7=TXMTIF: czekaj a? TX gotowe
 bit_set(bU1FIFO,1);			// U1FIFO: TXBE.5=Wyczy?? TX, RXBE.1=Wyczy?? RX
 enable_interrupts(INT_RDA);	// RX znowu W?
}//1

void INIT_PRG(void)
{//1
 setup_oscillator(OSC_HFINTRC_4MHZ | OSC_HFINTRC_ENABLED);
// Warto?? zegara bazowego podzielona przez 2
 bCLKRCLK=0b00000001;		// Bit 0=bRxyPPS=1=HFINTOSC, bity wyboru zegara
 set_tris_a(0b11000000);	// Bit 7=przycisk1, Bit 6=przycisk2, Bity 5:0=LED, (bez ADC-IN)
 set_tris_b(0b00001111);	// Bit5=WYJ_Przeka?nik, Bit3:0=WEJ_Adr1-SW
 set_tris_c(0b10001111);	// Bit7=RX-WEJ, 6:TX-WYJ, Bit3:0=N_Adr10-SW
 port_a_pullups(0b11000000);
 port_b_pullups(0b00001111);
 port_c_pullups(0b00001111);

 bPORTA=0;					// Ustaw wszystkie porty na 0 aby unikn?? niespodziewanego zachowania
 bPORTB=0;
 bPORTC=0;

// #### Bez ADC

 set_timer1(K_T1_SET);		// Timer1 wst?pne ?adowanie 100ms
 setup_timer_1(T1_INTERNAL | K_T1_DIV);	// Timer1 zegar wewn. dla taktu czasu

// Ustawienia DALI dla UART1, patrz PIC18Fx5K42_90003200A.pdf punkt 2.1
 bU1CON0=0b00111000;		// Bit 7:6=00=BRGS+ABDEN, 5:4=11=Tx+Rx w?., 3:0=1000=Urz?dzenie DALI
 bU1P1H=0; bU1P1L=0x16;	  	// =22 pó?okresy bitu bezczynno?ci mi?dzy ramkami forward
 bU1P2H=0; bU1P2L=0x16;		// =xx pó?okresów bitu bezczynno?ci dla ramek forward

// bU1BRGH=0; bU1BRGL=0xCF;	// 18F47K42: 1200 baud (=(4MHz/1200/16) -1)
 bU1BRGH=0; bU1BRGL=0xCB;	// 18F25K42: dostosowane dla S66 (Dlaczego?)

 bU1CON2=0b01100100;		// Ustaw Bit6=1=odwró? RXPOL, 5:4=10=2 bity stopu, 2=1=odwró? TXPOL
 bRC6PPS=0b00010011;		// Wybór pinu PortC:6=TX
 bU1RXPPS=0b00010111;		// Wybór pinu PortC:7=RX
 bU1CON1=0b10000000;		// Ustaw Bit7=1 W??czenie portu szeregowego W?

 enable_interrupts(INT_TIMER1);
 enable_interrupts(INT_RDA);
 enable_interrupts(GLOBAL);
 
 prg_state = STANDBY;		// Start w trybie Standby
}//1
