/*=============================================================================
                                  DEFINES
=============================================================================*/
// SFR
#byte ANSELA=0xF38
#byte ANSELB=0xF39
#byte ANSELC=0xF3A 
#byte IOCB=0xF62              // RB Interrupt On Change Reg, 7:4, 1=enabled       
#byte PORTC=0xF82             // 
#byte PORTB=0xF81
#byte PORTA=0xF80

// Port Pins
#define xKill24V pin_a5       // output pin 24DC Killer
#define xLedOut pin_a4        // signal LED
#define xConverterOn pin_a7   // turn the converter on/off
#define xCurrentPulse pin_c4  // drive current pulse mosfet
#define xTestJumper pin_c5    // jumper for test mode
#define xHeater1 pin_c0       // heater1 on/off (alone for half power) 
#define xHeater2 pin_c1       // heater2 on/off (both for full power)  
#define xDaliSend pin_b3      // 

//Misc
#define LED_TEST_TOL    3            // led test tolerance. 3 ~ 0,06V
#define LED_TEST_TIME   600          // x * 100ms ->  Testdauer
#define PULSE_TIME      1            // current pulse duration: 100ms
#define ADR_POTI200K    0b01011100   // 
#define ADR_POTI20K     0b01011010
#define POTI_CH_A       0b00000000
#define POTI_CH_B       0b10000000
#define PARAM_START     0 //15 //100          // Startwert parametrierung
#define EOL             6000000      // max. operating hours =  100.000h = 6.000.000min
#define EOL_CNT         20           // alle x minuten wird dieser wert auf die EOL Variable addiert und gespeichert
#define HOUR            60           // 60min = 1h
#define MINUTE          60           // 60s = 1min
#define TIMER3          12580
//#define EOL_CNT_START   30           // x * sek      

// Status bits
#define ON_OFF          0b00000001  // Lantern On / Off
#define DIMM            0b00000010  // Lantern dimmed (Power Level < 254)
#define ST_EOL          0b00000100  // EOL 
#define LED_FAIL        0b00001000  // LED Failure 
#define OUT_FAIL        0b00010000  // Converter Output > 20V || ON & < 0.5V
#define POWER           0b10000000  // Lantern is powered and working

// EEPROM Addresses
#define EE_PARAM        0x10   // leuchte parametriert? -> 'P' = yes
#define EE_CIRCUITS     0x11   // number of circuits
#define EE_STATUS       0x12   // status of nav light
#define EE_POTI         0x13   // digi poti value
#define EE_ADDRESS      0x14   // DALI short address, keine DALI Adresse = 0xFF
#define EE_GROUP_0_7    0x15   // DALI Group 0-7
#define EE_GROUP_8_15   0x16   // DALI Group 8-15
#define EE_BLINK        0x17   // Laterne mit fester Blinksequenz(Scene) 1-16
#define EE_OHC_START    0x30   // eeprom start address operating hours array (5x 4Byte due to byte endurance of EEPROM)
                               // 0x30 - 0x43 = 20Byte
#define PWM_MAX    1000
#define PWM_STEP   20 
int16 const HEATER_PWM[PWM_STEP] = {50,100,150,200,250,300,350,400,450,500,550,600,650,700,750,800,850,900,950,PWM_MAX};

#define SEQ  5                  // Max. Anzahl Szene 5 -> 0 - 4 !!!-> EEPROM 1-5, GO_TO_SCENE 0-4 !!!
#define TAKT 3
char const BLINK[SEQ][TAKT] = {             // x * 50ms Taktdauer AN, AUS, AN, AUS, ..., 0 = Beginn von vorn
                                1,1,0,      // 0: Seq1: 50ms An, 50ms Aus, wiederholen
                                3,3,0,      // 1: Seq2: 150ms An, 150ms Aus
                                5,5,0,      // 2: Seq3: 250ms An, 250ms Aus
                                10,10,0,    // 3: Seq4: 500ms An, 500ms Aus
                                6,4,0       // 4: Seq5: 300ms An, 200ms Aus
                              };     
   
/*=============================================================================
                                 VARIABLEN
=============================================================================*/
static unsigned int32
  dOHC               // operation hour counter
  ;  
static unsigned int16
  wLedTestTime       // 
  ;

static unsigned int8
  nop,
  bADC_Ch0,          // 
  bADC_Ch1,          // 
  bADC_Ch2,          // 
  bADC_Ch3,          // input current        
  bADC_Ch8,          // 
  bADC_Ch10,         // 
//  bADC_Ch12,         // 
  bCircuitNumber,    // 
  bCount500ms,
  bCount1000ms,
  bCountMin,         // count minute 
  bCountEOL,
  bCountPulse,
  bStatus,
  bOHoffset,        // EEPROM Offset for OHC Array
  bPotiValue,
  bHeaterHalfCnt,   // Heater PWM Wert
  bHeaterFullCnt,   // Heater PWM Wert
  bBlinkSeq,        // Blinksequenz  
  bBlinkCnt,        // Blinkzähler
  bBlinkdauer,
  bBlinkPhase    
  ;

struct flags1 
{
  int ms100    : 1;   // T0 Interrupt flag 100ms
  int dali_rcv : 1;   // dali receive data available
  int ms500    : 1;   // T0 Interrupt flag 500ms
  int dali_dim : 1;   // lantern dimmed by dali? 1 = yes
  int pulse    : 1;   // enable current pulse
  int ms1000   : 1;   // T0 Interrupt flag 1000ms
  int lamp_eol : 1;   // EOL 
  int param    : 1;   // Parametrierung vorhanden? 1=ja
  int heat_half: 1;
  int heat_full: 1;
  int blink    : 1;   // Blinkmerker AN/AUS
}flag;



/*=============================================================================
                              DALI DEFINES
=============================================================================*/
// DALI Modes
#define DALI_STBY  0x00   // Standby
#define DALI_SBIT  0x01   // Startbit (Send)
#define DALI_RCV   0x02   // Receive
#define DALI_SND   0x04   // Send
#define DALI_DAT   0x08   // Dali data available

//Misc
#define DALI_IN        pin_b4    // Dali Input
#define DALI_TIMEOUT   20        // x * 100ms
#define DALI_MAX_LEVEL 254
#define DALI_MIN_LEVEL 144       // 144 -> 5% PWM; 54 -> 0.5%
#define DALI_TEST_CNT  5         // 500ms Toggle am DALI Ein/Ausgang

char const PARAM[5] = {'P','A','R','A','M'};

/*=============================================================================
                              DALI VARIABLES
=============================================================================*/
static unsigned int16
  wDaliRcv           // dali receive data 16Bit address + command   
  ;

static unsigned int8
  bDaliRcvAdr,       // 
  bDaliRcvCmd,
  bDaliDTR,
  bDaliBitCnt,
  bDaliMode,
  bDaliBit,
  bReadIOC,
  bDaliMaxLevel,
  bDaliMinLevel,
  bDaliShortAdr,
  bDaliPowerLevel,
  bLastPowerLevel,
  bDaliSndBuff,
  bDaliSndCnt,
  bDaliSndClk,
  bDaliLastDAP,
  bDaliBusTimeout,
  bDaliGroup0_7,
  bDaliGroup8_15,
  bParamCnt,
  bCmdCnt,
  bLastCmd,
  bParam,
  bDaliTestCnt           // zum testen Dali Send/Receive
  ; 

struct dali1
{
  int cmd          : 1;  // dali cmd following
  int ioc          : 1;  // 
  int broadcast    : 1;
  int group        : 1;
  int snd          : 1;  //
  int ms100        : 1;
  int bustimeout   : 1;  // Timeoutflag wenn kein Datenempfang
}daliflag;
