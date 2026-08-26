# PavaElectrica 2026 - ESP32 WS2812 LED Controller

## 📋 Descripción

Demo interactiva para controlar LEDs RGB WS2812 (NeoPixels) usando un **ESP32**.

**Este proyecto ofrece TWO versiones:**

| Versión | Lenguaje | Framework | Rendimiento | Complejidad | Mejor para |
|---------|----------|-----------|------------|-------------|-----------|
| **MicroPython** | Python | MicroPython | Interpretado | Fácil | Desarrollo rápido, prototipado |
| **C/ESP-IDF** | C | ESP-IDF | Compilado | Media-Alta | Producción, rendimiento |

---

## 🚀 INSTALACIÓN RÁPIDA

### ⚡ Opción 1: MicroPython (Recomendado para principiantes)

**Windows PowerShell:**
```powershell
cd PavaElectrica2026
.\flash_micropython.ps1 -Port COM3
```

**Windows CMD:**
```cmd
cd PavaElectrica2026
flash_micropython.bat
```

**Linux/macOS:**
```bash
cd PavaElectrica2026
chmod +x flash_micropython.sh
./flash_micropython.sh /dev/ttyUSB0
```

👉 **Ver [INSTALL_MICROPYTHON.md](INSTALL_MICROPYTHON.md) para detalles completos**

---

### 🔥 Opción 2: C/ESP-IDF (Rendimiento máximo)

**Windows PowerShell:**
```powershell
cd PavaElectrica2026
.\flash_esp32.ps1 -Port COM3
```

**Linux/macOS:**
```bash
cd PavaElectrica2026
./flash_esp32.sh /dev/ttyUSB0
```

👉 **Ver [INSTALL.md](INSTALL.md) para detalles completos**

---

## 🛠️ Hardware Requerido

- **ESP32** (Wemos D1 Mini 32, DevKitC, etc.)
- **Tira de LEDs WS2812** (NeoPixels)
- **Cables de conexión** (DuPont)
- **Fuente de alimentación** (5V @ 2A mínimo para tiras largas)

## 🔌 Conexiones

```
ESP32 GPIO25 -----> LED Strip Data (DIN)
ESP32 GND    -----> LED Strip GND
5V PSU       -----> LED Strip +5V
```

### Notas importantes:
- El pin GPIO25 puede cambiarse en configuración
- Se recomienda usar un condensador de 1000µF entre +5V y GND cerca de la tira
- Usar una resistencia de 470Ω entre GPIO25 y el pin DIN de los LEDs

---

## 📚 DOCUMENTACIÓN

### 📖 Por Versión

**MicroPython:**
- [INSTALL_MICROPYTHON.md](INSTALL_MICROPYTHON.md) - Instalación paso a paso
- [QUICKSTART.md](QUICKSTART.md) - 5 pasos rápidos
- [main.py](main.py) - Código fuente

**C/ESP-IDF:**
- [INSTALL.md](INSTALL.md) - Instalación de ESP-IDF
- [QUICKSTART.md](QUICKSTART.md) - 5 pasos rápidos
- [main/main.c](main/main.c) - Código fuente

### 📋 General

- [PROJECT_SUMMARY.md](PROJECT_SUMMARY.md) - Resumen completo del proyecto
- [MIGRATION.md](MIGRATION.md) - Comparación MicroPython vs C
- [ADVANCED.md](ADVANCED.md) - Debugging y optimización avanzada

---

## ✨ Características de la Demo

El programa ofrece un menú interactivo con los siguientes efectos:

| Opción | Efecto | Descripción |
|--------|--------|-------------|
| 1 | **Color Rojo Fijo** | Todos los LEDs en rojo |
| 2 | **Color Verde Fijo** | Todos los LEDs en verde |
| 3 | **Color Azul Fijo** | Todos los LEDs en azul |
| 4 | **Arco Iris** | Efecto arco iris rotativo |
| 5 | **Persecución Bicolor** | Patrón alternado de colores |
| 6 | **Pulsación** | Fade in/out suave |
| 7 | **Persecución Teatral** | Efecto tipo teatro |
| 8 | **Chispas Aleatorias** | LEDs parpadeando aleatoriamente |
| 9 | **Apagar LEDs** | Apaga todos los LEDs |
| 0 | **Reiniciar ESP32** | Reinicio del dispositivo |

---

## 🎯 ¿Cuál Versión Elegir?

### 🐍 Elige MicroPython SI:
- ✅ Quieres desarrollar rápidamente
- ✅ Prefieres código simple y legible
- ✅ No necesitas máximo rendimiento
- ✅ Eres nuevo en programación embebida
- ✅ Quieres usar REPL interactivo
- ✅ Necesitas ciclos de desarrollo cortos

### ⚙️ Elige C/ESP-IDF SI:
- ✅ Necesitas máximo rendimiento
- ✅ Quieres proyecto de producción
- ✅ Requieres control fino del hardware
- ✅ Tienes experiencia con C
- ✅ Necesitas optimizar memoria/CPU
- ✅ Deseas usar toolchain oficial de Espressif

---

## 📊 Comparación de Rendimiento

| Métrica | MicroPython | C/ESP-IDF |
|---------|------------|-----------|
| **Velocidad** | Interpretado | **10x más rápido** |
| **RAM** | ~2-3 MB | **10x menos (250 KB)** |
| **Flash** | ~1-2 MB | **2x menos (500 KB)** |
| **CPU Load** | ~95% | **6x menor (15%)** |
| **Desarrollo** | **Más rápido** | Compilación necesaria |

---

## 🎮 Características de la Demo

El programa ofrece un menú interactivo con los siguientes efectos:

| Opción | Efecto | Descripción |
|--------|--------|-------------|
| 1 | **Color Rojo Fijo** | Todos los LEDs en rojo |
| 2 | **Color Verde Fijo** | Todos los LEDs en verde |
| 3 | **Color Azul Fijo** | Todos los LEDs en azul |
| 4 | **Arco Iris** | Efecto arco iris rotativo (10 ciclos) |
| 5 | **Persecución Bicolor** | Rojo y verde alternando (5 ciclos) |
| 6 | **Pulsación** | Fade in/out en cian (3 ciclos) |
| 7 | **Persecución Teatral** | Efecto de persecución tipo teatro |
| 8 | **Chispas Aleatorias** | LEDs parpadeando aleatoriamente (5 seg) |
| 9 | **Apagar LEDs** | Apaga todos los LEDs |
| 0 | **Reiniciar ESP32** | Reinicia el dispositivo |

## ⚙️ Configuración

### MicroPython

Edita `main.py`:

```python
LED_PIN = 25      # Pin GPIO donde están conectados los LEDs
NUM_LEDS = 14     # Cantidad de LEDs en la tira
UPDATE_SPEED = 100 # Velocidad de actualización en ms
```

Luego carga el archivo:
```bash
ampy --port COM3 put main.py /main.py
```

Ver [INSTALL_MICROPYTHON.md](INSTALL_MICROPYTHON.md) para detalles.

### C/ESP-IDF

```bash
idf.py menuconfig
```

Navega a: **Component config** → **WS2812 LED Demo Configuration**

Opciones disponibles:
- `LED_GPIO_PIN`: GPIO donde están conectados los LEDs (default: 25)
- `NUM_LEDS`: Cantidad de LEDs en la tira (default: 10)
- `LED_UPDATE_SPEED_MS`: Velocidad de actualización en ms (default: 100)
- `UART_PORT`: Puerto UART (default: 0)
- `UART_BAUD_RATE`: Velocidad UART en baud (default: 115200)


## 🚀 Uso

### MicroPython

1. Conectar ESP32 a la USB
2. Ejecutar `flash_micropython.ps1` (Windows) o `flash_micropython.sh` (Linux/macOS)
3. El monitor serial se abre automáticamente
4. Usar el menú interactivo

👉 Detalles: [INSTALL_MICROPYTHON.md](INSTALL_MICROPYTHON.md)

### C/ESP-IDF

1. Instalar ESP-IDF (ver [INSTALL.md](INSTALL.md))
2. Conectar ESP32 a la USB
3. Ejecutar `flash_esp32.ps1` (Windows) o `flash_esp32.sh` (Linux/macOS)
4. El monitor serial se abre automáticamente
5. Usar el menú interactivo

👉 Detalles rápidos: [QUICKSTART.md](QUICKSTART.md)

---

## 🔧 Troubleshooting

| Problema | Solución |
|----------|----------|
| **LEDs no se encienden** | Verificar conexiones, voltaje 5V, GPIO correcto |
| **Colores raros** | Algunos LEDs usan GRB, cambiar orden de RGB |
| **ESP32 no se detecta** | Mantener BOOT presionado, reintentar |
| **No hay output serial** | Verificar puerto COM, velocidad 115200 baud |
| **Error en compilación (C)** | Ver [INSTALL.md](INSTALL.md) - Solución de problemas |

Para más soluciones, ver documentación específica de cada versión.

---

## 📊 Estructura del Proyecto

```
PavaElectrica2026/
│
├── MicroPython/
│   ├── boot.py              # Script de inicio
│   ├── main.py              # Lógica principal (~400 líneas)
│   ├── flash_micropython.ps1
│   ├── flash_micropython.bat
│   └── flash_micropython.sh
│
├── C-ESP-IDF/
│   ├── main/
│   │   ├── main.c           # Código principal (~680 líneas)
│   │   ├── CMakeLists.txt
│   │   └── Kconfig
│   ├── CMakeLists.txt
│   ├── Makefile
│   ├── sdkconfig
│   ├── flash_esp32.ps1
│   ├── flash_esp32.bat
│   └── flash_esp32.sh
│
├── Documentación/
│   ├── README.md            # Este archivo
│   ├── QUICKSTART.md        # 5 pasos rápidos
│   ├── INSTALL.md           # ESP-IDF (C)
│   ├── INSTALL_MICROPYTHON.md
│   ├── ADVANCED.md          # Debugging avanzado (C)
│   ├── MIGRATION.md         # Comparación Python vs C
│   └── PROJECT_SUMMARY.md   # Resumen completo
│
└── .gitignore
```

---

## 📚 Referencias

### MicroPython
- [MicroPython Documentation](https://docs.micropython.org/)
- [ESP32 MicroPython Guide](https://docs.micropython.org/en/latest/esp32/)
- [Thonny IDE](https://thonny.org/)

### C/ESP-IDF
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- [ESP32 Datasheet](https://www.espressif.com/en/products/socs/esp32/resources)
- [LED Strip Driver](https://components.espressif.com/components/esp/led_strip/)

---

## 📄 Licencia

Código: MIT License
Documentación: Creative Commons BY-SA 4.0

---

**PavaElectrica 2026** - Control de LEDs RGB para ESP32 ✨

*¿Preguntas? Ver la documentación específica de tu versión elegida.*