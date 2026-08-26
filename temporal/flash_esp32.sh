#!/bin/bash

# Script para compilar y flashear ESP32 con ESP-IDF (C/C++)
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
print_header "ESP32 WS2812 LED Controller - Build & Flash Script"

print_info "Puerto: $PORT"
print_info "Velocidad: $BAUD baud"

# ===================== VERIFICAR ESP-IDF =====================
print_header "Paso 1: Verificar ESP-IDF"

if [ -z "$IDF_PATH" ]; then
    print_error "IDF_PATH no está configurado"
    echo ""
    echo "Por favor instala ESP-IDF desde:"
    echo "https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/"
    exit 1
fi

if [ ! -d "$IDF_PATH" ]; then
    print_error "Directorio IDF_PATH no existe: $IDF_PATH"
    exit 1
fi

print_success "ESP-IDF encontrado en: $IDF_PATH"

# ===================== VERIFICAR PYTHON =====================
print_header "Paso 2: Verificar Python"

if ! command -v python3 &> /dev/null; then
    print_error "Python3 no está instalado"
    echo "Instálalo con: sudo apt install python3 (Linux) o brew install python3 (macOS)"
    exit 1
fi

PYTHON_VERSION=$(python3 --version 2>&1)
print_success "Python encontrado: $PYTHON_VERSION"

# ===================== INSTALAR ESPTOOL =====================
print_header "Paso 3: Instalar esptool"

print_info "Instalando esptool..."
pip3 install --upgrade esptool --quiet
if [ $? -eq 0 ]; then
    print_success "esptool instalado"
else
    print_error "Error al instalar esptool"
    exit 1
fi

# ===================== COMPILAR PROYECTO =====================
print_header "Paso 4: Compilar el proyecto"

print_info "Compilando firmware con ESP-IDF..."
idf.py build

if [ $? -ne 0 ]; then
    print_error "Error durante la compilación"
    echo ""
    echo "Intenta:"
    echo "  1. idf.py fullclean"
    echo "  2. idf.py menuconfig (para verificar configuración)"
    echo "  3. Ejecuta este script de nuevo"
    exit 1
fi

print_success "Compilación exitosa"

# ===================== PREPARAR PARA FLASHEO =====================
print_header "Paso 5: Preparar ESP32 para flasheo"

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
print_header "Paso 6: Flashear ESP32"

print_info "Flasheando firmware..."
idf.py -p $PORT flash

if [ $? -ne 0 ]; then
    print_error "Error durante el flasheo"
    echo ""
    echo "Intenta:"
    echo "  1. Mantener presionado el botón BOOT durante el flasheo"
    echo "  2. idf.py -p $PORT erase_flash (borra la memoria)"
    echo "  3. Ejecuta este script de nuevo"
    exit 1
fi

print_success "Flasheo completado exitosamente"

# ===================== ABRIR MONITOR =====================
print_header "Paso 7: Monitor Serial"

print_info "Esperando a que ESP32 inicie..."
sleep 3

print_info "Abriendo monitor serial..."
echo -e "${YELLOW}Presiona Ctrl+] para cerrar el monitor${NC}"
echo ""

idf.py -p $PORT monitor

# ===================== COMPLETADO =====================
print_header "PROCESO COMPLETADO"

echo ""
print_success "Tu ESP32 está listo para usar!"
echo ""
echo -e "${YELLOW}CONFIGURACIÓN DE HARDWARE:${NC}"
echo ""
echo -e "${CYAN}  ESP32 GPIO25 -----> LED WS2812 Data (DIN)${NC}"
echo -e "${CYAN}  ESP32 GND    -----> LED WS2812 GND${NC}"
echo -e "${CYAN}  5V PSU       -----> LED WS2812 +5V${NC}"
echo ""
echo -e "${YELLOW}Para cambiar el pin:${NC}"
echo "  1. idf.py menuconfig"
echo "  2. Navega a: Component config > WS2812 LED Demo Configuration"
echo "  3. Cambia LED_GPIO_PIN al pin que desees"
echo ""
