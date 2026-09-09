# Documentacion del codigo

## 1. Funcion del programa

El firmware controla una pava electrica con un ESP32. Sus funciones principales son:

- Medir la temperatura con un sensor DS18B20.
- Mostrar la temperatura y el nivel de calentamiento en un OLED.
- Representar la temperatura con una tira de 14 NeoPixel.
- Seleccionar una temperatura objetivo con un boton.
- Activar o desactivar la fuente mediante el pulsador KCD4.
- Informar por el puerto serie lo que esta ocurriendo.

El programa esta escrito para Arduino usando PlatformIO.

## 2. Librerias

Las librerias se declaran en `platformio.ini`:

- `Adafruit NeoPixel`: controla los LEDs WS2812/NeoPixel.
- `Adafruit SSD1306`: controla el display OLED.
- `Adafruit GFX Library`: proporciona las funciones graficas del OLED.
- `OneWire`: permite comunicarse con el DS18B20.
- `DallasTemperature`: facilita la lectura de temperatura del DS18B20.
- `Wire`: implementa la comunicacion I2C del OLED.

## 3. Conexion de pines

| Dispositivo | Pin del ESP32 | Funcion |
|---|---:|---|
| NeoPixel DIN | GPIO25 | Datos de la tira LED |
| DS18B20 DATA | GPIO4 | Datos del sensor |
| Boton de temperatura | GPIO27 | Selecciona el objetivo |
| Pulsador KCD4 | GPIO26 | Enciende o apaga la fuente |
| Rele HJR-3FF | GPIO33 | Senal de activacion del rele |
| OLED SDA | GPIO21 | Bus I2C de datos |
| OLED SCL | GPIO22 | Bus I2C de reloj |

Los pines se pueden cambiar al principio de `src/main.cpp`.

### Conexion del DS18B20

El DS18B20 necesita una resistencia de 4.7 kOhm entre `DATA` y `3.3 V`:

```text
DS18B20 VCC  -> 3.3 V
DS18B20 GND  -> GND
DS18B20 DATA -> GPIO4
Resistencia de 4.7 kOhm entre DATA y 3.3 V
```

### Conexion de los botones

Los dos botones utilizan `INPUT_PULLUP`. Un terminal del boton va al GPIO y el otro va a GND. El boton se considera presionado cuando la entrada lee `LOW`.

### Control de la fuente mediante rele HJR-3FF

GPIO33 controla la entrada de un modulo adecuado para el rele HJR-3FF. El modulo debe tener transistor de accionamiento, diodo de proteccion y alimentacion apropiada para la bobina. No se debe conectar GPIO33 directamente a la bobina del rele, a la resistencia calefactora ni a una fuente de alta corriente.

El codigo considera que el rele se activa con nivel `HIGH`. Si el modulo trabaja con logica activa en `LOW`, se debe cambiar `RELE_ACTIVO` en `src/main.cpp` a `LOW`.

## 4. Variables principales

- `temperaturaC`: temperatura actual en grados Celsius.
- `indiceTemperatura`: posicion del objetivo seleccionado.
- `fuenteEncendida`: indica si la fuente esta activa.
- `ultimaLectura`: controla el intervalo entre lecturas del sensor.
- `ultimaActualizacionOLED`: controla la frecuencia de refresco del OLED.

Los objetivos disponibles son:

```cpp
{50.0f, 80.0f, 100.0f}
```

El objetivo inicial es 80 grados Celsius.

## 5. Lectura del DS18B20

La funcion `leerTemperatura()` solicita una medicion al sensor y obtiene el resultado con:

```cpp
sensor.getTempCByIndex(0)
```

La funcion trabaja en Celsius. Se aceptan valores entre -55 y 125 grados Celsius, que corresponden al rango normal del DS18B20. Si el sensor esta desconectado o entrega un valor invalido:

- Se muestra `Sensor ERR` en el OLED.
- Se envia un error al monitor serie.
- Se apagan los NeoPixel.

La temperatura se actualiza cada segundo. El sensor se configura a 9 bits para que la conversion tarde aproximadamente 94 ms y no retrase el intervalo de lectura. Al iniciar, se realiza una primera lectura inmediatamente.

## 6. Colores de los NeoPixel

La funcion `colorTemperatura()` selecciona el color segun la temperatura:

| Temperatura | Color |
|---:|---|
| Menor de 50 C | Azul |
| 50 a menor de 80 C | Transicion azul a amarillo |
| 80 a menor de 100 C | Amarillo a rojo |
| 100 C o mas | Rojo |

La cantidad de LEDs encendidos tambien representa el nivel de temperatura. El calculo usa un rango de 0 a 120 C:

```text
nivel = temperatura / 120
```

Por ejemplo, una temperatura de 60 C enciende aproximadamente el 50 % de la tira.

Los LEDs se apagan cuando:

- La fuente esta apagada.
- El DS18B20 no entrega una lectura valida.

## 7. Funcion de los botones

### Boton de temperatura

Cada pulsacion valida en GPIO27 avanza al siguiente objetivo:

```text
50 C -> 80 C -> 100 C -> 50 C
```

El nuevo objetivo se muestra en el OLED y se informa por el puerto serie.

### Pulsador principal KCD4

Cada pulsacion valida en GPIO26 cambia el estado del rele HJR-3FF y, por lo tanto, de la fuente de la pava:

```text
OFF -> ON -> OFF
```

Cuando cambia el estado, el programa actualiza GPIO33, los NeoPixel y el OLED.

Los botones tienen una rutina antirrebote de 40 ms para evitar varias detecciones por una sola pulsacion.

## 8. Informacion del OLED

El OLED usa la direccion I2C `0x3C` y muestra:

- Nombre del sistema.
- Temperatura actual en Celsius.
- Temperatura objetivo.
- Nivel porcentual.
- Estado de la fuente: `ON` u `OFF`.

El display se actualiza cada segundo junto con la lectura de temperatura, evitando parpadeos por refrescos demasiado frecuentes. La pantalla usa abreviaturas de color (`AZUL`, `TRANS`, `AMAR` o `ROJO`) para que todos los datos entren en 128 x 64 pixeles.

## 9. Mensajes del puerto serie

La comunicacion serie funciona a 115200 baudios. Los mensajes utilizan prefijos para facilitar la lectura:

- `[CONFIG]`: pines y configuracion inicial.
- `[LOG TEMP]`: lecturas del DS18B20, objetivo, porcentaje, relé y errores.
- `[BOTON TEMP]`: cambio de temperatura objetivo.
- `[KCD4]`: cambio de estado de la fuente.
- `[FUENTE]`: estado inicial de la fuente.
- `[LEDS]`: cantidad de LEDs, color y nivel.
- `[OLED]`: estado del display.
- `[SISTEMA]`: estado general del programa.

Para abrir el monitor serie con PlatformIO:

```bash
pio device monitor --port COM3 --baud 115200
```

## 10. Flujo de ejecucion

### `setup()`

1. Inicia el puerto serie.
2. Configura botones y salida de la fuente.
3. Apaga la fuente y los NeoPixel.
4. Inicializa la tira NeoPixel.
5. Inicializa el OLED por I2C.
6. Inicializa el DS18B20.
7. Muestra la pantalla inicial.

### `loop()`

1. Revisa si se presiono el boton de temperatura.
2. Revisa si se presiono KCD4.
3. Lee el DS18B20 cada segundo.
4. Actualiza los LEDs segun la temperatura.
5. Actualiza el OLED cada segundo.
6. Espera 5 ms para mantener el ciclo estable.

## 11. Archivos relacionados

- `src/main.cpp`: firmware principal de Arduino.
- `platformio.ini`: placa, framework y librerias.
- `README.md`: informacion general y conexiones del proyecto.

## 12. Compilacion y carga

Desde la carpeta del proyecto:

```bash
pio run
pio run --target upload --upload-port COM3
```

Despues de cargar el firmware, abre el monitor serie a 115200 baudios para comprobar que el OLED, el DS18B20, los botones y los NeoPixel se inicialicen correctamente.
