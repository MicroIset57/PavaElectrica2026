#!/usr/bin/env python3
"""
ESP32 Boot Script - Configuración de inicio
"""

import machine
import time

# Desactivar garbage collection durante la inicialización
import gc
gc.disable()

# Configurar el LED onboard como indicador
try:
    led = machine.Pin(2, machine.Pin.OUT)  # GPIO2 es el LED onboard de muchos ESP32
    led.on()
    time.sleep_ms(100)
    led.off()
except:
    pass

# Habilitar garbage collection después de la inicialización
gc.enable()

# Print de bienvenida
print("=" * 50)
print("PavaElectrica 2026 - WS2812 LED Controller")
print("=" * 50)
print(f"Frecuencia CPU: {machine.freq() / 1_000_000:.1f} MHz")
print("Boot completado. Iniciando main.py...")
print("=" * 50)
