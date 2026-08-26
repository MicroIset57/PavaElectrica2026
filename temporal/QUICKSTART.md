# 🚀 Quick Start Guide

## 5 Pasos para Comenzar

### Paso 1: Instalar ESP-IDF (5 minutos)

**Windows:**
```powershell
# Descargar instalador desde:
# https://github.com/espressif/idf-installer/releases
# Ejecutar y seguir los pasos
```

**Linux/macOS:**
```bash
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
source export.sh
```

Ver [INSTALL.md](INSTALL.md) para detalles completos.

---

### Paso 2: Conectar el ESP32

1. **Conecta el ESP32 a la USB**
2. **Identifica el puerto COM:**
   - Windows: Administrador de dispositivos → Puertos (COM y LPT)
   - Linux: `ls /dev/tty*` (generalmente `/dev/ttyUSB0`)
   - macOS: `ls /dev/tty.*` (generalmente `/dev/tty.usbserial-*`)

---

### Paso 3: Compilar y Flashear

**Opción A: Usar script (Recomendado)**

Windows PowerShell:
```powershell
cd PavaElectrica2026
.\flash_esp32.ps1 -Port COM3
```

Windows CMD:
```cmd
cd PavaElectrica2026
flash_esp32.bat
```

Linux/macOS:
```bash
cd PavaElectrica2026
chmod +x flash_esp32.sh
./flash_esp32.sh /dev/ttyUSB0
```

**Opción B: Comandos manuales**

```bash
# Compilar
idf.py build

# Flashear
idf.py -p COM3 flash

# Monitorear
idf.py -p COM3 monitor
```

---

### Paso 4: Conectar los LEDs

```
ESP32 GPIO25 -----> LED WS2812 DIN
ESP32 GND    -----> LED WS2812 GND
5V PSU       -----> LED WS2812 +5V
```

> ⚠️ **Importante:** Usar fuente de 5V separada si la tira tiene más de 10 LEDs

---

### Paso 5: Usar la Demo

En el monitor serial (115200 baud):

```
==================================================
      ESP32 WS2812 RGB LED DEMO
==================================================
1. Color Rojo Fijo
2. Color Verde Fijo
3. Color Azul Fijo
4. Arco Iris
5. Persecución Bicolor
6. Pulsación
7. Persecución Teatral
8. Chispas Aleatorias
9. Apagar LEDs
0. Reiniciar ESP32
==================================================

Ingresa una opción (0-9): _
```

Ingresa el número y presiona ENTER.

---

## 🎯 Configuración Rápida

### Cambiar el pin de los LEDs

```bash
idf.py menuconfig
```

Navega a:
```
Component config → WS2812 LED Demo Configuration → LED_GPIO_PIN
```

Cambia `25` por tu pin (ej: `32`, `18`, etc.)

### Cambiar cantidad de LEDs

```bash
idf.py menuconfig
```

Navega a:
```
Component config → WS2812 LED Demo Configuration → NUM_LEDS
```

Ingresa la cantidad (ej: `30`, `60`, etc.)

### Cambiar velocidad de UART

```bash
idf.py menuconfig
```

Navega a:
```
Component config → WS2812 LED Demo Configuration → UART_BAUD_RATE
```

Valores típicos: `115200`, `230400`, `460800`

---

## 🔧 Solución Rápida de Problemas

| Problema | Solución |
|----------|----------|
| **LEDs no se encienden** | Verificar conexiones físicas, voltaje 5V |
| **ESP32 no se detecta** | Mantener BOOT presionado mientras conectas |
| **Error de compilación** | `idf.py fullclean` y luego `idf.py build` |
| **No hay output serial** | Verificar puerto COM, reiniciar ESP32 |
| **Colores raros** | Probar cambiar orden RGB en `main/main.c` |

---

## 📊 Archivo de Configuración

Después de configurar con `menuconfig`, los cambios se guardan en `sdkconfig`.

Para restablecer:
```bash
rm sdkconfig
idf.py menuconfig
```

---

## 💾 Guardar tu Configuración

Para compartir la configuración:

```bash
# Copiar configuración
cp sdkconfig sdkconfig.example

# Restaurar desde copia
cp sdkconfig.example sdkconfig
idf.py build
```

---

## 🎓 Próximos Pasos

1. **Leer [README.md](README.md)** - Documentación completa
2. **Ver [ADVANCED.md](ADVANCED.md)** - Debugging y optimización
3. **Entender [MIGRATION.md](MIGRATION.md)** - Cambios de MicroPython a C

---

## 📞 Obtener Ayuda

- 📖 [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- 💬 [ESP32 Forum](https://esp32.com/)
- 🐛 [Issues de GitHub](https://github.com/espressif/esp-idf/issues)

---

**¿Listo? Comienza con el Paso 1 arriba. ¡Debería funcionar en 15 minutos!** ⏱️✨
