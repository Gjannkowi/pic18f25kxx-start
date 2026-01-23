#ifndef DALICMD_H
#define DALICMD_H

//=================== Indirekte Befehle Lampenleistung ===========================
#define OFF                0x00 // Lampe sofort aus ohne fading
#define UP                 0x01 // 200ms lang mit ausgewählter Stufengeschwindigkeit heller werden 
#define DOWN               0x02 // 200ms lang mit ausgewählter Stufengeschwindigkeit dunkler werden 
#define STEP_UP            0x03 // akt. Leistungswert eine Stufe höher ohne fading. Bis max Level
#define STEP_DOWN          0x04 // akt. Leistungswert eine Stufe niedriger ohne fading. Bis min Level, Lampe geht nicht aus. 
#define RECALL_MAX_LEVEL   0x05 // parametrierten max Level aufrufen. falls Lampe aus wird eingeschaltet
#define RECALL_MIN_LEVEL   0x06 // parametrierten min Level aufrufen. falls Lampe aus wird eingeschaltet
#define STEP_DOWN_AND_OFF  0x07 // akt. Leistungswert eine Stufe niedriger ohne fading. Bis Lampe aus.
#define ON_AND_STEP_UP     0x08 // akt. Leistungswert eine Stufe höher ohne fading. Bis max Level. Wenn Lampe AUS -> AN
#define ENABLE DAPC SEQUENCE	  0x09	
#define GO TO LAST ACTIVE LEVEL 0x0A
// 0B - 0F reserviert
#define SCENE              0x10 // 0x10 - 0x1F Szene 1 - 16 aufrufen -> verodern mit 0x01, 0x02, etc um gewünschte Szene einzustellen    

//====================== Konfigurationsbefehle ===================================
// Befehle werden doppelt gesendet. Ausf�hren erst nach dem 2. mal
#define RESET                            0x20  // Werkseinstellungen
#define STORE_ACTUAL_LEVEL_IN_THE_DTR    0x21  // akt. Helligkeitswert im DTR speichern (Data-Transfer-Register)
// 22 - 29 reserviert
#define STORE_THE_DTR_AS_MAX_LEVEL       0x2A  // akt. Wert im DTR als max. Leistung speichern
#define STORE_THE_DTR_AS_MIN_LEVEL       0x2B  // akt. Wert im DTR als min. Leistung speichern
#define STORE_THE_DTR_AS_SYSTEM_FAILURE_LEVEL 0x2C  // akt. Wert im  DTR als Leistung im Fehlerfall speichern
#define STORE_THE_DTR_AS_POWER_ON_LEVEL  0x2D  // akt. Wert im DTR als Leistung nach Einschalten speichern
#define STORE_THE_DTR_AS_FADE_TIME       0x2E  // akt. Wert im DTR als Stufenzeit speichern
#define STORE_THE_DTR_AS_FADE_RATE       0x2F  // akt. Wert im DTR als Stufengeschwindigkeit speichern
// 30 - 3F reserviert
#define STORE_THE_DTR_AS_SCENE           0x40  // 0x40 - 0x4F: akt. Wert im DTR als Szene x speichern
#define REMOVE_FROM_SCENE                0x50  // 0x50 - 0x5F: Szene x löschen
#define ADD_TO_GROUP                     0x60  // 0x60 - 0x6F: zur Gruppe x hinzufügen 
#define REMOVE_FROM_GROUP                0x70  // 0x70 - 0x7F: aus Gruppe x löschen
#define STORE_DTR_AS_SHORT_ADDRESS       0x80  // akt. Wert im DTR als Kurzadresse speichern
// 81 - 8F reserviert

//=========================== Zustandsabfragen ===================================
#define QUERY_STATUS                     0x90  // Zustand an Master melden
#define QUERY_BALLAST                    0x91  // arbeitet der Slave? Antwort 0xFF wenn ja
#define QUERY_LAMP_FAILURE               0x92  // Lampenspannung vorhanden? Messen -> Shunt -> ADC
#define QUERY_LAMP_POWER_ON              0x93  // akt. Level größer 0?
#define QUERY_LIMIT_ERROR                0x94  // wenn Soll level außerhalb der eingestellten min max ist
#define QUERY_RESET_STATE                0x95  // nur während des Inits?
#define QUERY_MISSING_SHORT_ADDRESS      0x96  // fehlt die Kurzadresse?
#define QUERY_VERSION_NUMBER             0x97  // Versions Nummer?
#define QUERY_CONTENT_DTR                0x98  // akt. Inhalt des DTR
#define QUERY_DEVICE_TYPE                0x99  // Leuchtenart (zB. 6 == LED Leuchte)
#define QUERY_PHYSICAL_MINIMUM_LEVEL     0x9A  // physikalisch kleinstmögliche Dimmstufe
#define QUERY_POWER_FAILURE              0x9B  // nur bei Init?
// 9C - 9F reserviert
#define QUERY_ACTUAL_LEVEL               0xA0  // akt. Level senden
#define QUERY_MAX_LEVEL                  0xA1  // max. zulässiger Lampenleistungswert
#define QUERY_MIN_LEVEL                  0xA2  // min. zulässiger Lampenleistungswert
#define QUERY_POWER_ON_LEVEL             0xA3  // Einschaltleistungswert
#define QUERY_SYSTEM_FAILURE_LEVEL       0xA4  // Fehlerfallleistungswert
#define QUERY_FADE                       0xA5  // high nibble == fade time, low nibble == fade rate
#define QUERY_MANUFACTURER_SPECIFIC_MODE 0xA6  // 
// A7 - AF reserviert
#define QUERY_SCENE_LEVEL                0xB0  // B0 - BF Leistungswert der einzelnen Szenen
#define QUERY_GROUPS_0_7                 0xC0  // zu welcher Gruppe gehört der Slave?
#define QUERY_GROUPS_8_15                0xC1
#define QUERY_RANDOM_ADDRESS_H           0xC2  // wahlfreie Adresse: obere Bits
#define QUERY_RANDOM_ADDRESS_M           0xC3  // wahlfreie Adresse: mittlere Bits
#define QUERY_RANDOM_ADDRESS_L           0xC4  // wahlfreie Adresse: untere Bits

//==========================??? Sonderbefehle ???=================================
#define TERMINATE                        0xA1
#define DTR0                             0xA3 

/*
#define TERMINATE                        0xA1  // Terminate special processes
#define DTR0                             0xA3  // Download information to the dtr                 
#define INITIALISE                       0xA5  // Addressing commands 
#define RANDOMISE                        0xA7
#define COMPARE                          0xA9
#define WITHDRAW                         0xAB
#define SEARCHADDRH                      0xB1
#define SEARCHADDRM                      0xB3
#define SEARCHADDRL                      0xB5
#define PROGRAM_SHORT_ADDRESS            0xB7
#define VERIFY_SHORT_ADDRESS             0xB9
#define QUERY_SHORT_ADDRESS              0xBB
#define PHYSICAL_SELECTION               0xBD
*/

#endif