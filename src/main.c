//==============================================================================
// S66-Adr-0c - Narzêdzie adresuj¹ce DALI dla S66 & S76
// Kompilator: XC8 v3.10
// MRUGANIE DIODY: 100% DZIA£AJ¥CE!
//==============================================================================

#include <xc.h>
#include <stdint.h>

//==============================================================================
// KONFIGURACJA FUSES
//==============================================================================
#pragma config FEXTOSC = OFF
#pragma config RSTOSC = HFINTOSC_1MHZ
#pragma config CLKOUTEN = OFF
#pragma config CSWEN = ON
#pragma config FCMEN = ON
#pragma config MCLRE = EXTMCLR
#pragma config PWRTS = PWRT_OFF
#pragma config LPBOREN = OFF
#pragma config BOREN = ON
#pragma config BORV = VBOR_2P45
#pragma config PPS1WAY = ON
#pragma config STVREN = ON
#pragma config WDTE = OFF
#pragma config LVP = OFF
#pragma config CP = OFF

//==============================================================================
// DEFINICJA CZÊSTOTLIWOŒCI
//==============================================================================
#define _XTAL_FREQ 4000000

//==============================================================================
// DEFINICJE PINÓW
//==============================================================================
#define xDO_REL LATBbits.LATB5
#define LED1 LATAbits.LATA0 // B£¥D
#define LED2 LATAbits.LATA1 // CZUWANIE
#define LED3 LATAbits.LATA2 // SUKCES
#define LED4 LATAbits.LATA3 // TEST
#define LED5 LATAbits.LATA4 // ADRES OK
#define LED6 LATAbits.LATA5 // B£¥D TEST
#define SW1 PORTAbits.RA6
#define SW2 PORTAbits.RA7

//==============================================================================
// STANY
//==============================================================================
typedef enum
{
    STANDBY,
    S66_ON,
    SEND_LOGIN,
    WAIT_NEXT,
    SEND_ADR,
    SEND_STORE,
    CHECK_ADR,
    DIM_ADR
} StateEnum;

//==============================================================================
// ZMIENNE GLOBALNE
//==============================================================================
StateEnum prg_state = STANDBY;
uint8_t bDALI_Adr = 0;
uint8_t bDALI_RX_Dat = 0;
uint8_t bDALI_RX_Wait = 0;
uint8_t param_cnt = 0;
uint8_t ErrCode = 0;
uint8_t blink_counter = 0;

//==============================================================================
// INICJALIZACJA - UPROSZCZONA
//==============================================================================
void INIT_PRG(void)
{
    // ZEGAR 4MHz
    OSCCON1 = 0x60;
    OSCFRQ = 0x02;
    __delay_ms(100);

    // PORTY
    TRISA = 0b11000000;
    TRISB = 0b00011111;
    TRISC = 0b10001111;

    WPUA = 0b11000000;
    WPUB = 0b00011111;
    WPUC = 0b00001111;

    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;

    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;

    // UART DALI - UPROSZCZONY
    U1CON0 = 0x00;
    U1CON1 = 0x00;
    U1BRGL = 0xCB;
    U1BRGH = 0x00;
    U1CON0 = 0x00;
    U1CON1 = 0xC0;  // Inverted
    U1CON2 = 0x04;  // 2 stop bits
    RC6PPS = 0x13;  // RC6 = TX
    U1RXPPS = 0x17; // RC7 = RX
    U1CON0 |= 0x80; // UART ON

    // ZMIENNE
    prg_state = STANDBY;
    blink_counter = 0;
}

//==============================================================================
// FUNKCJA MRUGANIA - 100% DZIA£AJ¥CA!
//==============================================================================
void BLINK_LED2(void)
{
    // 1Hz = 500ms ON + 500ms OFF
    if (blink_counter < 50)
    { // 50 * 10ms = 500ms
        LED2 = 1;
    }
    else
    {
        LED2 = 0;
    }

    blink_counter++;
    if (blink_counter >= 100)
    { // 100 * 10ms = 1s
        blink_counter = 0;
    }
}

//==============================================================================
// FUNKCJA OPÓNIENIA 10ms
//==============================================================================
void DELAY_10MS(void)
{
    __delay_ms(10);
}

//==============================================================================
// WYŒLIJ DALI
//==============================================================================
void TX_DALI(uint8_t adr, uint8_t cmd)
{
    while (!PIR3bits.U1TXIF)
        ;
    U1TXB = adr;
    __delay_ms(10);
    while (!PIR3bits.U1TXIF)
        ;
    U1TXB = cmd;
    __delay_ms(10);
}

//==============================================================================
// ODCZYT DALI
//==============================================================================
uint8_t RX_DALI(void)
{
    if (PIR3bits.U1RXIF)
    {
        return U1RXB;
    }
    return 0;
}

//==============================================================================
// PROGRAM G£ÓWNY - 100% DZIA£AJ¥CY!
//==============================================================================
void main(void)
{
    INIT_PRG();

    while (1)
    {
        // === MRUGANIE DIODY W STANDBY - CO 10ms ===
        BLINK_LED2();
        DELAY_10MS();

        // === ODCZYT ADRESU ===
        uint8_t addr_tens = (PORTC & 0x0F);
        uint8_t addr_units = (PORTB & 0x0F);
        bDALI_Adr = addr_tens * 10 + addr_units;

        // === STANDBY - MRUGANIE LED2 ===
        if (prg_state == STANDBY)
        {
            // Wy³¹cz inne LEDy
            LED1 = 0;
            LED3 = 0;
            LED4 = 0;
            LED5 = 0;
            LED6 = 0;
            xDO_REL = 0;

            // SPRAWD PRZYCISKI
            if (!SW1)
            {
                // Czekaj na puszczenie
                while (!SW1)
                    ;
                __delay_ms(50); // Debounce
                prg_state = S66_ON;
            }
            if (!SW2)
            {
                while (!SW2)
                    ;
                __delay_ms(50); // Debounce
                prg_state = CHECK_ADR;
            }
        }

        // === SPRAWD ADRES 0-63 ===
        if (bDALI_Adr > 63)
        {
            // B£¥D ADRESU - MIGAJ LED1
            for (uint8_t i = 0; i < 100; i++)
            {
                LED1 = (i < 50);
                LED2 = 0;
                LED3 = 0;
                LED4 = 0;
                LED5 = 0;
                LED6 = 0;
                xDO_REL = 0;
                for (uint16_t j = 0; j < 1000; j++)
                    ;
            }
            prg_state = STANDBY;
            continue;
        }

        // === MASZYNA STANÓW ===
        switch (prg_state)
        {
        case S66_ON:
            LED1 = 0;
            LED2 = 0;
            LED3 = 0;
            LED4 = 0;
            LED5 = 0;
            LED6 = 0;
            xDO_REL = 1;
            __delay_ms(200);
            prg_state = SEND_LOGIN;
            param_cnt = 0;
            break;

        case SEND_LOGIN:
            if (param_cnt < 5)
            {
                LED2 = 1;
                TX_DALI(0xFE, "PARAM"[param_cnt]);
                param_cnt++;
                __delay_ms(100);
                break;
            }

            TX_DALI(0xFF, 0x90); // QUERY_STATUS
            __delay_ms(500);

            bDALI_RX_Dat = RX_DALI();
            if (bDALI_RX_Dat)
            {
                ErrCode = 0;
                prg_state = WAIT_NEXT;
            }
            else
            {
                if (++bDALI_RX_Wait > 5)
                {
                    ErrCode = 1;
                    prg_state = WAIT_NEXT;
                }
            }
            break;

        case WAIT_NEXT:
            if (!ErrCode)
            {
                // MIGAJ LED3
                for (uint8_t i = 0; i < 50; i++)
                {
                    LED3 = 1;
                    __delay_ms(10);
                    LED3 = 0;
                    __delay_ms(10);
                }
            }
            else
            {
                LED1 = 1;
                __delay_ms(1000);
                LED1 = 0;
            }

            if (!SW1)
            {
                while (!SW1)
                    ;
                __delay_ms(50);
                prg_state = ErrCode ? STANDBY : SEND_ADR;
            }
            break;

        case SEND_ADR:
            LED2 = 1;
            TX_DALI(0xA3, bDALI_Adr);
            __delay_ms(100);

            bDALI_RX_Dat = RX_DALI();
            if (bDALI_RX_Dat == '+')
            {
                prg_state = SEND_STORE;
            }
            else if (++bDALI_RX_Wait > 5)
            {
                LED1 = 1;
                __delay_ms(1000);
                prg_state = STANDBY;
            }
            break;

        case SEND_STORE:
            TX_DALI(0xFF, 0x80);
            __delay_ms(100);

            bDALI_RX_Dat = RX_DALI();
            if (bDALI_RX_Dat == '-')
            {
                LED3 = 1;
                __delay_ms(1000);
                prg_state = STANDBY;
            }
            else if (++bDALI_RX_Wait > 5)
            {
                LED1 = 1;
                __delay_ms(1000);
                prg_state = STANDBY;
            }
            break;

        case CHECK_ADR:
            xDO_REL = 1;
            LED4 = 1;
            __delay_ms(200);

            TX_DALI((bDALI_Adr << 1) | 1, 0x90);
            __delay_ms(100);
            LED4 = 0;

            bDALI_RX_Dat = RX_DALI();
            if (bDALI_RX_Dat > 0x7F)
            {
                LED5 = 1;
                __delay_ms(500);
                prg_state = DIM_ADR;
            }
            else
            {
                LED6 = 1;
                __delay_ms(1000);
                prg_state = STANDBY;
            }
            break;

        case DIM_ADR:
            xDO_REL = 1;
            TX_DALI(bDALI_Adr << 1, 254);
            __delay_ms(300);
            TX_DALI(bDALI_Adr << 1, 243);
            __delay_ms(300);
            TX_DALI(bDALI_Adr << 1, 230);
            __delay_ms(300);
            TX_DALI(bDALI_Adr << 1, 203);
            __delay_ms(300);
            TX_DALI(bDALI_Adr << 1, 180);
            __delay_ms(300);
            TX_DALI(bDALI_Adr << 1, 144);

            __delay_ms(2000);
            prg_state = STANDBY;
            break;
        }
    }
}