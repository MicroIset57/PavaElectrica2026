# Configuración Avanzada y Debugging

## 🐛 Debugging en ESP-IDF

### 1. Nivel de Log

Para ver más detalles en la compilación:

```bash
# Verbose logging
idf.py -vvv build

# Super verbose
idf.py -vvvv build
```

### 2. Monitor Serial con Salida Formateada

```bash
# Monitor con decodificación de excepciones
idf.py -p COM3 monitor

# En el monitor, atajos útiles:
# Ctrl+]     - Salir del monitor
# Ctrl+T     - Submenú de comando
# Ctrl+A     - Ir a inicio de línea
# Ctrl+B     - Bajar el nivel de log (menos verbose)
# Ctrl+U     - Subir el nivel de log (más verbose)
```

### 3. Compilación en Debug

```bash
# Configurar modo debug
idf.py menuconfig
# Ir a: Compiler options → Optimization Level → Debug (-Og)

# Compilar con símbolos de debug
idf.py build
```

### 4. JTAG Debugging (Opcional)

Para debugging a nivel de CPU con un adaptador JTAG:

```bash
# Conectar debugger ESP-PROG o similar
idf.py openocd

# En otra terminal
idf.py gdb
```

---

## 📊 Análisis de Memoria

### Ver uso de SRAM/DRAM

```bash
# Mostrar estadísticas de memoria después de compilar
idf.py size

# Mostrar detalle por componente
idf.py size-components
```

### Optimizar consumo de memoria

En `main/Kconfig`:

```ini
config FREERTOS_TASK_STACK_SIZE_MIN
    int "Tamaño mínimo de stack de tarea"
    default 4096
    # Reducir si es posible
```

---

## ⚡ Optimización de Rendimiento

### 1. Aumentar frecuencia de CPU

```bash
idf.py menuconfig
# Device Drivers → CPU frequency → 240 MHz (default es 160 MHz)
```

### 2. Habilitar caché de instrucciones

```bash
idf.py menuconfig
# Performance → Code Cache on PSRAM → Enabled
```

### 3. Usar optimizaciones de compilador

En `CMakeLists.txt`:

```cmake
idf_build_set_property(COMPILE_OPTIONS "-O3" APPEND)
```

---

## 🔧 Personalizaciones Útiles

### 1. Cambiar velocidad UART

En `main/Kconfig`:

```ini
config UART_BAUD_RATE
    int "Velocidad UART"
    default 115200
    # Cambiar a 230400 o 460800 para más velocidad
```

### 2. Aumentar número de LEDs

```bash
idf.py menuconfig
# Component config → WS2812 LED Demo → NUM_LEDS → 30 (o más)
```

### 3. Cambiar velocidad de actualización

```bash
idf.py menuconfig
# Component config → WS2812 LED Demo → LED_UPDATE_SPEED_MS → 50 (más rápido)
```

---

## 📝 Limpiar y Recompilar

### Limpiar archivos de compilación

```bash
# Limpiar build
idf.py clean

# Limpiar todo completamente
idf.py fullclean

# Borrar caché de componentes
rm -rf managed_components/
```

### Recompilar desde cero

```bash
idf.py fullclean
idf.py build
```

---

## 🔍 Inspeccionar Flash

### Ver contenido del flash

```bash
# Leer 256 bytes desde dirección 0x0
esptool.py -p COM3 read_flash 0x0 256 dump.bin

# Mostrar contenido en hexadecimal
hexdump -C dump.bin
```

### Verificar integridad

```bash
# Comparar con firmware compilado
esptool.py -p COM3 verify_flash path/to/firmware.bin
```

---

## 📦 Crear un Binary Release

```bash
# Generar archivo de release
idf.py build

# El binario está en: build/ws2812_led_demo.bin
# Copiar para distribución
cp build/ws2812_led_demo.bin releases/

# También incluir offset de flasheo
# Crear script de flasheo simplificado
```

---

## 🚨 Problemas Comunes de Compilación

### "Component 'xxx' not found"

```bash
# Actualizar componentes
idf.py update-requirements

# O descargar manualmente
idf.py add-dependency esp/led_strip
```

### "malloc heap size is too small"

En `menuconfig`:
```
Compiler options → Number of usable SRAM banks → 8
```

### "Stack overflow"

```bash
# Aumentar stack de la tarea principal
idf.py menuconfig
# Component config → FreeRTOS → Main task stack size → 8192
```

---

## 💾 Backup y Restauración

### Hacer backup del firmware

```bash
# Leer todo el flash
esptool.py -p COM3 read_flash 0 0x400000 esp32_backup.bin

# Restaurar
esptool.py -p COM3 write_flash 0 esp32_backup.bin
```

---

## 🎓 Referencias Técnicas

- [ESP-IDF Build System](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html)
- [Debugging Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/jtag-debugging/)
- [Performance Optimization](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/performance/)

---

¡Usa estas herramientas para optimizar y debuggear tu proyecto! 🔧✨
