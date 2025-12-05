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

#define _XTAL_FREQ 32000000UL

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

// Struktura bitowa dla PORTA
struct
{
    unsigned char LED1 : 1; // RA0 - CZERWONY: B³êdny Adr >63
    unsigned char LED2 : 1; // RA1 - ¯Ó£TY: START po³¹czenia
    unsigned char LED3 : 1; // RA2 - ZIELONY: Online
    unsigned char LED4 : 1; // RA3 - ¯Ó£TY: Start adresowania
    unsigned char LED5 : 1; // RA4 - ZIELONY: Adres ok
    unsigned char LED6 : 1; // RA5 - CZERWONY: NIEPOWODZENIE
    unsigned char SW2 : 1;  // RA6 - Przycisk 2
    unsigned char SW1 : 1;  // RA7 - Przycisk 1
} RB_A;

// Stany programu
enum StateEnum
{
    STANDBY,
    S66_ON,
    SEND_LOGIN,
    WAIT_NEXT,
    SEND_ADR,
    SEND_STORE,
    CHECK_ADR,
    DIM_ADR
};

// Zmienne globalne
volatile enum StateEnum prg_state = STANDBY;
volatile unsigned char bBlink = 0;
volatile unsigned char bDALI_Adr = 0;
unsigned char ErrCode = 0;

// Prosty delay w milisekundach
void delay_ms(unsigned int ms)
{
    while (ms--)
    {
        __delay_ms(1);
    }
}

void main(void)
{
    OSCFRQ = 0b00000110; // Ustawienie czêstotliwoœci wewnêtrznego oscylatora na 32 MHz

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

    // G³ówna pêtla
    while (1)
    {
        // Aktualizuj strukturê RB_A na podstawie PORTA
        *((unsigned char *)&RB_A) = PORTA;

        // Migacz LED co ~100ms (prosty licznik)
        static unsigned int counter = 0;
        if (++counter >= 10000)
        {
            counter = 0;
            if (++bBlink >= 10)
                bBlink = 0;
        }

        // Pobierz i sprawdŸ adres z prze³¹czników obrotowych
        bDALI_Adr = ((PORTC & 0x0F) ^ 0x0F) * 10 + ((PORTB & 0x0F) ^ 0x0F);

        if (bDALI_Adr > 63) // >63 = nieprawid³owy = Reset
        {
            LATA &= 0xC0; // Wszystkie LED WY£
            if (bBlink < 5)
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
            if (bBlink == 1)
            {
                LATA |= 0x02; // LED2 (¯Ó£TY) b³yska krótko = Bezczynnoœæ
            }

            // SprawdŸ przycisk 1
            if (!RB_A.SW1)
            {
                delay_ms(50); // Debounce
                while (!RB_A.SW1)
                {
                    *((unsigned char *)&RB_A) = PORTA;
                }
                prg_state = S66_ON;
            }

            // SprawdŸ przycisk 2
            if (!RB_A.SW2)
            {
                delay_ms(50); // Debounce
                while (!RB_A.SW2)
                {
                    *((unsigned char *)&RB_A) = PORTA;
                }
                prg_state = CHECK_ADR;
            }
            break;

        case S66_ON:          // ## Krok 2: Przycisk 1 naciœniêty
            LATA &= 0xC0;     // Wszystkie LED WY£
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£
            delay_ms(200);
            prg_state = SEND_LOGIN;
            break;

        case SEND_LOGIN:      // ## Krok 3: Logowanie (symulacja)
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£ (utrzymaj)
            LATA &= 0xC0;     // Wszystkie LED WY£
            // Miganie LED2 (¯Ó£TY)
            if (bBlink < 5)
            {
                LATA |= 0x02;
            }

            static unsigned char login_counter = 0;
            if (++login_counter >= 50)
            {
                login_counter = 0;
                ErrCode = 0; // Symulacja sukcesu logowania
                prg_state = WAIT_NEXT;
            }
            break;

        case WAIT_NEXT:       // ## Krok 4: Wynik z LED, czekaj na przycisk 1
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£ (utrzymaj)
            LATA &= 0xC0;     // Wszystkie LED WY£
            if (!ErrCode)
            {
                if (bBlink < 5)
                {
                    LATA |= 0x04; // LED3 (ZIELONY) miga: Login OK
                }
            }
            else
            {
                LATA |= 0x01; // LED1 (CZERWONY) W£ = B³¹d loginu
            }

            if (!RB_A.SW1)
            {
                delay_ms(50);
                while (!RB_A.SW1)
                {
                    *((unsigned char *)&RB_A) = PORTA;
                }
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
            break;

        case SEND_ADR:        // ## Krok 5: Wyœlij nowy Adr (symulacja)
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£ (utrzymaj)
            LATA &= 0xC0;     // Wszystkie LED WY£
            LATA |= 0x02;     // LED2 (¯Ó£TY) W£
            delay_ms(200);
            LATA |= 0x04; // LED3 (ZIELONY) W£: Adr OK
            prg_state = SEND_STORE;
            break;

        case SEND_STORE:      // ## Krok 6: Wyœlij polecenie zapisu (symulacja)
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£ (utrzymaj)
            LATA &= 0xC0;     // Wszystkie LED WY£
            LATA |= 0x04;     // LED3 (ZIELONY) W£: Zapis OK

            if (!RB_A.SW1)
            {
                delay_ms(50);
                while (!RB_A.SW1)
                {
                    *((unsigned char *)&RB_A) = PORTA;
                }
                delay_ms(200);
                LATB &= ~(1 << 5); // RB5: PrzekaŸnik WY£
                prg_state = STANDBY;
            }
            break;

        case CHECK_ADR:       // ## Krok 7: SprawdŸ adres
            LATA &= 0xC0;     // Wszystkie LED WY£
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£
            LATA |= 0x08;     // LED4 (¯Ó£TY) W£: Sprawdzanie trwa
            delay_ms(200);
            LATA &= 0xC0;
            LATA |= 0x10; // LED5 (ZIELONY) W£ = Adr OK
            prg_state = DIM_ADR;
            break;

        case DIM_ADR:         // ## Krok 8: Test funkcji (symulacja)
            LATB |= (1 << 5); // RB5: PrzekaŸnik/S66 W£ (utrzymaj)
            delay_ms(300);

            if (!RB_A.SW2)
            {
                delay_ms(50);
                while (!RB_A.SW2)
                {
                    *((unsigned char *)&RB_A) = PORTA;
                }
                delay_ms(200);
                LATB &= ~(1 << 5); // RB5: PrzekaŸnik WY£
                prg_state = STANDBY;
            }
            break;

        default:          // ## Ostatni krok: Nieznany Case
            LATA |= 0x01; // LED1 (CZERWONY) krótko W£
            delay_ms(200);
            prg_state = STANDBY;
            break;
        }
    }
}