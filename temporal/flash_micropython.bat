@echo off
REM Script para instalar MicroPython en ESP32 y cargar la demo de LEDs WS2812
REM Windows Batch Script

setlocal enabledelayedexpansion

echo.
echo ======================================================
echo ESP32 WS2812 LED Controller - MicroPython Installer
echo ======================================================
echo.

REM Detectar puerto COM
echo Detectando puerto COM del ESP32...
for /f "tokens=1" %%A in ('wmic logicalvolumedevice get name ^| findstr COM') do (
    set "PORT=%%A"
    goto :found_port
)

:found_port
if "%PORT%"=="" (
    set PORT=COM3
    echo Usando puerto por defecto: !PORT!
    echo (Si no es correcto, edita este script)
) else (
    echo ✓ Puerto detectado: %PORT%
)

echo.

REM Verificar Python
echo Verificando Python y pip...
python --version >nul 2>&1
if errorlevel 1 (
    echo ✗ Error: Python no esta instalado
    echo Descarga Python desde https://www.python.org/
    pause
    exit /b 1
)

echo ✓ Python encontrado

echo.
echo ======================================================
echo Paso 1: Instalar herramientas necesarias
echo ======================================================
echo.

echo Instalando esptool...
pip install --upgrade esptool -q

echo Instalando adafruit-ampy...
pip install --upgrade adafruit-ampy -q

echo ✓ Herramientas instaladas

echo.
echo ======================================================
echo Paso 2: Descargar firmware MicroPython
echo ======================================================
echo.

echo Buscando archivo .bin en la carpeta actual...
for /r "." %%F in (esp32-*.bin) do (
    set "FIRMWARE=%%F"
    goto :found_firmware
)

:found_firmware
if "%FIRMWARE%"=="" (
    echo ✗ Error: No se encontro archivo esp32-*.bin
    echo.
    echo PASOS:
    echo 1. Abre en tu navegador:
    echo    https://micropython.org/download/esp32/
    echo 2. Descarga el archivo .bin mas reciente
    echo    (ej: esp32-20240602-v1.24.0.bin)
    echo 3. Guarda el archivo en esta carpeta
    echo 4. Ejecuta este script de nuevo
    echo.
    pause
    exit /b 1
)

echo ✓ Firmware encontrado: %FIRMWARE%

echo.
echo ======================================================
echo Paso 3: Preparar ESP32 para flasheo
echo ======================================================
echo.

echo INSTRUCCIONES IMPORTANTES:
echo 1. Conecta el ESP32 a la USB
echo 2. Presiona y MANTEN presionado el boton BOOT
echo 3. Presiona tambien el boton EN/RESET
echo 4. Suelta el boton EN/RESET (manten BOOT presionado)
echo 5. Suelta el boton BOOT cuando comience el flasheo
echo.
pause

echo.
echo ======================================================
echo Paso 4: Flashear MicroPython
echo ======================================================
echo.

echo Borrando memoria flash...
esptool.py --port %PORT% --baud 460800 erase_flash

if errorlevel 1 (
    echo ✗ Error al borrar la memoria flash
    pause
    exit /b 1
)

echo.
echo Escribiendo firmware...
esptool.py --port %PORT% --baud 460800 write_flash -z 0x1000 "%FIRMWARE%"

if errorlevel 1 (
    echo ✗ Error al flashear
    pause
    exit /b 1
)

echo ✓ Firmware flasheado exitosamente

echo.
echo ======================================================
echo Paso 5: Cargar archivos de la demo
echo ======================================================
echo.

echo Esperando reinicio (3 segundos)...
timeout /t 3 /nobreak

echo.
echo Subiendo boot.py...
ampy --port %PORT% put boot.py /boot.py

echo Subiendo main.py...
ampy --port %PORT% put main.py /main.py

echo ✓ Archivos cargados exitosamente

echo.
echo ======================================================
echo Paso 6: Verificacion
echo ======================================================
echo.

echo Archivos en ESP32:
ampy --port %PORT% ls /

echo.
echo ======================================================
echo COMPLETADO EXITOSAMENTE
echo ======================================================
echo.
echo Tu ESP32 esta listo con MicroPython!
echo.
echo CONFIGURACION DE HARDWARE:
echo.
echo   ESP32 GPIO25 ----^> LED WS2812 Data (DIN)
echo   ESP32 GND    ----^> LED WS2812 GND
echo   5V PSU       ----^> LED WS2812 +5V
echo.
echo Para cambiar el pin GPIO: edita LED_PIN en main.py
echo.
echo El programa esta listo en: /main.py
echo Presiona RESET del ESP32 para ejecutar
echo.
pause
