# 📦 Guía de Instalación - MicroPython para ESP32

## 🚀 Instalación Rápida (Script Automático) - Recomendado

### Windows - PowerShell

```powershell
cd PavaElectrica2026
.\flash_micropython.ps1 -Port COM3
```

Reemplaza `COM3` con tu puerto actual.

### Windows - Batch

```cmd
cd PavaElectrica2026
flash_micropython.bat
```

### Linux/macOS - Bash

```bash
cd PavaElectrica2026
chmod +x flash_micropython.sh
./flash_micropython.sh /dev/ttyUSB0
```

Reemplaza `/dev/ttyUSB0` con tu puerto (ver abajo cómo encontrarlo).

---

## 📋 Requisitos Previos

### 1. Python 3.7+
- **Windows**: Descargar desde https://www.python.org/
- **Linux**: `sudo apt install python3`
- **macOS**: `brew install python3`

### 2. Descargar Firmware de MicroPython

1. Abre: https://micropython.org/download/esp32/
2. Descarga la última versión estable (ej: `esp32-20240602-v1.24.0.bin`)
3. Guarda el archivo en la carpeta `PavaElectrica2026`

---

## 🔍 Encontrar tu Puerto COM

### Windows

1. **Administrador de dispositivos**
   - Presiona `Win + R`
   - Escribe `devmgmt.msc` y presiona Enter
   - Expande "Puertos (COM y LPT)"
   - Tu ESP32 aparecerá como `USB-SERIAL CH340 (COM3)` o similar
   - Anota el puerto (ej: `COM3`)

2. **Usando PowerShell**
   ```powershell
   Get-CimInstance Win32_SerialPort | Select-Object Name, Description
   ```

### Linux

```bash
# Ver todos los puertos seriales
ls /dev/tty*

# El ESP32 aparecerá como /dev/ttyUSB0 o /dev/ttyUSB1
# Si necesitas permisos:
sudo usermod -a -G dialout $USER
# (Requiere logout/login)
```

### macOS

```bash
# Ver puertos USB
ls /dev/tty.*

# El ESP32 aparecerá como /dev/tty.usbserial-XXXXXXXX
```

---

## ⚙️ Instalación Manual Paso a Paso

Si prefieres hacer el flasheo manualmente:

### Paso 1: Instalar herramientas

```bash
pip install esptool adafruit-ampy
```

### Paso 2: Borrar la memoria flash

```bash
esptool.py --port COM3 --baud 460800 erase_flash
```

### Paso 3: Flashear MicroPython

```bash
esptool.py --port COM3 --baud 460800 write_flash -z 0x1000 esp32-20240602-v1.24.0.bin
```

### Paso 4: Esperar reinicio (5 segundos)

### Paso 5: Cargar archivos

```bash
ampy --port COM3 put boot.py /boot.py
ampy --port COM3 put main.py /main.py
```

### Paso 6: Verificar

```bash
ampy --port COM3 ls /
```

Deberías ver:
```
boot.py
main.py
```

---

## 🔧 Configuración Post-Instalación

### Cambiar pin GPIO de los LEDs

Edita `main.py`:

```python
LED_PIN = 25  # Cambiar a tu pin (ej: 32, 18, etc.)
```

### Cambiar cantidad de LEDs

En `main.py`:

```python
NUM_LEDS = 14  # Cambiar según tu tira
```

### Cambiar velocidad de actualización

En `main.py`:

```python
UPDATE_SPEED = 100  # Milisegundos (menor = más rápido)
```

Después de cambios, recarga los archivos:

```bash
ampy --port COM3 put main.py /main.py
```

---

## 🎮 Usar la Demo

### Opción 1: Monitor Serial (Interactivo)

```bash
# Windows
python -m serial.tools.miniterm COM3 115200

# Linux/macOS
screen /dev/ttyUSB0 115200
```

Verás un menú como este:

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

Ingresa un número y presiona ENTER.

### Opción 2: Ejecución Automática

El programa se ejecuta automáticamente cuando enciendas el ESP32 (se carga desde `boot.py` y `main.py`).

---

## 🔌 Conexiones de Hardware

```
ESP32 GPIO25 -----> LED WS2812 Data (DIN)
ESP32 GND    -----> LED WS2812 GND
5V PSU       -----> LED WS2812 +5V
```

### Notas Importantes

- Usar fuente de 5V separada si hay más de 10 LEDs
- Agregar condensador 1000µF entre +5V y GND
- Resistencia de 470Ω entre GPIO25 y DIN del LED
- Los LEDs son sensibles al orden de conexión

---

## 🐛 Solución de Problemas

### Problema: "No se encontró archivo esp32-*.bin"

**Solución:**
1. Abre https://micropython.org/download/esp32/
2. Descarga el .bin más reciente
3. Guarda en la carpeta `PavaElectrica2026`
4. Ejecuta el script de nuevo

### Problema: "Permiso denegado" en Linux

**Solución:**
```bash
sudo usermod -a -G dialout $USER
# Luego cierra sesión y vuelve a iniciar
```

### Problema: "Puerto COM no encontrado"

**Solución:**
- Verificar que el ESP32 está conectado
- Probar otro cable USB
- Presionar BOOT mientras conectas
- Ejecutar script con puerto manual: `.\flash_micropython.ps1 -Port COM4`

### Problema: "Error durante flasheo"

**Solución:**
```bash
# Intentar con menor velocidad de baud
esptool.py --port COM3 --baud 115200 write_flash -z 0x1000 esp32-*.bin
```

### Problema: "Los LEDs no se encienden"

**Solución:**
- Verificar conexiones físicas
- Comprobar voltaje en la tira (debe ser 5V)
- Revisar que GPIO25 sea el correcto
- Intentar con menos LEDs primero

### Problema: "Colores extraños en LEDs"

**Solución:**
Algunos LEDs tienen orden GRB en lugar de RGB.

En `main.py`, cambia:
```python
# De:
np[i] = (r, g, b)

# A:
np[i] = (g, r, b)
```

---

## 💻 Editar Código en el ESP32

### Opción 1: Thonny IDE (Recomendado para principiantes)

1. Descargar: https://thonny.org/
2. Instalar
3. Conectar ESP32
4. En Thonny: View → Files (muestra archivos del ESP32)
5. Editar `main.py` directamente en el ESP32
6. Presionar RESET para aplicar cambios

### Opción 2: Editor Local + ampy

1. Editar `main.py` con tu editor favorito
2. Guardar
3. Cargar al ESP32:
   ```bash
   ampy --port COM3 put main.py /main.py
   ```
4. Presionar RESET en el ESP32

### Opción 3: REPL Interactivo

```bash
# Windows
python -m serial.tools.miniterm COM3 115200

# Presionar Ctrl+A para entrar en REPL
# Escribir código Python directo
```

---

## 📚 Extensiones Recomendadas

### Agregar más efectos

En `main.py`, agrega funciones como:

```python
def effect_strobe(r, g, b, speed=100):
    """Efecto de estroboscópico"""
    for _ in range(10):
        set_all_color(r, g, b)
        time.sleep_ms(speed)
        clear_leds()
        time.sleep_ms(speed)
```

### Usar botones

```python
import machine

# Botón en GPIO0 (BOOT)
button = machine.Pin(0, machine.Pin.IN)

if button.value() == 0:
    effect_rainbow()
```

### Usar sensores

```python
from machine import ADC

# Sensor de luz en GPIO34
sensor = ADC(machine.Pin(34))
brightness = sensor.read()  # 0-4095
```

---

## 🔗 Referencias Útiles

- [MicroPython Docs](https://docs.micropython.org/)
- [ESP32 MicroPython](https://docs.micropython.org/en/latest/esp32/)
- [NeoPixel Library](https://docs.micropython.org/en/latest/esp8266/tutorial/neopixel.html)
- [Foro ESP32](https://esp32.com/)

---

## 📝 Comandos Útiles con ampy

```bash
# Listar archivos
ampy --port COM3 ls /

# Ver contenido de archivo
ampy --port COM3 cat /main.py

# Eliminar archivo
ampy --port COM3 rm /main.py

# Crear carpeta
ampy --port COM3 mkdir /lib

# Ejecutar archivo
ampy --port COM3 run script.py
```

---

## ✅ Checklist de Instalación

- [ ] Descargar firmware MicroPython (.bin)
- [ ] Identificar puerto COM del ESP32
- [ ] Ejecutar script de instalación
- [ ] Conectar LEDs WS2812
- [ ] Verificar con monitor serial
- [ ] Probar cada efecto del menú
- [ ] Cambiar configuración (GPIO, NUM_LEDS, etc.)

---

**¡Instalación completada!** 🎉

Tu ESP32 está listo con MicroPython y la demo de LEDs WS2812.

Para más información, ver [README.md](README.md)
