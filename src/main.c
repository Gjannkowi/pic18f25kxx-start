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

// ============== FUNKCJE DEBUGOWANIA ==============
// Pokazuje wartoœæ bajtu na 6 LED (binarnie) przez okreœlony czas
void DEBUG_ShowByte(unsigned char value, unsigned int ms)
{
    LATA &= 0xC0;           // Wyczyœæ LED 1-6
    LATA |= (value & 0x3F); // Poka¿ 6 dolnych bitów na LED
    delay_ms(ms);
    LATA &= 0xC0; // Wyczyœæ
}

// Pokazuje informacjê o b³êdzie - miga czerwonym LED okreœlon¹ liczbê razy
void DEBUG_Error(unsigned char count)
{
    for (unsigned char i = 0; i < count; i++)
    {
        LATA |= 0x01; // LED1 (CZERWONY)
        delay_ms(200);
        LATA &= 0xC0;
        delay_ms(200);
    }
}

// ============== FUNKCJE DALI ==============
// UWAGA: Przerwania Timer1 i UART RX NIE DZIA£AJ¥ na tym sprzêcie/kompilatorze
// U¿ywamy pollingu w pêtli g³ównej

// Wyœlij ramkê DALI (adres + komenda)
void TX_DALI(unsigned char adr, unsigned char cmd)
{
    // Wyczyœæ bufor RX PRZED wys³aniem (usuniêcie starych danych)
    U1FIFO |= 0x02; // RXBE (bit 1) - Wyczyœæ bufor RX

    // Wyœlij bajt adresu
    U1TXB = adr;
    // Wyœlij bajt komendy
    U1TXB = cmd;

    // Czekaj a¿ transmisja zakoñczona
    while (!(U1ERRIR & 0x80))
        ; // TXMTIF: czekaj a¿ TX gotowe

    // Poczekaj chwilê na zakoñczenie transmisji na linii
    delay_ms(5);

    // Wyczyœæ echo w³asnej transmisji z bufora RX
    U1FIFO |= 0x02; // RXBE (bit 1) - Wyczyœæ bufor RX
}

// Odczytaj dane z UART DALI (zwraca 1 jeœli dane dostêpne, 0 jeœli nie)
unsigned char RX_DALI(unsigned char *data)
{
    // SprawdŸ czy s¹ dane w buforze (RXBE = 0 oznacza dane dostêpne)
    // U1FIFO bit 0 (RXBE) = 0 gdy bufor niepusty
    if (!(U1FIFO & 0x01)) // Bit 0 = RXBE (RX Buffer Empty)
    {
        *data = U1RXB; // Odczytaj dane
        return 1;      // Dane odebrane
    }
    return 0; // Brak danych
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

        // Sprawdzaj odbiór DALI w ka¿dym cyklu (zastêpuje przerwania)
        unsigned char rx_data;
        if (RX_DALI(&rx_data))
        {
            bDALI_RX_Dat = rx_data;
            xDALI_RX_OK = 1;
        }

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

            // SprawdŸ przycisk 1 (aktywny niski - RA7)
            // Czekaj dopóki jest wciœniêty, potem zmieñ stan
            if (!(PORTA & 0x80)) // Bit 7 = 0 (LOW) = wciœniêty
            {
                // Czekaj a¿ przycisk zwolniony
                while (!(PORTA & 0x80))
                    ;
                prg_state = S66_ON;
            }

            // SprawdŸ przycisk 2 (aktywny niski - RA6)
            if (!(PORTA & 0x40)) // Bit 6 = 0 (LOW) = wciœniêty
            {
                // Czekaj a¿ przycisk zwolniony
                while (!(PORTA & 0x40))
                    ;
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
                LATA &= 0xC0;
                LATA |= 0x02; // LED2 (¯Ó£TY) W£ podczas wysy³ania

                // DEBUG: Poka¿ który znak wysy³amy (1-5 = LED kombinacje)
                if (param_cnt == 0)
                    LATA |= 0x01; // P: LED1+2
                if (param_cnt == 1)
                    LATA |= 0x04; // A: LED2+3
                if (param_cnt == 2)
                    LATA |= 0x08; // R: LED2+4
                if (param_cnt == 3)
                    LATA |= 0x10; // A: LED2+5
                if (param_cnt == 4)
                    LATA |= 0x20; // M: LED2+6

                TX_DALI(D_DAP_ADDRESS, K_param[param_cnt]);
                delay_ms(50); // Krótkie opóŸnienie po wys³aniu
                param_cnt++;
                break;
            }

            // Po wys³aniu wszystkich 5 znaków, czekaj na odpowiedŸ 'P'
            if (param_cnt == 5)
            {
                delay_ms(200); // Daj lampie wiêcej czasu na odpowiedŸ

                // SprawdŸ czy s¹ dane w buforze
                unsigned char rx_tmp;
                if (RX_DALI(&rx_tmp))
                {
                    bDALI_RX_Dat = rx_tmp;
                    xDALI_RX_OK = 1;
                }

                param_cnt = 6; // Oznacz ¿e sprawdziliœmy
            }

            // DEBUG: Pokazuj co odebrano przez 2 sekundy
            if (param_cnt == 6 && bDALI_RX_Wait == 0)
            {
                LATA &= 0xC0;
                // Pokazuj wartoœæ odbieran¹ jako kombinacjê LED
                LATA |= (bDALI_RX_Dat & 0x3F);
                delay_ms(2000);
                LATA &= 0xC0;
                bDALI_RX_Wait = 1; // Oznacz ¿e pokazaliœmy
            }

            // SprawdŸ czy lampa odpowiedzia³a 'P' (0x50 = 80 dec)
            if (bDALI_RX_Dat == 'P') // Lampa odpowiada 'P' po poprawnej sekwencji PARAM
            {
                ErrCode = 0; // Login OK
                xDALI_RX_OK = 0;
                bDALI_RX_Dat = 0;
                prg_state = WAIT_NEXT;
            }
            else if (bDALI_RX_Wait > 15) // Timeout 1.5s total
            {
                ErrCode = 1; // Brak odpowiedzi lub b³êdna
                prg_state = WAIT_NEXT;
            }
            else if (param_cnt == 6)
            {
                bDALI_RX_Wait++;
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

            // Czekaj na przycisk 1 (RA7, aktywny niski)
            if (!(PORTA & 0x80)) // Przycisk wciœniêty
            {
                // Czekaj a¿ przycisk zwolniony
                while (!(PORTA & 0x80))
                    ;
                delay_ms(40); // Debouncing
                state_counter = 0;
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

        case SEND_ADR:    // ## Krok 5: Wyœlij nowy Adr do DTR0
            LATA &= 0xC0; // Wszystkie LED WY£
            LATA |= 0x02; // LED2 (¯Ó£TY) W£: Wysy³anie adresu

            // DEBUG: Poka¿ wysy³any adres (6 dolnych bitów)
            LATA |= (bDALI_Adr & 0x3F);
            delay_ms(1000); // Poka¿ przez 1s
            LATA &= 0xC0;

            // WA¯NE: W trybie PARAM komenda specjalna DTR0 (0xA3)
            // wysy³ana jest jako DAP (adres 0xFE) + DTR0 (0xA3) + wartoœæ
            // Zgodnie z firmware lampy: case DTR0: bDaliDTR = value;
            TX_DALI(D_SPECIAL_STORE_DTR0, bDALI_Adr);
            delay_ms(200); // Czekaj na odpowiedŸ '+'

            // SprawdŸ odpowiedŸ
            unsigned char rx_check;
            xDALI_RX_OK = 0;
            bDALI_RX_Dat = 0;
            if (RX_DALI(&rx_check))
            {
                bDALI_RX_Dat = rx_check;
                xDALI_RX_OK = 1;

                // DEBUG: Poka¿ odpowiedŸ przez 2s
                LATA &= 0xC0;
                LATA |= (rx_check & 0x3F);
                delay_ms(2000);
                LATA &= 0xC0;
            }
            else
            {
                // DEBUG: Brak odpowiedzi - mignij czerwonym
                LATA |= 0x01;
                delay_ms(500);
                LATA &= 0xC0;
            }
            bDALI_RX_Wait = 0;

            // DTR0 ustawiony - przejdŸ do zapisania adresu
            prg_state = SEND_STORE;
            break;

        case SEND_STORE:  // ## Krok 6: Wyœlij polecenie zapisu (2x!)
            LATA &= 0xC0; // Wszystkie LED WY£
            LATA |= 0x10; // LED5 (¯Ó£TY) W£: Zapisywanie
            delay_ms(500);

            // WA¯NE: Komendy konfiguracyjne musz¹ byæ wys³ane 2x!
            // W trybie PARAM wysy³amy do BROADCAST (0xFF), poniewa¿:
            // - Lampa jest ju¿ w trybie PARAM (bParam = 1)
            // - Tylko ona odpowie, bo tylko ona ma bParam = 1
            // - Komenda musi mieæ selector bit = 1 (komenda), wiêc: 0xFF | 0x01 = 0xFF
            LATA |= 0x08;                                // LED4 W£ podczas 1. wysy³ki
            TX_DALI(0xFF, D_STORE_DTR_AS_SHORT_ADDRESS); // 0xFF = broadcast z selector=1
            delay_ms(100);

            LATA &= 0xC0;
            LATA |= 0x10;                                // LED5 W£
            LATA |= 0x20;                                // LED6 W£ podczas 2. wysy³ki
            TX_DALI(0xFF, D_STORE_DTR_AS_SHORT_ADDRESS); // 2x!
            delay_ms(200);                               // Daj czas na zapisanie w EEPROM

            // SprawdŸ odpowiedŸ '-'
            {
                unsigned char rx_store;
                bDALI_RX_Dat = 0;
                if (RX_DALI(&rx_store))
                {
                    bDALI_RX_Dat = rx_store;

                    // DEBUG: Poka¿ odpowiedŸ przez 2s
                    LATA &= 0xC0;
                    LATA |= (rx_store & 0x3F);
                    delay_ms(2000);
                    LATA &= 0xC0;
                }
                else
                {
                    // DEBUG: Brak odpowiedzi - mignij 3x czerwonym
                    for (unsigned char i = 0; i < 3; i++)
                    {
                        LATA |= 0x01;
                        delay_ms(300);
                        LATA &= 0xC0;
                        delay_ms(300);
                    }
                }
            }

            // Zapis zakoñczony - poka¿ sukces
            LATA &= 0xC0;
            LATA |= 0x04; // LED3 (ZIELONY) W£: Zapis OK

            // Czekaj na przycisk 1 aby kontynuowaæ
            while (PORTA & 0x80)
                ; // Czekaj na przycisk 1
            while (!(PORTA & 0x80))
                ; // i czekaj a¿ zwolniony
            delay_ms(200);

            LATB &= ~(1 << 5);   // Wy³¹cz przekaŸnik
            prg_state = STANDBY; // Wróæ do STANDBY
            break;

        case CHECK_ADR:       // ## Krok 7: SprawdŸ nowy Adr (UPROSZCZONA WERSJA)
            LATA &= 0xC0;     // Wszystkie LED WY£
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£
            LATA |= 0x10;     // LED5 (¯Ó£TY) W£: Sprawdzanie Adr trwa...
            delay_ms(200);    // Czekaj a¿ S66 gotowe

            // Wyczyœæ bufor przed wys³aniem
            U1FIFO |= 0x02; // RXBE (bit 1) - Wyczyœæ bufor RX

            // Wyœlij QUERY_STATUS do danego adresu
            TX_DALI((unsigned char)((bDALI_Adr << 1) | 1), D_QUERY_STATUS);

            // Czekaj na odpowiedŸ (maksymalnie 100ms)
            bDALI_RX_Dat = 0;
            xDALI_RX_OK = 0;
            {
                unsigned char timeout = 0;
                unsigned char rx_data = 0;
                while (timeout < 100) // Timeout 100ms
                {
                    if (RX_DALI(&rx_data))
                    {
                        bDALI_RX_Dat = rx_data;
                        xDALI_RX_OK = 1;
                        break;
                    }
                    delay_ms(1);
                    timeout++;
                }
            }

            // DEBUG: Poka¿ wartoœæ przez 1 sekundê
            LATA &= 0xC0;
            LATA |= (bDALI_RX_Dat & 0x3F);
            delay_ms(1000);
            LATA &= 0xC0;

            if (xDALI_RX_OK && bDALI_RX_Dat >= 0x80) // OdpowiedŸ OK (0x80 lub wy¿sza)
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

        default:          // ## Ostatni krok: Nieznany Case
            LATA |= 0x01; // LED1 (CZERWONY) krótko W£
            delay_ms(200);
            prg_state = STANDBY;
            break;
        }
    }
}