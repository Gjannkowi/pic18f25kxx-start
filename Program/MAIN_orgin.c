#define VERSION_NO "S66-Adr-0c"		// + bin 0 wg. koniec String
#HEXCOMMENT\Version: VERSION_NO		// Wpis w pliku Hex koniec
/*********************   P R O G R A M   G ? Ó W N Y   ********************
 OPROGRAMOWANIE : Samodzielne narz?dzie adresuj?ce dla S66 & S76
 AUTOR          : E.Eickhoff
 MIEJSCE+DATA   : Bremen, 23.11.23
 KOMPILATOR     : PIC C-Compiler CCS PCM V5.096  + MPLAB x 6.0

 ## WARIANT Z PRZYCISKIEM 2. Gdy S66 ok, PRZYCIEMNIENIE (stare przypisanie LED) ###
  
 OPIS FUNKCJI
 a) Po pod??czeniu zasilania 24V i S66
	LED 1 czerwony miga     : B??d, adres >63
	LED 2 ?ó?ty miga krótko : gotowy (Standby/Idle), czeka na przycisk 1

	NOWE ADRESOWANIE przyciskiem 1
 b) Naci?nij przycisk 1:
	W??cz S66 przeka?nikiem i rozpocznij login
	LED 2 ?ó?ty miga        : Login S66 w toku
	LED 3 zielony miga 1/s  : Login S66 udany
	LED 1 czerwony W?       : Login S66 b??dny
	LED ?wieci a? do ponownego naci?ni?cia przycisku 1
 c) Naci?nij przycisk 1 ponownie jako potwierdzenie
	LED 3 zielony W?        : Login Ok, zapisz nowy adr., potwierd? przyciskiem 1
  	LED 1 czerwony W?       : B??d loginu, nast?pnie restart do STANDBY/Idle			
	W ka?dym przypadku potem przeka?nik/S66 WY? i powrót do Standby/Idle

	SPRAWDZENIE ADRESOWANIA przyciskiem 2
 d) Naci?nij przycisk 2, TYLKO w trybie Standby/Idle
 	W??cz S66 przeka?nikiem (S66 ok. 2s W?) i rozpocznij sprawdzanie adr.
	LED 5(poprz. 4) ?ó?ty W?  : Sprawdzanie adr. S66 w toku ...		
	LED 6(poprz. 5) zielony W?: Adres S66 OK (jak prze??cznik adresu)
	LED 4(poprz. 6) czerwony W?: S66 brak odpowiedzi, nie W? lub inny adres
	LED ?wieci a? do ponownego naci?ni?cia przycisku 2
								
************************************************************************/

#include <18F25K42.h>	// u?ywane z bie??cego katalogu C-Compilera
#include "DEF_VAR.h"	// sta?e globalne i zmienne, inicjalizowane na 0
#include "BIOS.h"		// Funkcje podstawowe w tym DALI

/****************** PROGRAM G?ÓWNY ********************************/
void main(void)
{//1
 INIT_PRG();                        // Inicjalizacja
 while(TRUE) /*--------------- OD TEGO MIEJSCA P?TLA NIESKO?CZONA co 100ms -------*/
  {//2
	if (!xT1_FLAG) continue;        // tylko co 100ms

// ### Od tego miejsca tylko co 100ms ###
	xT1_FLAG=0;						// Przygotuj na nast?pne 100ms
	if ((bBlink++)>=10) bBlink=0;	// Migacz LED 1s

	// Pobierz i sprawd? adres z prze??czników obrotowych
	bDALI_Adr=((bPORTC&0x0F)^0x0F)*10 + ((bPORTB&0x0F)^0x0F);
	if (bDALI_Adr>63)				// >63= nieprawid?owy = Reset
	 {//3
		bPORTA &= 0xC0;				// Wszystkie LED WY?
		RB_A.LED1=(bBlink<5);		// CZERWONY 1 Miga: Adr >63
		output_bit(xDO_REL,0);		// RB5: Przeka?nik/S66 WY?
		prg_state=STANDBY;			// Reset
		xDALI_RX_OK=bDALI_RX_Dat=0;	// ewent. wyczy?? ?mieci z bufora DALI
		continue;					// Przerwij while i od razu nowa runda
	 }//3

	// Je?li BRAK b??du Adr., to od tego miejsca w?a?ciwy przebieg programu 
	switch (prg_state)
	 {//3
		case STANDBY:		// ## Krok 1: Stan bezczynno?ci, Narz?dzie W?, S66 WY?
			output_bit(xDO_REL,0);	// Przeka?nik/S66 WY?
			bPORTA &= 0xC0;			// Wszystkie LED WY?
			RB_A.LED2=(bBlink==1);	// ?ó?ty 2 b?yska krótko = Bezczynno??
			bDALI_RX_Wait=0;		// Inicjalizacja TimeOut DALI
			param_cnt=0;
			while(!RB_A.SW1)		// Przycisk 1 i czekaj a? zwolniony
				prg_state=S66_ON;
			while(!RB_A.SW2)		// Przycisk 2 i czekaj a? zwolniony
				prg_state=CHECK_ADR;
			break;

		case S66_ON:		// ## Krok 2: Przycisk 1 naci?ni?ty = S66 W?
			bPORTA &= 0xC0;			// Wszystkie LED WY?
			output_bit(xDO_REL,1);	// RB5: Przeka?nik/S66 W?
			delay_ms(200);			// krótko czekaj a? S66 ok
			prg_state=SEND_LOGIN;
			break;

		case SEND_LOGIN:	// ## Krok 3: Po??cz S66 w tryb parametryzacji
			if (param_cnt<5)		// Wy?lij 5 znaków P,A,R,A,M
			 {//4					// 1 znak na 100ms
				output_toggle(xDO_YLED2);	// ?Ó?TY 1 Miga
				TX_DALI(D_DAP_ADDRESS, K_param[param_cnt]);
				param_cnt++;
				break;
			 }//4
			TX_DALI(D_BROADCAST_ADDRESS, D_QUERY_STATUS); // zapytaj o status
			delay_ms(500);			// krótko czekaj a? S66 odpowie
			if (xDALI_RX_OK)		// Odpowied? od query status (1x 0x50)
			 {//4					// W przeciwnym razie odbierz 0x81
				ErrCode=0;
				bDALI_RX_Wait=0;	// Ustaw TimeOut
				prg_state=WAIT_NEXT;// z LED do nast?pnego kroku
			 }//4
			else
			if ((bDALI_RX_Wait++)>5)// maks. 0,5s czekaj na odpowied? DALI
			 {//4
				ErrCode=1;
				prg_state=WAIT_NEXT;// z LED do nast?pnego kroku
			 }//4
			break;
	
		case WAIT_NEXT:		// ## Krok 4: z wynikiem LED czekaj na przycisk 1
			bPORTA &= 0xC0;			// Wszystkie LED WY?
			if (!ErrCode)	RB_A.LED3=(bBlink<5);	// ZIELONY 1 miga: S66 Login OK
			else			RB_A.LED1=1;			// CZERWONY 1 W? = B??d loginu
			while(!RB_A.SW1)		// Przycisk 1, i czekaj a? zwolniony
			 {//4
				if (!ErrCode)	prg_state=SEND_ADR;
				else			prg_state=STANDBY;
			 }//4
			delay_ms(40);
			break;

	case SEND_ADR:		// ## Krok 5: Wy?lij nowy Adr
			bPORTA &= 0xC0;			// Wszystkie LED WY?
			RB_A.LED2=1;			// ?Ó?TY 1 W?: S66 Wy?lij nowy Adr
			TX_DALI(D_SPECIAL_STORE_DTR0,bDALI_Adr); // wy?lij_adres()
			delay_ms(40);			// krótko czekaj a? S66 odpowie
			if (bDALI_RX_Dat == '+')	// Odpowied? ok
			 {//4
				RB_A.LED3=1;		// ZIELONY 1 W?: S66 Adr OK
				bDALI_RX_Wait=0;	// Ustaw TimeOut
				prg_state = SEND_STORE;	// Zapisz
			 }//4
			else
			if ((bDALI_RX_Wait++)>5)// maks. 0,5s czekaj na odpowied? DALI
			 {//4
				RB_A.LED1=1;		// CZERWONY 1 W? = B??d Adr
				while(RB_A.SW1);	// Czekaj na przycisk 1
				while(!RB_A.SW1);	// i czekaj a? zwolniony
				delay_ms(200);
				prg_state=STANDBY;
			 }//4
 			break;

		case SEND_STORE:	// ## Krok 6: Wy?lij polecenie zapisu
			bPORTA &= 0xC0;			// Wszystkie LED WY?
			TX_DALI(D_BROADCAST_ADDRESS, D_STORE_DTR_AS_SHORT_ADDRESS); // wy?lij_sygna?_zapisu()
									// wysy?ane 2x
			delay_ms(40);			// krótko czekaj a? S66 odpowie
			if (bDALI_RX_Dat == '-')// weryfikuj
			 {//4
				RB_A.LED3=1;		// ZIELONY 1 W?: S66 Zapis OK
				while(RB_A.SW1);	// Czekaj na przycisk 1
				while(!RB_A.SW1);	// i czekaj a? zwolniony
				delay_ms(200);
				prg_state = STANDBY;// i restart
			 }//4
			else
			if ((bDALI_RX_Wait++)>5)// maks. 0,5s czekaj na odpowied? DALI
			 {//4
				RB_A.LED1=1;		// CZERWONY 1 W? = B??d zapisu
				while(RB_A.SW1);	// Czekaj na przycisk 1
				while(!RB_A.SW1);	// i czekaj a? zwolniony
				delay_ms(200);
				prg_state=STANDBY;
			 }//4
 			break;

		// ####### Od tego miejsca tylko sprawdzanie Adr przyciskiem 2 ##########
		case CHECK_ADR:		// ## Krok 7: Przycisk 2: Sprawd? nowy Adr
			bPORTA &= 0xC0;			// Wszystkie LED WY?
			output_bit(xDO_REL,1);	// Przeka?nik/S66 W?
			RB_A.LED4=1;			// tmp ?Ó?TY 5(4) W?: S66 Sprawdzanie Adr trwa ...
			delay_ms(200);			// krótko czekaj a? S66 ok
			TX_DALI((bDALI_Adr<<1 | 1), D_QUERY_STATUS); // zapytaj z nowym Adr.
			// Adres to YAAAAAAS, tutaj zawsze S=1
			delay_ms(40);			// krótko czekaj a? S66 odpowie
			RB_A.LED4=0;			// tmp ?Ó?TY 5(4) WY?
			if (bDALI_RX_Dat>0x7F)	// Odpowied? od query status (0x80 lub 0x81)
			 {//4
				RB_A.LED5=1;		// tmpZIELONY 6(5) W? = Adr OK
				prg_state=DIM_ADR;	// Je?li S66 OK, to krótkie przyciemnienie
			 }//4
			else
			if ((bDALI_RX_Wait++)>5)// maks. 0,5s czekaj na odpowied? DALI
			 {//4
				RB_A.LED6=1;		// tmp CZERWONY 4(6) W? = B??d Adr
				while(RB_A.SW2);	// Czekaj na przycisk 2
				while(!RB_A.SW2);	// i czekaj a? zwolniony
				delay_ms(200);		// ma?a pauza ze wzgl?du na drgania przycisków
				prg_state=STANDBY;
			 }//4
			break;

		case DIM_ADR:		// ## Krok 8: S66 Przyciemnienie w dó? jako test funkcji
			// ARC dla S66 : min=5%=144, 25%=203, 75%=243, 100%=254
			// YAAAAAAS (Y=0=ADR -- Adr -- S=0=DAP)
			TX_DALI((bDALI_Adr<<1), K_DALI_MAX_LEVEL); // 1:Przyciemnij 100%=254
			delay_ms(300);
			TX_DALI((bDALI_Adr<<1), 243); // 2:Przyciemnij 75%
			delay_ms(300);
			TX_DALI((bDALI_Adr<<1), 230); // 2:Przyciemnij 50%
			delay_ms(300);
			TX_DALI((bDALI_Adr<<1), 203); // 2:Przyciemnij 25%
			delay_ms(300);
			TX_DALI((bDALI_Adr<<1), 180); // 2:Przyciemnij ok.10%
			delay_ms(300);
			TX_DALI((bDALI_Adr<<1), K_DALI_MIN_LEVEL); // 2:Przyciemnij 5%=144
			while(RB_A.SW2);	// Czekaj na przycisk 2
			while(!RB_A.SW2);	// i czekaj a? zwolniony
			delay_ms(200);		// ma?a pauza ze wzgl?du na drgania przycisków
			prg_state=STANDBY;
			break;
			
		default:			// ## Ostatni krok: Je?li nieznany Case
			RB_A.LED1=1;			// CZERWONY 1 krótko W?
			delay_ms(200);
			prg_state=STANDBY;		// Reset
			break;

	 }//3		// KONIEC CASE

	xDALI_RX_OK=bDALI_RX_Dat=0;	// ewent. wyczy?? ?mieci z bufora DALI

  }//2		// KONIEC WHILE

}//1	// KONIEC PRG