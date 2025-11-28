# ==============================================
# PIC18F25K42 + XC8 v3.10 – wersja ostateczna
# ==============================================

DEVICE   = PIC18F25K42
XC8      = "C:/Program Files/Microchip/xc8/v3.10/bin/xc8-cc.exe"
DFP      = "C:/Program Files/Microchip/PIC18F-K_DFP/xc8"
BUILDDIR = build
OUTDIR   = out
HEX_SRC  = $(BUILDDIR)\PIC18F25K42_Blink.hex
HEX_DST  = $(OUTDIR)\PIC18F25K42_Blink.hex
SOURCE   = src/main.c

all: $(HEX_DST)

$(HEX_DST): $(SOURCE)
	if not exist $(BUILDDIR) mkdir $(BUILDDIR)
	if not exist $(OUTDIR)   mkdir $(OUTDIR)
	$(XC8) -mcpu=$(DEVICE) -mdfp=$(DFP) -O2 -o $(HEX_SRC) $(SOURCE)
	cmd /c copy "$(HEX_SRC)" "$(HEX_DST)" >nul

clean:
	@if exist $(BUILDDIR)\* del /Q $(BUILDDIR)\* >nul 2>&1
	@if exist $(OUTDIR)\*   del /Q $(OUTDIR)\*   >nul 2>&1
	@echo Czyszczenie zakonczone – katalogi build/ i out/ pozostaly puste

flash:
	@echo.
	@echo ==========================================
	@echo GOTOWE! Czysty hex: $(HEX_DST)
	@echo ==========================================

.PHONY: all clean flash