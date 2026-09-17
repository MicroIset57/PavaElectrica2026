#include <Arduino.h>
#include <Wire.h>              // el OLED usa I2C
#include <U8x8lib.h>           // "
#include <OneWire.h>           // interface del sensor DS18B20
#include <DallasTemperature.h> // "
#include <FastLED.h>           // los LEDS RGB Neopixel

// Configuración Neopixel
#define PIN_NEOPIXEL 25
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS 14
#define BRIGHTNESS 255

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
const unsigned long REBOTE_MS = 40;

CRGB leds[NUM_LEDS];
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensor(&oneWire);

float temperaturaC = DEVICE_DISCONNECTED_C;
uint8_t indiceTemperatura = 1;
bool pavaEncendida = false;
bool oledDisponible = false;
uint8_t direccionOLED = 0;
bool conversionEnCurso = false;
unsigned long inicioConversion = 0;
uint16_t esperaConversionMs = 750; // lo ajusta setup() segun la resolucion

// struct Boton
// {
//   uint8_t pin;
//   bool estadoEstable;
//   unsigned long cambio;
// };

// Boton botonTemperatura = {PIN_BOTON_TEMPERATURA, HIGH, HIGH, 0};
// Boton botonFuente = {PIN_KCD4, HIGH, HIGH, 0};

uint8_t detectarDireccionOLED()
{
  const uint8_t direcciones[] = {0x3C, 0x3D};
  for (uint8_t direccion : direcciones)
  {
    Wire.beginTransmission(direccion);
    if (Wire.endTransmission() == 0)
    {
      return direccion;
    }
  }
  return 0;
}

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
void actualizarCuadroLeds()
{

  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88(87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16; // gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis;
  sLastMillis = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;

    uint16_t bri16 = (uint32_t)((uint32_t)b16 * (uint32_t)b16) / 65536;
    uint8_t bri8 = (uint32_t)(((uint32_t)bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV(hue8, sat8, bri8);

    uint16_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend(leds[pixelnumber], newcolor, 64);
  }
  FastLED.show();
}

void actualizarOLED()
{
  // actualizar el display
}

// Etapa 1: dispara la conversion y vuelve enseguida (no espera los ~94 ms).
void iniciarLecturaTemperatura()
{
  sensor.requestTemperatures();
  inicioConversion = millis();
  conversionEnCurso = true;
}

// Etapa 2: cuando el sensor ya termino, se lee el scratchpad (~6 ms).
// 9 bits -> 93.75 ms de conversion en el DS18B20 (margen: 110 ms)
void completarLecturaTemperatura()
{
  // conversionEnCurso solo dice "la pedi"; el sensor no avisa cuando termina,
  // asi que hay que darle el tiempo que garantiza el datasheet.
  if (!conversionEnCurso || (millis() - inicioConversion) < esperaConversionMs)
    return;
  conversionEnCurso = false;

  float nuevaTemperatura = sensor.getTempCByIndex(0);
  if (nuevaTemperatura != DEVICE_DISCONNECTED_C && nuevaTemperatura > -55.0f &&
      nuevaTemperatura < 125.0f)
  {
    temperaturaC = nuevaTemperatura;
    float porcentaje = constrain(temperaturaC, 0.0f, 120.0f) / 120.0f * 100.0f;
    Serial.printf("[LOG TEMP] %.1f grados | objetivo %.0f grados | nivel %.0f%% | rele %s\n",
                  temperaturaC, TEMPERATURAS_SELECCIONABLES[indiceTemperatura],
                  porcentaje, pavaEncendida ? "ON" : "OFF");
  }
  else
  {
    temperaturaC = DEVICE_DISCONNECTED_C;
    Serial.println("[LOG TEMP] ERROR: DS18B20 desconectado o lectura invalida");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(1000);
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

  //--display OLED--
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  u8x8.begin();
  u8x8.setFont(u8x8_font_px437wyse700a_2x2_r);
  u8x8.drawString(0, 0, "PAVA ELECTRICA");
  u8x8.setFont(u8x8_font_px437wyse700b_2x2_r);
  u8x8.drawString(0, 2, "ALIENIGENA");

  //--leds NeoPixel--
  FastLED.addLeds<LED_TYPE, PIN_NEOPIXEL, COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip)
      .setDither(BRIGHTNESS < 255);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  //--sensor de temperatura DS18B20--
  sensor.begin();
  sensor.setResolution(9);
  esperaConversionMs = sensor.millisToWaitForConversion(); // 9 bits -> 94 ms
  sensor.setWaitForConversion(false); // lectura no bloqueante: no frena los LEDs
  Serial.printf("[TEMP] Sensores encontrados: %d\n", sensor.getDeviceCount());
  Serial.println("[TEMP] Resolucion: 9 bits | lectura cada 1 segundo");
  Serial.printf("[TEMP] Objetivo inicial: %.0f C\n",
                TEMPERATURAS_SELECCIONABLES[indiceTemperatura]);
  iniciarLecturaTemperatura();
  actualizarOLED();
  Serial.println("[SISTEMA] Control de temperatura iniciado");
}

void loop()
{
  // if (botonPresionado(botonTemperatura))
  // {
  //   indiceTemperatura = (indiceTemperatura + 1) % NUM_TEMPERATURAS;
  //   Serial.printf("[BOTON TEMP] Nuevo objetivo: %.0f C\n",
  //                 TEMPERATURAS_SELECCIONABLES[indiceTemperatura]);
  //   // actualizarOLED();
  // }

  // if (botonPresionado(botonFuente))
  // {
  //   pavaEncendida = !pavaEncendida;
  //   digitalWrite(PIN_RELE_HJR3FF,
  //                pavaEncendida ? RELE_ACTIVO : !RELE_ACTIVO);
  //   Serial.printf("[KCD4] Pulsacion detectada -> rele HJR-3FF %s, pava %s\n",
  //                 pavaEncendida ? "ON" : "OFF",
  //                 pavaEncendida ? "ON" : "OFF");

  //   // actualizarOLED();
  // }

  if (!conversionEnCurso)
  {
    EVERY_N_MILLIS(1000)
    {
      iniciarLecturaTemperatura();
    }
  }
  completarLecturaTemperatura();

  EVERY_N_MILLIS(1000)
  {
    actualizarOLED();
  }

  EVERY_N_MILLIS(30)
  {
    actualizarCuadroLeds();
  }
}