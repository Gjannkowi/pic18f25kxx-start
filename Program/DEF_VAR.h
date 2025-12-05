/********** Definicje dla ustawie? programu ***********************/
#ifdef __DEBUG 
	#device ICD=TRUE			// ICD tylko w trybie Debug, wtedy Fuses nieaktywne !!
#else
	#fuses NOWDT,PUT_1MS,BROWNOUT,BORV28,NOLVP,NOWRT,noWRTD,PROTECT
#endif

#use fast_IO(ALL)				// Rejestry TRIS zarz?dzane r?cznie !
#use delay(internal=4MHZ)		// Zegar wewn. uC = baza czasu (x PPLEN !) dla opó?nienia bez WDT-Rst

/*** DEFINICJE PINÓW PORTÓW ***/
#define xDO_YLED2	PIN_A1		// ZASILANIE W?
#define xDO_REL		PIN_B5		// PRZEKA?NIK S66 W?/WY?
//#define xDALI_TX	PIN_C6
//#define xDALI_RX	PIN_C7

/*** DEFINICJE KOMEND DALI ***/
#define D_SPECIAL_STORE_DTR0	0xA3
#define D_BROADCAST_ADDRESS		0xFF
#define D_DAP_ADDRESS			0xFE
#define D_STORE_DTR_AS_SHORT_ADDRESS 0x80
#define D_QUERY_STATUS			0x90	// Zg?o? stan do mastera
#define D_QUERY_CONTENT_DTR		0x98
#define D_OFF				0x00	// Lampa natychmiast wy??czona bez wygaszania
#define D_RECALL_MAX_LEVEL	0x05	// Wywo?aj sparametryzowany poziom max. je?li lampa wy??czona zostanie w??czona
#define D_RECALL_MIN_LEVEL	0x06

//===== DEFINICJE DANYCH DALI
#define K_DALI_MIN_LEVEL	144		// 144 -> 5% PWM; 54->0.5%
#define K_DALI_MAX_LEVEL	254

/*** DEFINICJE STA?YCH CZASOWYCH uC ***/
#define K_T1_NUM_MS 100			// EE: T1=100ms
#define K_T1_SET	65535-(K_T1_NUM_MS*1000/8) // =65535-(0,1s/(4/Hz)/DIV))
#define K_T1_DIV	T1_DIV_BY_8		// DIV=1/2/4/8 przy 0,5/1/4/16MHz
// T1-Set: 0,5MHz:DIV=1->T1=53035; 1MHz:2->53035; 4MHz:8->53035; 16MHz:8->15535

/* ############### OD TEGO MIEJSCA ZMIENNE GLOBALNE ######################### */
struct {	// bPORTA.0..7
	int LED1	: 1;	// CZERWONY: B??dny Adr >63
	int LED2	: 1;	// ?Ó?TY: START po??czenia+Login S66
	int LED3	: 1;	// ZIELONY: S66 Online
	int LED4	: 1;	// ?Ó?TY: Start adresowania
	int LED5	: 1;	// ZIELONY: Adres ok
	int LED6	: 1;	// CZERWONY: Adresowanie NIEPOWODZENIE
	int SW2		: 1;
	int SW1		: 1;
} RB_A; 
#byte RB_A	= getenv("SFR:PORTA")

static const char K_param[5] = {'P', 'A', 'R', 'A', 'M'}; // Sekwencja logowania

enum StateEnum			// Stany funkcji
 {//1
	STANDBY,			// Stan bezczynno?ci
	S66_ON,
	SEND_LOGIN,
	WAIT_NEXT,
	SEND_ADR,
	SEND_STORE,
	CHECK_ADR,
	DIM_ADR
 }//1
;
StateEnum prg_state;	// Start w trybie Standby

static short			// =INT1: 1-bitowa zmienna, init na 0
    xT1_FLAG,			// Flaga 40ms lub 100ms z T1
	xDALI_RX_OK			// Status przerwania RX (1=Odbiór OK)
	;

static unsigned char	// =INT8: globalne zmienne 8-bitowe, init na 0
	bBlink,
	bDALI_Adr, 			// Adres DALI 0..63
	bDALI_RX_Dat,		// DALI RX z przerwania, ramka wsteczna: Tylko jeden bajt danych
	bDALI_RX_Wait,		// Czas oczekiwania na DALI RX
	param_cnt,
	ErrCode
	;

static unsigned char	// =INT8: Definicja rejestrów SF, sta?e adresy poni?ej
	bPORTA,				// patrz te? powy?ej RB_A
	bPORTB, bPORTC,
	bPIR3, bRC6PPS, bU1RXPPS, bU1CTSPPS,
	bCLKRCLK, bU1CON0, bU1CON1, bU1CON2, bU1BRGL, bU1BRGH,
	bU1FIFO, bU1UIR, bU1ERRIR, bU1ERRIE,
	bU1RXB, bU1TXB,
	bU1P1L,bU1P1H, bU1P2L, bU1P2H
	;
// SFR na sta?e adresy dla DALI-PIC 18F25/45K42 (STARE #byte bPIR3=0x39A3 itd.)
#byte bPORTA	= getenv("SFR:PORTA")		// patrz te? powy?ej RB_A
#byte bPORTB	= getenv("SFR:PORTB")
#byte bPORTC	= getenv("SFR:PORTC")
#byte bPIR3 	= getenv("SFR:PIR3")		// Bit 4:U1TXIF=1=TX Int, Bit3:U1RXIF
#byte bRC6PPS	= getenv("SFR:RC6PPS")		// Wybór pinu TX
#byte bU1RXPPS	= getenv("SFR:U1RXPPS")		// Wybór pinu RX
#byte bU1CTSPPS	= getenv("SFR:U1CTSPPS")	// UART1 TX Clear To Send
#byte bCLKRCLK	= getenv("SFR:CLKRCLK")		// Bit 4:0 Bity wyboru zegara
#byte bU1CON0	= getenv("SFR:U1CON0")		// Ustawienie UART1 DALI, BRGS=0
#byte bU1CON1	= getenv("SFR:U1CON1")		// Ustaw bit W??czenie portu szeregowego W?
#byte bU1CON2	= getenv("SFR:U1CON2")		// Ustaw bity TXPOL+RXPOL 1:0=2 bity stopu
#byte bU1BRGL	= getenv("SFR:U1BRGL")		// UART1 pr?dko?? transmisji
#byte bU1BRGH	= getenv("SFR:U1BRGH")
#byte bU1FIFO	= getenv("SFR:U1FIFO")		// TXBE.5=Wyczy?? TX, RXBE.1=Wyczy?? RX
#byte bU1UIR	= getenv("SFR:U1UIR")
#byte bU1ERRIR	= getenv("SFR:U1ERRIR")		// Bit7=TXMTIF, 6=PERIF (Flaga przerwania b??du parzysto?ci rejestru flag przerwa? b??dów UART)
#byte bU1ERRIE	= getenv("SFR:U1ERRIE")		//
#byte bU1RXB	= getenv("SFR:U1RXB")		// Bufor RX
#byte bU1TXB	= getenv("SFR:U1TXB")		// Bufor TX
#byte bU1P1L	= getenv("SFR:U1P1L")		// pó?okresy bitu bezczynno?ci mi?dzy ramkami forward
#byte bU1P1H	= getenv("SFR:U1P1H")
#byte bU1P2L	= getenv("SFR:U1P2L")		// pó?okresy bitu bezczynno?ci dla ramek forward
#byte bU1P2H	= getenv("SFR:U1P2H")