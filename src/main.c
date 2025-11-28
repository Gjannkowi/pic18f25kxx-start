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

#define _XTAL_FREQ 32000000UL

#ifndef OSCFRQ
#define OSCFRQ (*(volatile unsigned char *)0x39DF)
#endif
#ifndef ANSELA
#define ANSELA (*(volatile unsigned char *)0x3A40)
#endif
#ifndef ODCONA
#define ODCONA (*(volatile unsigned char *)0x3A42)
#endif
#ifndef TRISA
#define TRISA (*(volatile unsigned char *)0x3FC2)
#endif
#ifndef WPUA
#define WPUA (*(volatile unsigned char *)0x3A41)
#endif
void main(void)
{
    OSCFRQ = 0b00000110; // Ustawienie czêstotliwoœci wewnêtrznego oscylatora na 32 MHz

    ANSELA = 0x00; // Wy³¹czenie wejœæ analogowych na wszystkich pinach portu A
    ODCONA = 0x00; // Wy³¹czenie trybu open-drain

    // Ustaw kierunek portu: RA0-RA5 jako wyjœcia (0), RA6-RA7 jako wejœcia (1)
    TRISA = 0b11000000;

    // W³¹cz wewnêtrzne rezystory podci¹gaj¹ce tylko na RA6 i RA7
    WPUA = 0xC0;

    // Ustaw stan pocz¹tkowy: wszystkie diody œwiec¹
    LATA = 0x3F;

    WPUA = 0xC0; // Ponowne w³¹czenie podci¹gniêæ na RA6 i RA7 (opcjonalnie)

    // Brak dodatkowych opóŸnieñ i zbêdnych ustawieñ

    while (1)
    {
        // Test: stan RA6 pokazuje siê na diodzie RA0, stan RA7 na diodzie RA1
        // Pozosta³e diody s¹ wy³¹czone
        unsigned char ra6 = (PORTA & 0x40) ? 1 : 0; // Odczyt stanu RA6
        unsigned char ra7 = (PORTA & 0x80) ? 1 : 0; // Odczyt stanu RA7

        // Jeœli przycisk nie jest wciœniêty (stan wysoki) – dioda œwieci
        // Jeœli przycisk jest wciœniêty (stan niski) – dioda gaœnie
        LATA = (ra6 << 0) | (ra7 << 1); // RA0=RA6, RA1=RA7
        // Pozosta³e diody wy³¹czone
    }
}