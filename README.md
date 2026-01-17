# README #
Addressing Tools - Narz?dzie do ustawiania i sprawdzania adresu dali lamp latern

### Repozytorium ###
* Link: https://bitbucket.org/glamoxas/xxx

### Informacje ###
* Wersja: 1.0
* Data wydania: 22 Listopad 2025
* Autor: Grzegorz Jakowicz
* Pozosta?y kontakt: Dzia? technologii, Glamox Dobczyce, Polska

### ?rodowisko ###
* Procesor: PIC18F25K42
* Kompilator: XC8 ver. 3.10
* Sk?adnik kompilatora: PIC18F-K_DFP
* IDE: Visual Studio Code
* Run: PowerShell

---
### Konfiguracja ###

#### Spis tre?ci
- [Instalacja ?rodowiska MPLAB](#instalacja-?rodowiska-mplab)
- [Kompilacja HeX](#kompilacja-hex)


#### Instalacja ?rodowiska MPLAB ####

1. Pobierz **MPLAB XC8 Compiler** v3.10 lub nowszy:
   https://www.microchip.com/mplab/compilers

2. Pobierz **PIC18F-K Device Family Pack (DFP)**:
   - Opcja A: Zainstaluj MPLAB X IDE (zawiera Content Manager do automatycznej instalacji DFP) z:  
     https://www.microchip.com/en-us/tools-resources/develop/mplab-x-ide
   - Opcja B: Pobierz bezpo?rednio DFP z:  
     https://packs.download.microchip.com/  
     Szukaj: `Microchip.PIC18F-K_DFP.x.x.xxx.atpack` (Microchip PIC18F-K Series Device Support (1.15.303) lub nowszy)

3. Zainstaluj kompilator, akceptuj?c domy?ln? ?cie?k?:  
   `C:\Program Files\Microchip\xc8\v3.10`

4. Zainstaluj DFP:
   - Je?li u?ywasz MPLAB X IDE: Tools ? Packs ? PIC18F-K_DFP
   - Je?eli r?cznie: rozpakuj .atpack do `C:\Program Files\Microchip\MPLABX\packs\Microchip\PIC18F-K_DFP\`

5. Je?eli u?ywasz innych ?cie?ek, zaktualizuj je w:
   - `Makefile` (linia ~16-17)
   - `build.ps1` (linia ~75-76)
   - `.vscode/c_cpp_properties.json` (linie 7-10, 18)


#### Kompilacja HEX ####

1. PowerShell Script

```powershell
# Kompilacja z XC8
.\build.ps1 -Compiler XC8

# Czyszczenie plików
.\build.ps1 -Clean

# Czyszczenie i budowanie
.\build.ps1 -Compiler XC8 -Clean

# Tryb verbose (wi?cej informacji)
.\build.ps1 -Compiler XC8 -Verbose
```

2. Visual Studio Code

   - U?yj skrótu klawiszowego: `Ctrl+Shift+B`
   - Lub otwórz palet? polece? (`Ctrl+Shift+P`) ? `Tasks: Run Build Task`

   **Dost?pne zadania:**
   - **Build PIC18F25K42 (XC8)** - Kompilacja z kompilatorem XC8
   - **Clean Build Files** - Czyszczenie plików kompilacji
   - **Build with PowerShell Script (XC8)** - Kompilacja przez skrypt PowerShell

   **Wyniki kompilacji:**
   - Pliki wyj?ciowe w katalogu `output/`
   - Plik HEX: `output/PIC_addressing.hex`
   - Pliki tymczasowe w katalogu `build/`


