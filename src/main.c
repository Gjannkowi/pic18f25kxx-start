#pragma config FEXTOSC = OFF // Wybór trybu zewnêtrznego oscylatora (oscylator wy³¹czony)
#pragma config WDTE = OFF    // Tryb pracy Watchdog (wy³¹czony)
#pragma config LVP = OFF     // Programowanie niskonapiêciowe wy³¹czone
#include <xc.h>
#include <pic18f25k42.h>

// W³asne definicje rejestrów na wypadek problemów z plikami nag³ówkowymi
#ifndef PORTA
#define PORTA (*(volatile unsigned char *)0xFCA)
#endif
#ifndef LATA
#define LATA (*(volatile unsigned char *)0xFBA)
#endif
#ifndef LATB
#define LATB (*(volatile unsigned char *)0xFBB)
#endif
#ifndef PORTB
#define PORTB (*(volatile unsigned char *)0xFCB)
#endif
#ifndef PORTC
#define PORTC (*(volatile unsigned char *)0xFCC)
#endif

#define _XTAL_FREQ 4000000UL // 4MHz zgodnie z orygina³em CCS

#ifndef OSCFRQ
#define OSCFRQ (*(volatile unsigned char *)0x39DF)
#endif
#ifndef ANSELA
#define ANSELA (*(volatile unsigned char *)0x3A40)
#endif
#ifndef ANSELB
#define ANSELB (*(volatile unsigned char *)0x3A48)
#endif
#ifndef ANSELC
#define ANSELC (*(volatile unsigned char *)0x3A50)
#endif
#ifndef ODCONA
#define ODCONA (*(volatile unsigned char *)0x3A42)
#endif
#ifndef ODCONA
#define ODCONA (*(volatile unsigned char *)0x3A42)
#endif
#ifndef ODCONB
#define ODCONB (*(volatile unsigned char *)0x3A4A)
#endif
#ifndef TRISA
#define TRISA (*(volatile unsigned char *)0x3FC2)
#endif
#ifndef TRISB
#define TRISB (*(volatile unsigned char *)0x3FC3)
#endif
#ifndef TRISC
#define TRISC (*(volatile unsigned char *)0x3FC4)
#endif
#ifndef WPUA
#define WPUA (*(volatile unsigned char *)0x3A41)
#endif
#ifndef WPUB
#define WPUB (*(volatile unsigned char *)0x3A49)
#endif
#ifndef WPUC
#define WPUC (*(volatile unsigned char *)0x3A51)
#endif

// Rejestry UART1 dla DALI
#ifndef U1CON0
#define U1CON0 (*(volatile unsigned char *)0x3AB0)
#endif
#ifndef U1CON1
#define U1CON1 (*(volatile unsigned char *)0x3AB1)
#endif
#ifndef U1CON2
#define U1CON2 (*(volatile unsigned char *)0x3AB2)
#endif
#ifndef U1BRGL
#define U1BRGL (*(volatile unsigned char *)0x3AB3)
#endif
#ifndef U1BRGH
#define U1BRGH (*(volatile unsigned char *)0x3AB4)
#endif
#ifndef U1FIFO
#define U1FIFO (*(volatile unsigned char *)0x3AB5)
#endif
#ifndef U1UIR
#define U1UIR (*(volatile unsigned char *)0x3AB6)
#endif
#ifndef U1ERRIR
#define U1ERRIR (*(volatile unsigned char *)0x3AB7)
#endif
#ifndef U1ERRIE
#define U1ERRIE (*(volatile unsigned char *)0x3AB8)
#endif
#ifndef U1RXB
#define U1RXB (*(volatile unsigned char *)0x3AC0)
#endif
#ifndef U1TXB
#define U1TXB (*(volatile unsigned char *)0x3AC1)
#endif
#ifndef U1RXBE
#define U1RXBE (*(volatile unsigned char *)0x3AC2)
#endif
#ifndef U1TXBE
#define U1TXBE (*(volatile unsigned char *)0x3AC4)
#endif
#ifndef RC6PPS
#define RC6PPS (*(volatile unsigned char *)0x3A7E)
#endif
#ifndef U1RXPPS
#define U1RXPPS (*(volatile unsigned char *)0x3AE8)
#endif
#ifndef U1P1L
#define U1P1L (*(volatile unsigned char *)0x3ABA)
#endif
#ifndef U1P1H
#define U1P1H (*(volatile unsigned char *)0x3ABB)
#endif
#ifndef U1P2L
#define U1P2L (*(volatile unsigned char *)0x3ABC)
#endif
#ifndef U1P2H
#define U1P2H (*(volatile unsigned char *)0x3ABD)
#endif

// Rejestry Timer1 dla timeout
#ifndef T1CON
#define T1CON (*(volatile unsigned char *)0x39CF)
#endif
#ifndef T1CLK
#define T1CLK (*(volatile unsigned char *)0x39D0)
#endif
#ifndef TMR1H
#define TMR1H (*(volatile unsigned char *)0x39D2)
#endif
#ifndef TMR1L
#define TMR1L (*(volatile unsigned char *)0x39D1)
#endif
#ifndef PIR3
#define PIR3 (*(volatile unsigned char *)0x39A3)
#endif
#ifndef PIE3
#define PIE3 (*(volatile unsigned char *)0x39B3)
#endif
#ifndef INTCON
#define INTCON (*(volatile unsigned char *)0x3FD2)
#endif
#ifndef CLKRCLK
#define CLKRCLK (*(volatile unsigned char *)0x39C5)
#endif

// Struktura bitowa dla PORTA
struct
{
    unsigned char LED1 : 1; // RA0 - CZERWONY: B³êdny Adr >63
    unsigned char LED2 : 1; // RA1 - ¯Ó£TY: START po³¹czenia
    unsigned char LED3 : 1; // RA2 - ZIELONY: Online
    unsigned char LED4 : 1; // RA3 - CZERWONY: Start adresowania
    unsigned char LED5 : 1; // RA4 - ¯Ó£TY: Adres ok
    unsigned char LED6 : 1; // RA5 - ZIELONY: NIEPOWODZENIE
    unsigned char SW2 : 1;  // RA6 - Przycisk 2
    unsigned char SW1 : 1;  // RA7 - Przycisk 1
} RB_A;

// ============== ZMIENNE GLOBALNE ==============
unsigned char state_counter = 0; // Licznik dla op\u00f3\u017anie\u0144 w stanach (100ms * state_counter)

// ============== DEFINICJE KOMEND DALI ==============
#define D_SPECIAL_STORE_DTR0 0xA3
#define D_BROADCAST_ADDRESS 0xFF
#define D_DAP_ADDRESS 0xFE
#define D_STORE_DTR_AS_SHORT_ADDRESS 0x80
#define D_QUERY_STATUS 0x90 // Zg³oœ stan do mastera
#define D_QUERY_CONTENT_DTR 0x98
#define D_OFF 0x00              // Lampa natychmiast wy³¹czona
#define D_RECALL_MAX_LEVEL 0x05 // Wywo³aj poziom max
#define D_RECALL_MIN_LEVEL 0x06

// ============== DEFINICJE DANYCH DALI ==============
#define K_DALI_MIN_LEVEL 144 // 144 -> 5% PWM
#define K_DALI_MAX_LEVEL 254

// ============== DEFINICJE CZASOWE ==============
#define K_T1_NUM_MS 100                                   // Timer1 = 100ms
#define K_T1_SET (65535UL - (K_T1_NUM_MS * 1000UL / 8UL)) // Wartoœæ wstêpna Timer1 (unsigned long)

// Stany programu
enum StateEnum
{
    STANDBY,
    S66_ON,
    SEND_LOGIN,
    WAIT_NEXT,
    SEND_ADR,
    SEND_STORE,
    SHOW_RESULT, // Nowy stan - pokazywanie wyniku
    CHECK_ADR,   // Czekanie 200ms przed sprawdzeniem
    DIM_ADR,     // Sprawdzanie odpowiedzi QUERY_STATUS
    DIM_TEST     // Test œciemniania
};

// Zmienne globalne
volatile enum StateEnum prg_state = STANDBY;
volatile unsigned char bBlink = 0;
volatile unsigned char bDALI_Adr = 0;
unsigned char ErrCode = 0;

// Zmienne DALI
volatile unsigned char xT1_FLAG = 0;      // Flaga Timer1 100ms
volatile unsigned char xDALI_RX_OK = 0;   // Status odbioru RX (1=OK)
volatile unsigned char bDALI_RX_Dat = 0;  // Dane odebrane z DALI
volatile unsigned char bDALI_RX_Wait = 0; // Timeout odbioru DALI
unsigned char param_cnt = 0;              // Licznik sekwencji logowania

// Sekwencja logowania PARAM
const char K_param[5] = {'P', 'A', 'R', 'A', 'M'};

// Prosty delay w milisekundach
void delay_ms(unsigned int ms)
{
    while (ms--)
    {
        __delay_ms(1);
    }
}

// ============== FUNKCJE DALI ==============
// UWAGA: Przerwania Timer1 i UART RX NIE DZIA£AJ¥ na tym sprzêcie/kompilatorze
// U¿ywamy pollingu w pêtli g³ównej

// Wyœlij ramkê DALI (adres + komenda)
void TX_DALI(unsigned char adr, unsigned char cmd)
{
    // Wyœlij bajt adresu
    U1TXB = adr;
    // Wyœlij bajt komendy
    U1TXB = cmd;

    // Czekaj a¿ transmisja zakoñczona
    while (!(U1ERRIR & 0x80))
        ; // TXMTIF: czekaj a¿ TX gotowe

    // Wyczyœæ bufory TX i RX
    U1FIFO |= 0x22; // TXBE (bit 5) + RXBE (bit 1) - Wyczyœæ oba
}

// Inicjalizacja UART1 dla protoko³u DALI
void INIT_DALI(void)
{
    // Konfiguracja UART1 dla DALI (1200 baud, Manchester encoding)
    U1CON0 = 0x38; // Bit 7:6=00 BRGS+ABDEN, 5:4=11 Tx+Rx w³., 3:0=1000 Urz¹dzenie DALI

    // Pó³okresy bitu bezczynnoœci miêdzy ramkami
    U1P1H = 0x00;
    U1P1L = 0x16; // =22 pó³okresy
    U1P2H = 0x00;
    U1P2L = 0x16; // =22 pó³okresy

    // Baud rate dla 1200 baud przy 4MHz
    U1BRGH = 0x00;
    U1BRGL = 0xCB; // Dostosowane dla S66

    // Konfiguracja polaryzacji i bitów stopu
    U1CON2 = 0x64; // Bit6=1 odwróæ RXPOL, 5:4=10 2 bity stopu, 2=1 odwróæ TXPOL

    // Mapowanie pinów
    RC6PPS = 0x13;  // RC6 = TX
    U1RXPPS = 0x17; // RC7 = RX

    // W³¹cz UART
    U1CON1 = 0x80; // Bit7=1 W³¹czenie portu szeregowego
}

// Inicjalizacja Timer1 dla czasowania 100ms
void INIT_TIMER1(void)
{
    // Wy³¹cz Timer1 przed konfiguracj¹
    T1CON = 0x00;

    // Wyczyœæ flagê przed konfiguracj¹
    PIR3 &= ~0x01; // Wyczyœæ TMR1IF

    // Ustaw wartoœæ pocz¹tkow¹ dla 100ms
    TMR1H = (unsigned char)((K_T1_SET >> 8) & 0xFF);
    TMR1L = (unsigned char)(K_T1_SET & 0xFF);

    // Konfiguracja Timer1: FOSC/4, prescaler 1:8, 16-bit mode
    T1CLK = 0x01;       // FOSC/4 jako Ÿród³o zegara (4MHz/4 = 1MHz)
    T1CON = 0b00110001; // Bit5:4=11 (prescaler 1:8), Bit0=1 (w³¹cz)
                        // 1MHz / 8 = 125kHz, 12500 ticks = 100ms

    // Przerwania NIE DZIA£AJ¥ - u¿ywamy pollingu
}

void main(void)
{
    // Konfiguracja oscylatora na 4MHz (HFINTOSC)
    OSCFRQ = 0b00000010;  // Ustawienie czêstotliwoœci wewnêtrznego oscylatora na 4 MHz
    CLKRCLK = 0b00000001; // Bit 0=1: HFINTOSC jako Ÿród³o zegara dla peryferiów

    ANSELA = 0x00; // Wy³¹czenie wejœæ analogowych na wszystkich pinach portu A
    ANSELB = 0x00; // Wy³¹czenie wejœæ analogowych na porcie B
    ANSELC = 0x00; // Wy³¹czenie wejœæ analogowych na porcie C
    ODCONA = 0x00; // Wy³¹czenie trybu open-drain
    ODCONB = 0x00; // Wy³¹czenie trybu open-drain dla portu B

    // Ustaw kierunek portów
    TRISA = 0b11000000; // RA0-RA5 jako wyjœcia (LED), RA6-RA7 jako wejœcia (przyciski)
    TRISB = 0b00001111; // RB5=WYJ (przekaŸnik), RB3:0=WEJ (prze³¹czniki adresu)
    TRISC = 0b10001111; // RC7=RX-WEJ, RC6=TX-WYJ, RC3:0=WEJ (prze³¹czniki adresu)

    // W³¹cz wewnêtrzne rezystory podci¹gaj¹ce
    WPUA = 0xC0; // RA6 i RA7 (przyciski)
    WPUB = 0x0F; // RB3:0 (prze³¹czniki)
    WPUC = 0x0F; // RC3:0 (prze³¹czniki)

    // Ustaw stan pocz¹tkowy
    LATA = 0x00; // Wszystkie LED wy³¹czone
    LATB = 0x00; // PrzekaŸnik RB5 WY£

    // Inicjalizacja protoko³u DALI i Timer1
    INIT_DALI();
    INIT_TIMER1();

    // Test LED - krótkie b³yœniêcie wszystkich LED na start (1 sekunda)
    LATA = 0x3F; // W³¹cz wszystkie LED (RA0-RA5)
    delay_ms(1000);
    LATA = 0x00; // Wy³¹cz wszystkie LED
    delay_ms(200);

    // G³ówna pêtla
    while (1)
    {
        // Wykrywanie przepe³nienia Timer1 (bez u¿ywania flagi TMR1IF)
        static unsigned char last_tmr1h = 0xFF;
        unsigned char current_tmr1h = TMR1H;

        // Wykryj przepe³nienie: TMR1H skoczy³ z wysoka (>0xE0) na nisko (<0x20)
        if (last_tmr1h > 0xE0 && current_tmr1h < 0x20)
        {
            // Przepe³nienie! Reload Timer1 i ustaw flagê
            TMR1H = (unsigned char)((K_T1_SET >> 8) & 0xFF);
            TMR1L = (unsigned char)(K_T1_SET & 0xFF);
            xT1_FLAG = 1;
        }
        last_tmr1h = current_tmr1h;

        // Wykonuj kod tylko co 100ms gdy Timer1 przepe³ni siê
        if (!xT1_FLAG)
            continue;

        // ### Od tego miejsca tylko co 100ms ###
        xT1_FLAG = 0; // Wyczyœæ flagê na nastêpne 100ms

        // Aktualizuj strukturê RB_A na podstawie PORTA
        *((unsigned char *)&RB_A) = PORTA;

        // Migacz LED 1s (10 x 100ms)
        if (++bBlink >= 10)
            bBlink = 0;

        // Pobierz i sprawdŸ adres z prze³¹czników obrotowych
        bDALI_Adr = ((PORTC & 0x0F) ^ 0x0F) * 10 + ((PORTB & 0x0F) ^ 0x0F);

        if (bDALI_Adr > 63) // >63 = nieprawid³owy = Reset
        {
            LATA &= 0xC0;   // Wszystkie LED WY£
            if (bBlink < 5) // 0.5s ON / 0.5s OFF
            {
                LATA |= 0x01; // LED1 (CZERWONY) Miga: Adr >63
            }
            LATB &= ~(1 << 5);   // RB5: PrzekaŸnik/S66 WY£
            prg_state = STANDBY; // Reset
            continue;
        }

        // W³aœciwy przebieg programu
        switch (prg_state)
        {
        case STANDBY:          // ## Krok 1: Stan bezczynnoœci
            LATB &= ~(1 << 5); // RB5: PrzekaŸnik/S66 WY£
            LATA &= 0xC0;      // Wszystkie LED WY£
            if (bBlink == 1)   // Krótkie b³yœniêcie
            {
                LATA |= 0x02; // LED2 (¯Ó£TY) b³yska = Bezczynnoœæ
            }
            bDALI_RX_Wait = 0; // Inicjalizacja TimeOut DALI
            param_cnt = 0;
            xDALI_RX_OK = 0; // Reset flagi odbioru
            bDALI_RX_Dat = 0;
            state_counter = 0; // Reset licznika stanu

            // SprawdŸ przycisk 1 (aktywny niski)
            if (!RB_A.SW1)
            {
                prg_state = S66_ON;
            }

            // SprawdŸ przycisk 2 (aktywny niski)
            if (!RB_A.SW2)
            {
                prg_state = CHECK_ADR;
            }
            break;

        case S66_ON:          // ## Krok 2: Przycisk 1 naciœniêty
            LATA &= 0xC0;     // Wszystkie LED WY£
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£

            // Czekaj 2 cykle (200ms) a¿ S66 gotowe
            if (++state_counter >= 2)
            {
                state_counter = 0;
                param_cnt = 0;     // Reset licznika PARAM
                xDALI_RX_OK = 0;   // Reset flagi odbioru przed wysy³aniem
                bDALI_RX_Dat = 0;  // Reset danych
                bDALI_RX_Wait = 0; // Reset timeoutu
                prg_state = SEND_LOGIN;
            }
            break;

        case SEND_LOGIN:       // ## Krok 3: Po³¹cz S66 w tryb parametryzacji
            if (param_cnt < 5) // Wyœlij 5 znaków P,A,R,A,M (1 znak na 100ms)
            {
                LATA ^= 0x02; // LED2 (¯Ó£TY) prze³¹cz (miga)
                TX_DALI(D_DAP_ADDRESS, K_param[param_cnt]);
                param_cnt++;
                break;
            }

            // Wyœlij QUERY_STATUS tylko raz (gdy param_cnt == 5)
            if (param_cnt == 5)
            {
                TX_DALI(D_BROADCAST_ADDRESS, D_QUERY_STATUS);
                param_cnt = 6;     // Ustaw na 6 ¿eby wiêcej nie wysy³aæ
                bDALI_RX_Wait = 0; // Reset timeoutu - liczy od teraz
                break;             // Przerwij i poczekaj na odpowiedŸ w nastêpnym cyklu
            }

            // Czekaj minimum 5 cykli (500ms) przed sprawdzeniem odpowiedzi
            // (tak jak w oryginale delay_ms(500))
            if (bDALI_RX_Wait < 5)
            {
                bDALI_RX_Wait++;
                break; // Czekaj dalej
            }

            // Po 500ms sprawdŸ odpowiedŸ
            if (xDALI_RX_OK && bDALI_RX_Dat >= 0x80) // Prawid³owa odpowiedŸ (0x80-0xFF)
            {
                ErrCode = 0; // Login OK
                prg_state = WAIT_NEXT;
            }
            else if (bDALI_RX_Wait > 10) // Timeout 1s total (10 cykli od wys³ania)
            {
                ErrCode = 1; // Brak odpowiedzi lub b³êdna
                prg_state = WAIT_NEXT;
            }
            else
            {
                bDALI_RX_Wait++; // Liczy dalej do 10
            }
            break;

        case WAIT_NEXT:       // ## Krok 4: Wynik z LED, czekaj na przycisk 1
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£ (utrzymaj)
            LATA &= 0xC0;     // Wszystkie LED WY£

            // Inicjalizacja timeoutu przy pierwszym wejœciu
            if (state_counter == 0)
            {
                state_counter = 1; // Oznacz ¿e ju¿ zainicjalizowano
            }

            if (!ErrCode)
            {
                if (bBlink < 5) // 0.5s ON / 0.5s OFF
                {
                    LATA |= 0x04; // LED3 (ZIELONY) miga: Login OK
                }
            }
            else
            {
                LATA |= 0x01; // LED1 (CZERWONY) W£ = B³¹d loginu
            }

            // Czekaj na przycisk 1 lub timeout
            if (!RB_A.SW1)
            {
                state_counter = 0; // Reset
                if (!ErrCode)
                {
                    prg_state = SEND_ADR;
                }
                else
                {
                    LATB &= ~(1 << 5); // RB5: PrzekaŸnik WY£
                    prg_state = STANDBY;
                }
            }
            else if (state_counter > 100) // Timeout 10s (100 cykli po 100ms)
            {
                state_counter = 0;
                LATB &= ~(1 << 5);
                prg_state = STANDBY;
            }
            else
            {
                state_counter++; // Licz czas
            }
            break;

        case SEND_ADR:    // ## Krok 5: Wyœlij nowy Adr
            LATA &= 0xC0; // Wszystkie LED WY£
            LATA |= 0x02; // LED2 (¯Ó£TY) W£: Wysy³anie adresu

            // Wyœlij tylko raz (gdy bDALI_RX_Wait == 0)
            if (bDALI_RX_Wait == 0)
            {
                TX_DALI(D_SPECIAL_STORE_DTR0, bDALI_Adr);
            }

            bDALI_RX_Wait++; // Inkrementuj w ka¿dym cyklu

            if (bDALI_RX_Dat == '+') // OdpowiedŸ OK
            {
                LATA |= 0x04; // LED3 (ZIELONY) W£: Adr OK
                bDALI_RX_Wait = 0;
                prg_state = SEND_STORE;
            }
            else if (bDALI_RX_Wait > 5) // Timeout 500ms
            {
                LATA &= 0xC0;
                LATA |= 0x01;       // LED1 (CZERWONY) W£ = B³¹d Adr
                state_counter = 10; // 10 cykli = 1s
                prg_state = SHOW_RESULT;
            }
            break;

        case SEND_STORE:  // ## Krok 6: Wyœlij polecenie zapisu
            LATA &= 0xC0; // Wszystkie LED WY£

            // Wyœlij tylko raz (gdy bDALI_RX_Wait == 0)
            if (bDALI_RX_Wait == 0)
            {
                TX_DALI(D_BROADCAST_ADDRESS, D_STORE_DTR_AS_SHORT_ADDRESS);
            }

            bDALI_RX_Wait++; // Inkrementuj w ka¿dym cyklu

            if (bDALI_RX_Dat == '-') // Weryfikuj odpowiedŸ
            {
                LATA |= 0x04;       // LED3 (ZIELONY) W£: Zapis OK
                state_counter = 20; // 20 cykli = 2s
                prg_state = SHOW_RESULT;
            }
            else if (bDALI_RX_Wait > 5) // Timeout 500ms
            {
                LATA |= 0x01;       // LED1 (CZERWONY) W£ = B³¹d zapisu
                state_counter = 10; // 10 cykli = 1s
                prg_state = SHOW_RESULT;
            }
            break;

        case SHOW_RESULT: // ## Stan pomocniczy: Pokazywanie wyniku przez okreœlony czas
            // LED ju¿ ustawiona w poprzednim stanie
            if (--state_counter == 0)
            {
                LATB &= ~(1 << 5); // Wy³¹cz przekaŸnik
                prg_state = STANDBY;
            }
            break;

        case CHECK_ADR:       // ## Krok 7: SprawdŸ nowy Adr (zgodnie z orygina³em)
            LATA &= 0xC0;     // Wszystkie LED WY£
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£
            LATA |= 0x10;     // LED5 (¯Ó£TY) W£: Sprawdzanie Adr trwa...
            delay_ms(200);    // Czekaj a¿ S66 gotowe

            // Wyczyœæ przed wys³aniem
            bDALI_RX_Dat = 0;
            xDALI_RX_OK = 0;

            TX_DALI((unsigned char)((bDALI_Adr << 1) | 1), D_QUERY_STATUS);

            // Aktywnie sprawdzaj odpowiedŸ przez 100ms
            {
                unsigned char timeout = 100; // 100 cykli po 1ms
                while (timeout-- > 0)
                {
                    delay_ms(1);
                    if (PIR3 & 0x08) // U1RXIF - dane przysz³y
                    {
                        bDALI_RX_Dat = U1RXB; // Odczytaj
                        xDALI_RX_OK = 1;
                        PIR3 &= ~0x08; // Wyczyœæ flagê
                        break;
                    }
                }
            }

            // DEBUG: Wyœwietl wartoœæ bDALI_RX_Dat na LEDach
            LATA &= 0xC0;
            LATA |= (bDALI_RX_Dat & 0x3F); // 6 dolnych bitów na LED0-5
            delay_ms(2000);                // Trzymaj 2 sekundy

            // Wyczyœæ LED
            LATA &= 0xC0;

            if (bDALI_RX_Dat > 0x7F) // OdpowiedŸ OK (0x80 lub 0x81)
            {
                LATA |= 0x20;        // LED6 (ZIELONY) W£ = Adr OK
                prg_state = DIM_ADR; // PrzejdŸ do œciemniania
            }
            else // Brak prawid³owej odpowiedzi
            {
                LATA |= 0x08; // LED4 (CZERWONY) W£ = B³¹d Adr
                // Czekaj na przycisk S2 aby wróciæ
                while (!(PORTA & 0x40)) // Czekaj a¿ S2 zwolniony
                    ;
                while (PORTA & 0x40) // Czekaj na naciœniêcie S2
                    ;
                delay_ms(200);
                LATB &= ~(1 << 5); // Wy³¹cz przekaŸnik
                prg_state = STANDBY;
            }
            break;

        case DIM_ADR: // ## Krok 8: S66 Œciemnianie jako test funkcji (zgodnie z orygina³em)
            // LED6 (ZIELONY) ju¿ œwieci - ustawiony w CHECK_ADR
            // ARC dla S66: min=5%=144, 25%=203, 75%=243, 100%=254
            // YAAAAAAS (Y=0=ADR -- Adr -- S=0=DAP)
            TX_DALI((unsigned char)(bDALI_Adr << 1), K_DALI_MAX_LEVEL); // 100%=254
            delay_ms(300);
            TX_DALI((unsigned char)(bDALI_Adr << 1), 243); // 75%
            delay_ms(300);
            TX_DALI((unsigned char)(bDALI_Adr << 1), 230); // 50%
            delay_ms(300);
            TX_DALI((unsigned char)(bDALI_Adr << 1), 203); // 25%
            delay_ms(300);
            TX_DALI((unsigned char)(bDALI_Adr << 1), 180); // ~10%
            delay_ms(300);
            TX_DALI((unsigned char)(bDALI_Adr << 1), K_DALI_MIN_LEVEL); // 5%=144

            // Czekaj na przycisk S2 aby wróciæ
            while (!(PORTA & 0x40)) // Czekaj a¿ S2 zwolniony
                ;
            while (PORTA & 0x40) // Czekaj na naciœniêcie S2
                ;
            delay_ms(200);
            LATB &= ~(1 << 5); // Wy³¹cz przekaŸnik
            prg_state = STANDBY;
            break;

        default:               // ## Ostatni krok: Nieznany Case
            LATA |= 0x01;      // LED1 (CZERWONY) krótko W£
            state_counter = 2; // 200ms
            prg_state = SHOW_RESULT;
            break;
        }

        // Wyczyœæ œmieci z bufora DALI (jak w oryginale)
        xDALI_RX_OK = 0;
        bDALI_RX_Dat = 0;
    }
}