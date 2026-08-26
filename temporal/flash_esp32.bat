@echo off
REM Script para compilar y flashear ESP32 con ESP-IDF (C/C++)
REM Windows Batch Script

setlocal enabledelayedexpansion

echo.
echo ======================================================
echo ESP32 WS2812 LED Controller - Build & Flash Script
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
    echo Error: No se encontro puerto COM
    echo.
    echo Por favor:
    echo 1. Conecta el ESP32 a la USB
    echo 2. Abre el Administrador de dispositivos
    echo 3. Encuentra el puerto COM en "Puertos (COM y LPT)"
    echo.
    echo Luego edita este script y reemplaza "COM3" con tu puerto
    set PORT=COM3
    echo Usando puerto por defecto: !PORT!
)

echo Puerto detectado: %PORT%
echo.

REM Verificar ESP-IDF
echo Verificando ESP-IDF...
if "%IDF_PATH%"=="" (
    echo Error: IDF_PATH no esta configurado
    echo.
    echo Por favor instala ESP-IDF:
    echo https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/
    echo.
    pause
    exit /b 1
)

echo ✓ ESP-IDF encontrado en: %IDF_PATH%
echo.

REM Verificar esptool
echo Verificando esptool...
python --version >nul 2>&1
if errorlevel 1 (
    echo Error: Python no esta instalado
    echo Descarga Python desde https://www.python.org/
    pause
    exit /b 1
)

pip install --upgrade esptool >nul 2>&1

echo.
echo ======================================================
echo Paso 1: Compilar el proyecto
echo ======================================================
echo.

echo Compilando el firmware...
idf.py build

if errorlevel 1 (
    echo.
    echo Error durante la compilación
    pause
    exit /b 1
)

echo ✓ Compilación exitosa

echo.
echo ======================================================
echo Paso 2: Flashear en ESP32
echo ======================================================
echo.

echo Preparando flasheo...
echo Presiona ENTER para continuar
echo (Mantén presionado el botón BOOT del ESP32 mientras se flashea)
echo.
pause

echo.
echo Flasheando firmware...
idf.py -p %PORT% flash

if errorlevel 1 (
    echo.
    echo Error al flashear. Intenta:
    echo 1. Mantener presionado el botón BOOT mientras se flashea
    echo 2. Verificar que el puerto %PORT% es correcto
    echo 3. Ejecutar: idf.py -p COM3 erase_flash (reemplazar COM3)
    pause
    exit /b 1
)

echo.
echo ======================================================
echo Paso 3: Monitoreo Serial
echo ======================================================
echo.

echo Abriendo monitor serial...
timeout /t 2 /nobreak

echo.
idf.py -p %PORT% monitor

echo.
echo ======================================================
echo PROCESO COMPLETADO
echo ======================================================
echo.
pause
