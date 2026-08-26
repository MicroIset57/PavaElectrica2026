#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Script para compilar y flashear ESP32 con ESP-IDF (C/C++)
    
.DESCRIPTION
    Este script automatiza el proceso de:
    1. Compilar el proyecto con ESP-IDF
    2. Flashear el ESP32
    3. Abrir el monitor serial
    
.PARAMETER Port
    Puerto COM del ESP32 (ej: COM3)
    
.PARAMETER Baud
    Velocidad en baudios (por defecto: 460800)
    
.EXAMPLE
    .\flash_esp32.ps1 -Port COM3
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
Write-Header "ESP32 WS2812 LED Controller - Build & Flash Script (C/ESP-IDF)"

Write-Info "Puerto configurado: $Port"
Write-Info "Velocidad: $Baud baud"

# ===================== VERIFICAR ESP-IDF =====================
Write-Header "Paso 1: Verificar ESP-IDF"

if (-not $env:IDF_PATH) {
    Write-Error "IDF_PATH no está configurado"
    Write-Host ""
    Write-Host "Por favor instala ESP-IDF desde:" -ForegroundColor Yellow
    Write-Host "https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/" -ForegroundColor Cyan
    exit 1
}

Write-Success "ESP-IDF encontrado en: $env:IDF_PATH"

# ===================== VERIFICAR PYTHON =====================
Write-Header "Paso 2: Verificar Python"

try {
    $pythonVersion = python --version 2>&1
    Write-Success "Python encontrado: $pythonVersion"
} catch {
    Write-Error "Python no está instalado"
    Write-Host "Descargalo desde: https://www.python.org/"
    exit 1
}

# ===================== INSTALAR ESPTOOL =====================
Write-Info "Instalando esptool..."
try {
    pip install --upgrade esptool -q
    Write-Success "esptool instalado"
} catch {
    Write-Error "Error al instalar esptool"
    exit 1
}

# ===================== COMPILAR PROYECTO =====================
Write-Header "Paso 3: Compilar el proyecto"

Write-Info "Compilando firmware con ESP-IDF..."
try {
    & idf.py build
    if ($LASTEXITCODE -ne 0) {
        throw "Compilación fallida"
    }
    Write-Success "Compilación exitosa"
} catch {
    Write-Error "Error durante la compilación"
    Write-Host "Intenta:"
    Write-Host "  1. Ejecutar: idf.py fullclean"
    Write-Host "  2. Ejecutar: idf.py menuconfig (para verificar config)"
    Write-Host "  3. Ejecutar de nuevo este script"
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
Write-Header "Paso 5: Flashear ESP32"

Write-Info "Flasheando firmware..."
try {
    & idf.py -p $Port flash
    if ($LASTEXITCODE -ne 0) {
        throw "Flasheo fallido"
    }
    Write-Success "Flasheo completado exitosamente"
} catch {
    Write-Error "Error durante el flasheo"
    Write-Host "Intenta:"
    Write-Host "  1. Mantener presionado el botón BOOT durante el flasheo"
    Write-Host "  2. Ejecutar: idf.py -p $Port erase_flash (borra la memoria)"
    Write-Host "  3. Ejecutar de nuevo este script"
    exit 1
}

# ===================== ABRIR MONITOR =====================
Write-Header "Paso 6: Monitor Serial"

Write-Info "Esperando a que ESP32 inicie..."
Start-Sleep -Seconds 3

Write-Info "Abriendo monitor serial..."
Write-Host "Presiona Ctrl+] para cerrar el monitor" -ForegroundColor Yellow

& idf.py -p $Port monitor

# ===================== COMPLETADO =====================
Write-Header "PROCESO COMPLETADO"

Write-Host ""
Write-Host "Tu ESP32 está listo para usar!" -ForegroundColor Green
Write-Host ""
Write-Host "CONFIGURACIÓN DE HARDWARE:" -ForegroundColor Yellow
Write-Host ""
Write-Host "  ESP32 GPIO25 -----> LED WS2812 Data (DIN)" -ForegroundColor Cyan
Write-Host "  ESP32 GND    -----> LED WS2812 GND" -ForegroundColor Cyan
Write-Host "  5V PSU       -----> LED WS2812 +5V" -ForegroundColor Cyan
Write-Host ""
Write-Host "Para cambiar el pin:" -ForegroundColor Yellow
Write-Host "  1. Ejecuta: idf.py menuconfig" -ForegroundColor Cyan
Write-Host "  2. Ve a: Component config > WS2812 LED Demo Configuration" -ForegroundColor Cyan
Write-Host "  3. Cambia LED_GPIO_PIN al pin que desees" -ForegroundColor Cyan
Write-Host ""

