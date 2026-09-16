#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>

// Configuración Neopixel
#define PIN_NEOPIXEL 25
#define NUM_LEDS 14
#define LED_TYPE NEO_GRB + NEO_KHZ800

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pines configurables
#define PIN_DS18B20 4
#define PIN_BOTON_TEMPERATURA 27
#define PIN_KCD4 26
#define PIN_RELE_HJR3FF 33
#define RELE_ACTIVO HIGH
#define PIN_OLED_SDA 21
#define PIN_OLED_SCL 22

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1

const float TEMPERATURAS_SELECCIONABLES[] = {50.0f, 80.0f, 100.0f};
const uint8_t NUM_TEMPERATURAS = sizeof(TEMPERATURAS_SELECCIONABLES) /
                                 sizeof(TEMPERATURAS_SELECCIONABLES[0]);
const unsigned long INTERVALO_LECTURA_MS = 1000;
const unsigned long INTERVALO_OLED_MS = 1000;
const unsigned long REBOTE_MS = 40;

Adafruit_NeoPixel strip(NUM_LEDS, PIN_NEOPIXEL, LED_TYPE);
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensor(&oneWire);

float temperaturaC = DEVICE_DISCONNECTED_C;
uint8_t indiceTemperatura = 1;
bool fuenteEncendida = false;
bool oledDisponible = false;
uint8_t direccionOLED = 0;
unsigned long ultimaLectura = 0;
unsigned long ultimaActualizacionOLED = 0;

struct Boton {
  uint8_t pin;
  bool estadoEstable;
  bool ultimaLectura;
  unsigned long cambio;
};

Boton botonTemperatura = {PIN_BOTON_TEMPERATURA, HIGH, HIGH, 0};
Boton botonFuente = {PIN_KCD4, HIGH, HIGH, 0};

uint8_t detectarDireccionOLED() {
  const uint8_t direcciones[] = {0x3C, 0x3D};
  for (uint8_t direccion : direcciones) {
    Wire.beginTransmission(direccion);
    if (Wire.endTransmission() == 0) {
      return direccion;
    }
  }
  return 0;
}

bool botonPresionado(Boton &boton) {
  bool lectura = digitalRead(boton.pin);
  if (lectura != boton.ultimaLectura) {
    boton.cambio = millis();
    boton.ultimaLectura = lectura;
  }

  if ((millis() - boton.cambio) > REBOTE_MS && lectura != boton.estadoEstable) {
    boton.estadoEstable = lectura;
    return lectura == LOW;
  }
  return false;
}

uint32_t colorTemperatura(float temperatura) {
  if (temperatura < 68.0f) {
    return strip.Color(0, 0, 255);
  }
  if (temperatura < 140.0f) {
    uint8_t paso = (uint8_t)((temperatura - 68.0f) * 255.0f / 72.0f);
    return strip.Color(0, paso, 255);
  }
  if (temperatura < 176.0f) {
    uint8_t paso = (uint8_t)((temperatura - 140.0f) * 255.0f / 36.0f);
    return strip.Color(paso, 255, 255 - paso);
  }
  if (temperatura < 194.0f) {
    uint8_t paso = (uint8_t)((temperatura - 176.0f) * 255.0f / 18.0f);
    return strip.Color(255, 255 - paso, 0);
  }
  return strip.Color(255, 0, 0);
}

bool temperaturaValida() {
  return temperaturaC != DEVICE_DISCONNECTED_C;
}

float temperaturaFahrenheit() {
  return temperaturaC * 9.0f / 5.0f + 32.0f;
}

uint8_t nivelTemperatura() {
  if (!temperaturaValida()) {
    return 0;
  }
  return (uint8_t)(constrain(temperaturaFahrenheit(), 32.0f, 212.0f) /
                   212.0f * 100.0f);
}

const char *nombreColorTemperatura(float temperatura) {
  if (temperatura < 68.0f) {
    return "AZUL";
  }
  if (temperatura < 140.0f) {
    return "CIAN";
  }
  if (temperatura < 176.0f) {
    return "VERDE";
  }
  if (temperatura < 194.0f) {
    return "AMARILLO";
  }
  return "ROJO";
}

const char *nombreColorOLED(float temperatura) {
  if (temperatura < 30.0f) {
    return "AZUL";
  }
  if (temperatura < 60.0f) {
    return "CIAN";
  }
  if (temperatura < 90.0f) {
    return "VERDE";
  }
  if (temperatura < 110.0f) {
    return "AMAR";
  }
  return "ROJO";
}

void actualizarLuces() {
  uint8_t ledsEncendidos = temperaturaValida()
                               ? (uint8_t)ceil(nivelTemperatura() / 100.0f * NUM_LEDS)
                               : NUM_LEDS;
  uint32_t color = temperaturaValida() ? colorTemperatura(temperaturaFahrenheit())
                                       : strip.Color(255, 0, 0);
  uint32_t colorBase = temperaturaValida() ? strip.Color(3, 3, 3) : color;
  for (uint8_t led = 0; led < NUM_LEDS; led++) {
    strip.setPixelColor(led, led < ledsEncendidos ? color : colorBase);
  }
  strip.show();
  Serial.printf("[LEDS] %u/%u resaltados, color %s, nivel %u%%\n",
                ledsEncendidos, NUM_LEDS,
                temperaturaValida() ? nombreColorTemperatura(temperaturaFahrenheit())
                                     : "ERROR",
                nivelTemperatura());
}

void actualizarOLED() {
  if (!oledDisponible) {
    return;
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.print("PAVA ELECTRICA");
  display.setCursor(91, 0);
  display.print(fuenteEncendida ? "ON" : "OFF");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  display.setCursor(0, 18);
  display.setTextSize(2);
  if (!temperaturaValida()) {
    display.print("ERROR TEMP");
  } else {
    display.print(temperaturaFahrenheit(), 1);
    display.println(" F");
  }

  display.setTextSize(1);
  display.setCursor(0, 39);
  display.print("OBJETIVO ");
  display.print(TEMPERATURAS_SELECCIONABLES[indiceTemperatura] * 9.0f / 5.0f + 32.0f,
               0);
  display.print(" F   ");
  display.print(temperaturaValida() ? nombreColorOLED(temperaturaFahrenheit())
                                    : "ERROR");
  display.drawRect(0, 51, 128, 8, SSD1306_WHITE);
  if (temperaturaValida()) {
    display.fillRect(2, 53, (uint8_t)(nivelTemperatura() * 124 / 100), 4,
                     SSD1306_WHITE);
  }
  display.display();
}

void leerTemperatura() {
  sensor.requestTemperatures();
  float nuevaTemperatura = sensor.getTempCByIndex(0);
  if (nuevaTemperatura != DEVICE_DISCONNECTED_C && nuevaTemperatura > -55.0f &&
      nuevaTemperatura < 125.0f) {
    temperaturaC = nuevaTemperatura;
        Serial.printf("[LOG TEMP] %.1f Fahrenheit | objetivo %.0f Fahrenheit | nivel %u%% | rele %s | color %s\n",
          temperaturaFahrenheit(),
          TEMPERATURAS_SELECCIONABLES[indiceTemperatura] * 9.0f / 5.0f + 32.0f,
          nivelTemperatura(), fuenteEncendida ? "ON" : "OFF",
          nombreColorTemperatura(temperaturaFahrenheit()));
  } else {
    temperaturaC = DEVICE_DISCONNECTED_C;
    Serial.println("[LOG TEMP] ERROR: DS18B20 desconectado o lectura invalida");
  }
  actualizarLuces();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("========================================");
  Serial.println("   PAVA ELECTRICA - INICIO DEL SISTEMA");
  Serial.println("========================================");
  Serial.println("[LOGS] Lectura del DS18B20 cada 1 segundo");
  Serial.println("[LOGS] Monitor serie: 115200 baudios");
  Serial.printf("[CONFIG] NeoPixel: GPIO%d, %u LEDs\n", PIN_NEOPIXEL, NUM_LEDS);
  Serial.printf("[CONFIG] DS18B20: GPIO%d\n", PIN_DS18B20);
  Serial.printf("[CONFIG] Boton temperatura: GPIO%d\n", PIN_BOTON_TEMPERATURA);
  Serial.printf("[CONFIG] KCD4: GPIO%d | rele HJR-3FF: GPIO%d\n", PIN_KCD4,
                PIN_RELE_HJR3FF);
  Serial.printf("[CONFIG] OLED I2C: SDA GPIO%d, SCL GPIO%d\n",
                PIN_OLED_SDA, PIN_OLED_SCL);
  pinMode(PIN_BOTON_TEMPERATURA, INPUT_PULLUP);
  pinMode(PIN_KCD4, INPUT_PULLUP);
  pinMode(PIN_RELE_HJR3FF, OUTPUT);
  digitalWrite(PIN_RELE_HJR3FF, !RELE_ACTIVO);
  Serial.println("[RELE] HJR-3FF OFF al arrancar");

  strip.begin();
  strip.setBrightness(100);
  strip.clear();
  strip.show();
  Serial.println("[LEDS] Tira NeoPixel lista, brillo 100/255");

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  Wire.setClock(100000);
  Serial.println("[OLED] I2C configurado a 100 kHz");
  direccionOLED = detectarDireccionOLED();
  if (direccionOLED == 0) {
    Serial.println("[OLED] ERROR: no hay respuesta en 0x3C ni 0x3D");
  } else {
    Serial.printf("[OLED] Dispositivo detectado en 0x%02X\n", direccionOLED);
    oledDisponible = display.begin(SSD1306_SWITCHCAPVCC, direccionOLED);
    if (!oledDisponible) {
      Serial.println("[OLED] ERROR: fallo al inicializar SSD1306");
    } else {
      Serial.println("[OLED] Inicializado correctamente");
    }
  }
  sensor.begin();
  sensor.setResolution(9);
  sensor.setWaitForConversion(true);
  Serial.printf("[TEMP] Sensores encontrados: %d\n", sensor.getDeviceCount());
  Serial.println("[TEMP] Resolucion: 9 bits | lectura cada 1 segundo");
  Serial.printf("[TEMP] Objetivo inicial: %.0f F\n",
                TEMPERATURAS_SELECCIONABLES[indiceTemperatura] * 9.0f / 5.0f + 32.0f);
  leerTemperatura();
  ultimaLectura = millis();
  actualizarOLED();
  Serial.println("[SISTEMA] Control de temperatura iniciado");
}

void loop() {
  if (botonPresionado(botonTemperatura)) {
    indiceTemperatura = (indiceTemperatura + 1) % NUM_TEMPERATURAS;
    Serial.printf("[BOTON TEMP] Nuevo objetivo: %.0f F\n",
            TEMPERATURAS_SELECCIONABLES[indiceTemperatura] * 9.0f / 5.0f + 32.0f);
    actualizarOLED();
  }

  if (botonPresionado(botonFuente)) {
    fuenteEncendida = !fuenteEncendida;
    digitalWrite(PIN_RELE_HJR3FF,
           fuenteEncendida ? RELE_ACTIVO : !RELE_ACTIVO);
    Serial.printf("[KCD4] Pulsacion detectada -> rele HJR-3FF %s, pava %s\n",
            fuenteEncendida ? "ON" : "OFF",
            fuenteEncendida ? "ON" : "OFF");
    actualizarLuces();
    actualizarOLED();
  }

  unsigned long ahora = millis();
  if (ahora - ultimaLectura >= INTERVALO_LECTURA_MS) {
    ultimaLectura = ahora;
    leerTemperatura();
  }
  if (ahora - ultimaActualizacionOLED >= INTERVALO_OLED_MS) {
    ultimaActualizacionOLED = ahora;
    actualizarOLED();
  }
  delay(5);
}