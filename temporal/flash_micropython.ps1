#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Script para instalar MicroPython en ESP32 y cargar la demo de LEDs WS2812
    
.DESCRIPTION
    Este script automatiza el proceso de:
    1. Verificar herramientas necesarias (esptool, ampy)
    2. Flashear firmware MicroPython
    3. Cargar archivos boot.py y main.py
    4. Abrir monitor serial
    
.PARAMETER Port
    Puerto COM del ESP32 (ej: COM3)
    
.PARAMETER Baud
    Velocidad en baudios (por defecto: 460800)
    
.EXAMPLE
    .\flash_micropython.ps1 -Port COM3
#>

param(
    [string]$Port = "COM3",
    [int]$Baud = 460800
)

# Variables de colores
$fg_green = "`e[32m"
$fg_yellow = "`e[33m"
$fg_red = "`e[31m"
$reset = "`e[0m"

function Write-Header {
    param([string]$Text)
    Write-Host ""
    Write-Host "=====================================================" -ForegroundColor Cyan
    Write-Host $Text -ForegroundColor Cyan
    Write-Host "=====================================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Success {
    param([string]$Text)
    Write-Host "${fg_green}✓ $Text${reset}" -ForegroundColor Green
}

function Write-Error {
    param([string]$Text)
    Write-Host "${fg_red}✗ $Text${reset}" -ForegroundColor Red
}

function Write-Info {
    param([string]$Text)
    Write-Host "${fg_yellow}→ $Text${reset}" -ForegroundColor Yellow
}

# ===================== INICIO =====================
Write-Header "ESP32 WS2812 LED Controller - MicroPython Installer"

Write-Info "Puerto configurado: $Port"
Write-Info "Velocidad: $Baud baud"

# ===================== VERIFICAR PYTHON =====================
Write-Header "Paso 1: Verificar Python"

try {
    $pythonVersion = python --version 2>&1
    Write-Success "Python encontrado: $pythonVersion"
} catch {
    Write-Error "Python no está instalado"
    Write-Host "Descargalo desde: https://www.python.org/"
    exit 1
}

# ===================== INSTALAR HERRAMIENTAS =====================
Write-Header "Paso 2: Instalar herramientas necesarias"

Write-Info "Instalando esptool..."
try {
    pip install --upgrade esptool -q
    Write-Success "esptool instalado"
} catch {
    Write-Error "Error al instalar esptool"
    exit 1
}

Write-Info "Instalando adafruit-ampy..."
try {
    pip install --upgrade adafruit-ampy -q
    Write-Success "adafruit-ampy instalado"
} catch {
    Write-Error "Error al instalar adafruit-ampy"
    exit 1
}

# ===================== BUSCAR FIRMWARE =====================
Write-Header "Paso 3: Buscar firmware MicroPython"

$firmwareFiles = Get-ChildItem -Filter "esp32-*.bin" -ErrorAction SilentlyContinue
$firmwarePath = $null

if ($firmwareFiles.Count -gt 0) {
    $firmwarePath = $firmwareFiles[0].FullName
    Write-Success "Firmware encontrado: $(Split-Path -Leaf $firmwarePath)"
} else {
    Write-Error "No se encontró archivo esp32-*.bin"
    Write-Host ""
    Write-Host "PASOS:" -ForegroundColor Yellow
    Write-Host "1. Abre en tu navegador: https://micropython.org/download/esp32/"
    Write-Host "2. Descarga el archivo .bin más reciente (ej: esp32-20240602-v1.24.0.bin)"
    Write-Host "3. Guarda el archivo en esta carpeta: $(Get-Location)"
    Write-Host "4. Ejecuta este script de nuevo"
    Write-Host ""
    pause
    exit 1
}

# ===================== PREPARAR PARA FLASHEO =====================
Write-Header "Paso 4: Preparar ESP32 para flasheo"

Write-Host ""
Write-Host "INSTRUCCIONES IMPORTANTES:" -ForegroundColor Yellow
Write-Host "1. Conecta el ESP32 a la USB"
Write-Host "2. Presiona y MANTÉN presionado el botón BOOT"
Write-Host "3. Presiona también el botón EN/RESET"
Write-Host "4. Suelta el botón EN/RESET (mantén BOOT presionado)"
Write-Host "5. Suelta el botón BOOT cuando comience el flasheo"
Write-Host "6. Presiona ENTER para continuar..."
Write-Host ""

$null = Read-Host

# ===================== FLASHEAR ESP32 =====================
Write-Header "Paso 5: Flashear MicroPython"

Write-Info "Borrando memoria flash..."
try {
    esptool.py --port $Port --baud $Baud erase_flash | Out-Null
    Write-Success "Memoria flash borrada"
} catch {
    Write-Error "Error al borrar la memoria flash"
    Write-Host "Intenta:"
    Write-Host "  - Verificar que el puerto $Port es correcto"
    Write-Host "  - Cambiar la velocidad de baud"
    exit 1
}

Write-Info "Escribiendo firmware MicroPython..."
try {
    esptool.py --port $Port --baud $Baud write_flash -z 0x1000 $firmwarePath | Out-Null
    Write-Success "Firmware flasheado correctamente"
} catch {
    Write-Error "Error al escribir el firmware"
    exit 1
}

# ===================== ESPERAR REINICIO =====================
Write-Header "Paso 6: Esperar reinicio del ESP32"

Write-Info "Esperando 5 segundos para reiniciar..."
Start-Sleep -Seconds 5
Write-Success "ESP32 reiniciado"

# ===================== CARGAR ARCHIVOS =====================
Write-Header "Paso 7: Cargar archivos de la demo"

Write-Info "Subiendo boot.py..."
try {
    ampy --port $Port put boot.py /boot.py
    Write-Success "boot.py cargado"
} catch {
    Write-Error "Error al cargar boot.py"
    Write-Host "(Continuando...)"
}

Write-Info "Subiendo main.py..."
try {
    ampy --port $Port put main.py /main.py
    Write-Success "main.py cargado"
} catch {
    Write-Error "Error al cargar main.py"
    exit 1
}

# ===================== VERIFICACIÓN =====================
Write-Header "Paso 8: Verificación de archivos"

Write-Info "Listando archivos en ESP32:"
try {
    $files = ampy --port $Port ls /
    Write-Host $files
    Write-Success "Archivos verificados"
} catch {
    Write-Error "Error al listar archivos"
}

# ===================== ABRIR MONITOR =====================
Write-Header "Paso 9: Monitor Serial"

Write-Info "Abriendo monitor serial..."
Write-Host "Presiona Ctrl+] para cerrar" -ForegroundColor Yellow

$pythonCmd = "import sys; from serial.tools.miniterm import main; sys.exit(main())"
python -m serial.tools.miniterm $Port 115200

# ===================== COMPLETADO =====================
Write-Header "PROCESO COMPLETADO"

Write-Host ""
Write-Host "Tu ESP32 está listo con MicroPython!" -ForegroundColor Green
Write-Host ""
Write-Host "CONFIGURACIÓN DE HARDWARE:" -ForegroundColor Yellow
Write-Host ""
Write-Host "  ESP32 GPIO25 -----> LED WS2812 Data (DIN)" -ForegroundColor Cyan
Write-Host "  ESP32 GND    -----> LED WS2812 GND" -ForegroundColor Cyan
Write-Host "  5V PSU       -----> LED WS2812 +5V" -ForegroundColor Cyan
Write-Host ""
Write-Host "Para cambiar el pin GPIO: edita LED_PIN en main.py" -ForegroundColor Yellow
Write-Host ""
