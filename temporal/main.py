#!/usr/bin/env python3
"""
ESP32 WS2812 RGB LED Demo - Programa Principal
Controla una tira de LEDs RGB WS2812 (NeoPixels) en un ESP32
"""

import time
import machine
import neopixel

# ======================== CONFIGURACIÓN ========================
# Pin GPIO donde están conectados los LEDs WS2812
LED_PIN = 25  # Cambiar según tu configuración
# Cantidad de LEDs en la tira
NUM_LEDS = 14
# Velocidad de actualización (ms)
UPDATE_SPEED = 100

# ======================== INICIALIZACIÓN ========================
# Crear objeto NeoPixel
np = neopixel.NeoPixel(machine.Pin(LED_PIN), NUM_LEDS)

# ======================== FUNCIONES AUXILIARES ========================
def set_all_color(r, g, b):
    """Establece todos los LEDs al mismo color RGB"""
    for i in range(NUM_LEDS):
        np[i] = (r, g, b)
    np.write()

def clear_leds():
    """Apaga todos los LEDs"""
    set_all_color(0, 0, 0)

def rainbow(brightness=255):
    """Efecto arco iris en los LEDs"""
    for shift in range(360):
        for i in range(NUM_LEDS):
            # Calcular matiz para cada LED
            hue = (shift + i * 360 // NUM_LEDS) % 360
            # Convertir HSV a RGB
            r, g, b = hsv_to_rgb(hue, 255, brightness)
            np[i] = (r, g, b)
        np.write()
        time.sleep_ms(UPDATE_SPEED)

def chase_colors(color1, color2, speed=200):
    """Efecto de persecución con dos colores"""
    for cycle in range(NUM_LEDS * 2):
        for i in range(NUM_LEDS):
            if (i + cycle) % 2 == 0:
                np[i] = color1
            else:
                np[i] = color2
        np.write()
        time.sleep_ms(speed)

def pulse(r, g, b, cycles=3, speed=50):
    """Efecto de pulsación suave"""
    brightness_vals = list(range(0, 256, 5)) + list(range(255, -1, -5))
    
    for _ in range(cycles):
        for brightness in brightness_vals:
            factor = brightness / 255
            set_all_color(
                int(r * factor),
                int(g * factor),
                int(b * factor)
            )
            time.sleep_ms(speed)

def theater_chase(r, g, b, delay=100):
    """Efecto de persecución tipo teatro"""
    for q in range(3):
        for i in range(0, NUM_LEDS, 3):
            # Encender cada 3er LED
            for j in range(NUM_LEDS):
                if (j + q) % 3 == 0:
                    np[j] = (r, g, b)
                else:
                    np[j] = (0, 0, 0)
            np.write()
            time.sleep_ms(delay)

def sparkles(num_sparkles=3, duration=3000):
    """Efecto de chispas aleatorias"""
    import random
    start_time = time.ticks_ms()
    
    while time.ticks_diff(time.ticks_ms(), start_time) < duration:
        clear_leds()
        for _ in range(num_sparkles):
            idx = random.randint(0, NUM_LEDS - 1)
            r = random.randint(100, 255)
            g = random.randint(100, 255)
            b = random.randint(100, 255)
            np[idx] = (r, g, b)
        np.write()
        time.sleep_ms(100)

def hsv_to_rgb(h, s, v):
    """Convierte HSV a RGB
    h: 0-360 (matiz)
    s: 0-255 (saturación)
    v: 0-255 (valor/brillo)
    """
    h = h % 360
    s = s / 255
    v = v / 255
    
    c = v * s
    x = c * (1 - abs((h / 60) % 2 - 1))
    m = v - c
    
    if h < 60:
        r, g, b = c, x, 0
    elif h < 120:
        r, g, b = x, c, 0
    elif h < 180:
        r, g, b = 0, c, x
    elif h < 240:
        r, g, b = 0, x, c
    elif h < 300:
        r, g, b = x, 0, c
    else:
        r, g, b = c, 0, x
    
    return (
        int((r + m) * 255),
        int((g + m) * 255),
        int((b + m) * 255)
    )

# ======================== MENÚ DEMO ========================
def show_menu():
    """Muestra el menú de demostración"""
    print("\n" + "="*50)
    print("ESP32 WS2812 RGB LED DEMO")
    print("="*50)
    print("1. Color Rojo Fijo")
    print("2. Color Verde Fijo")
    print("3. Color Azul Fijo")
    print("4. Arco Iris")
    print("5. Persecución Bicolor")
    print("6. Pulsación")
    print("7. Persecución Teatral")
    print("8. Chispas Aleatorias")
    print("9. Apagar LEDs")
    print("0. Reiniciar")
    print("="*50)

def run_demo(option):
    """Ejecuta la demostración seleccionada"""
    if option == 1:
        print("Mostrando color Rojo...")
        set_all_color(255, 0, 0)
        time.sleep(3)
    elif option == 2:
        print("Mostrando color Verde...")
        set_all_color(0, 255, 0)
        time.sleep(3)
    elif option == 3:
        print("Mostrando color Azul...")
        set_all_color(0, 0, 255)
        time.sleep(3)
    elif option == 4:
        print("Efecto Arco Iris (10 ciclos)...")
        for _ in range(10):
            rainbow(200)
    elif option == 5:
        print("Persecución Bicolor (Rojo-Verde)...")
        for _ in range(5):
            chase_colors((255, 0, 0), (0, 255, 0), speed=150)
    elif option == 6:
        print("Pulsación Cian...")
        for _ in range(3):
            pulse(0, 255, 255, cycles=2, speed=30)
    elif option == 7:
        print("Persecución Teatral (Blanco)...")
        for _ in range(2):
            theater_chase(255, 255, 255, delay=100)
    elif option == 8:
        print("Chispas Aleatorias...")
        sparkles(num_sparkles=5, duration=5000)
    elif option == 9:
        print("Apagando LEDs...")
        clear_leds()
    elif option == 0:
        print("Reiniciando...")
        machine.reset()

# ======================== PROGRAMA PRINCIPAL ========================
if __name__ == "__main__":
    print("\n✓ ESP32 inicializado correctamente")
    print(f"✓ {NUM_LEDS} LEDs WS2812 configurados en GPIO {LED_PIN}")
    
    # Luz de bienvenida
    clear_leds()
    time.sleep(0.5)
    set_all_color(0, 100, 0)  # Verde
    time.sleep(0.5)
    clear_leds()
    
    try:
        while True:
            show_menu()
            print("\nIngresa el número de la opción: ", end="")
            
            # Esperar entrada del usuario
            option_str = input().strip()
            
            try:
                option = int(option_str)
                if 0 <= option <= 9:
                    run_demo(option)
                else:
                    print("Opción inválida. Por favor ingresa un número entre 0 y 9")
            except ValueError:
                print("Entrada inválida. Por favor ingresa un número")
            
            clear_leds()
            time.sleep(0.5)
    
    except KeyboardInterrupt:
        print("\n\nPrograma interrumpido por el usuario")
        clear_leds()
        print("LEDs apagados. ¡Hasta luego!")
