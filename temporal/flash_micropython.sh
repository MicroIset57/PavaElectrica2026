#!/bin/bash

# Script para instalar MicroPython en ESP32 y cargar la demo de LEDs WS2812
# Linux/macOS Bash Script

# Variables de color
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Parámetros
PORT="${1:-/dev/ttyUSB0}"
BAUD="${2:-460800}"

# Funciones
print_header() {
    echo ""
    echo -e "${CYAN}=================================================${NC}"
    echo -e "${CYAN}$1${NC}"
    echo -e "${CYAN}=================================================${NC}"
    echo ""
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}→ $1${NC}"
}

# ===================== INICIO =====================
print_header "ESP32 WS2812 LED Controller - MicroPython Installer"

print_info "Puerto: $PORT"
print_info "Velocidad: $BAUD baud"

# ===================== VERIFICAR PYTHON =====================
print_header "Paso 1: Verificar Python"

if ! command -v python3 &> /dev/null; then
    print_error "Python3 no está instalado"
    echo "Instálalo con: sudo apt install python3 (Linux) o brew install python3 (macOS)"
    exit 1
fi

PYTHON_VERSION=$(python3 --version 2>&1)
print_success "Python encontrado: $PYTHON_VERSION"

# ===================== INSTALAR HERRAMIENTAS =====================
print_header "Paso 2: Instalar herramientas necesarias"

print_info "Instalando esptool..."
pip3 install --upgrade esptool --quiet
if [ $? -eq 0 ]; then
    print_success "esptool instalado"
else
    print_error "Error al instalar esptool"
    exit 1
fi

print_info "Instalando adafruit-ampy..."
pip3 install --upgrade adafruit-ampy --quiet
if [ $? -eq 0 ]; then
    print_success "adafruit-ampy instalado"
else
    print_error "Error al instalar adafruit-ampy"
    exit 1
fi

# ===================== BUSCAR FIRMWARE =====================
print_header "Paso 3: Buscar firmware MicroPython"

FIRMWARE=$(ls esp32-*.bin 2>/dev/null | head -n1)

if [ -z "$FIRMWARE" ]; then
    print_error "No se encontró archivo esp32-*.bin"
    echo ""
    echo -e "${YELLOW}PASOS:${NC}"
    echo "1. Abre en tu navegador:"
    echo "   https://micropython.org/download/esp32/"
    echo "2. Descarga el archivo .bin más reciente"
    echo "   (ej: esp32-20240602-v1.24.0.bin)"
    echo "3. Guarda el archivo en esta carpeta: $(pwd)"
    echo "4. Ejecuta este script de nuevo"
    echo ""
    exit 1
fi

print_success "Firmware encontrado: $FIRMWARE"

# ===================== PREPARAR PARA FLASHEO =====================
print_header "Paso 4: Preparar ESP32 para flasheo"

echo ""
echo -e "${YELLOW}INSTRUCCIONES IMPORTANTES:${NC}"
echo "1. Conecta el ESP32 a la USB"
echo "2. Presiona y MANTÉN presionado el botón BOOT"
echo "3. Presiona también el botón EN/RESET"
echo "4. Suelta el botón EN/RESET (mantén BOOT presionado)"
echo "5. Suelta el botón BOOT cuando comience el flasheo"
echo "6. Presiona ENTER para continuar..."
echo ""

read -p ""

# ===================== FLASHEAR ESP32 =====================
print_header "Paso 5: Flashear MicroPython"

print_info "Borrando memoria flash..."
esptool.py --port $PORT --baud $BAUD erase_flash

if [ $? -ne 0 ]; then
    print_error "Error al borrar la memoria flash"
    exit 1
fi

print_success "Memoria flash borrada"

print_info "Escribiendo firmware MicroPython..."
esptool.py --port $PORT --baud $BAUD write_flash -z 0x1000 "$FIRMWARE"

if [ $? -ne 0 ]; then
    print_error "Error al escribir el firmware"
    exit 1
fi

print_success "Firmware flasheado exitosamente"

# ===================== ESPERAR REINICIO =====================
print_header "Paso 6: Esperar reinicio del ESP32"

print_info "Esperando 5 segundos..."
sleep 5
print_success "ESP32 reiniciado"

# ===================== CARGAR ARCHIVOS =====================
print_header "Paso 7: Cargar archivos de la demo"

print_info "Subiendo boot.py..."
ampy --port $PORT put boot.py /boot.py
if [ $? -eq 0 ]; then
    print_success "boot.py cargado"
else
    print_error "Error al cargar boot.py (continuando...)"
fi

print_info "Subiendo main.py..."
ampy --port $PORT put main.py /main.py
if [ $? -eq 0 ]; then
    print_success "main.py cargado"
else
    print_error "Error al cargar main.py"
    exit 1
fi

# ===================== VERIFICACIÓN =====================
print_header "Paso 8: Verificación de archivos"

print_info "Listando archivos en ESP32:"
ampy --port $PORT ls /

# ===================== COMPLETADO =====================
print_header "PROCESO COMPLETADO"

echo ""
print_success "Tu ESP32 está listo con MicroPython!"
echo ""
echo -e "${YELLOW}CONFIGURACIÓN DE HARDWARE:${NC}"
echo ""
echo -e "${CYAN}  ESP32 GPIO25 -----> LED WS2812 Data (DIN)${NC}"
echo -e "${CYAN}  ESP32 GND    -----> LED WS2812 GND${NC}"
echo -e "${CYAN}  5V PSU       -----> LED WS2812 +5V${NC}"
echo ""
echo -e "${YELLOW}Para cambiar el pin GPIO: edita LED_PIN en main.py${NC}"
echo ""
echo "El programa está listo en: /main.py"
echo "Presiona RESET del ESP32 para ejecutar"
echo ""
