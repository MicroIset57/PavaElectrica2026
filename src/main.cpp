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
#define PIN_DS18B20 4    // sensor de temperatura
#define PIN_BOTON 27     // mueve las temperaturas del menu en el display
#define PIN_RELE 33      // enciende la pava y la apaga al alcanzar la temperatura
#define PIN_BUZZER 23    // buzzer
#define PIN_OLED_SDA 21  // I2C para display
#define PIN_OLED_SCL 22  // "
#define RELE_ACTIVO HIGH // se activa el transistor NPN con un 1.

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1

const float TEMPERATURAS_SELECCIONABLES[] = {60.0f, 70.0f, 80.0f, 90.0f, 100.0f};
const uint8_t NUM_TEMPERATURAS = sizeof(TEMPERATURAS_SELECCIONABLES) /
                                 sizeof(TEMPERATURAS_SELECCIONABLES[0]);

const unsigned long DURACION_BIENVENIDA_MS = 3000; // cuanto se ve "ISET 57"
const unsigned long REBOTE_MS = 40;                // antirrebote del pulsador

// --- tiempos del proceso ---
const unsigned long ESPERA_INICIO_MS = 4000;                // sin tocar el boton -> arranca
const unsigned long AVISO_LISTO_MS = 5000;                  // buzzer al alcanzar el objetivo
const unsigned long MANTENIMIENTO_MS = 5UL * 60UL * 1000UL; // 5 minutos
const unsigned long PERIODO_AVISO_MANT_MS = 15000;          // cada cuanto los 4 beeps
const unsigned long FALLA_SENSOR_MS = 10000;                // sin lectura valida -> aborta
const float HISTERESIS_C = 2.0f;                            // si baja 2 grados, recalienta

// --- patron de los beeps cortos ---
const unsigned long BEEP_ON_MS = 80;
const unsigned long BEEP_OFF_MS = 120;
const uint16_t BEEPS_MANTENIMIENTO = 4;
const uint16_t BEEPS_FINAL = 30;

#define BUZZER_ACTIVO HIGH // buzzer activo (con oscilador propio) a nivel alto

CRGB leds[NUM_LEDS];
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);
OneWire oneWire(PIN_DS18B20);
DallasTemperature sensor(&oneWire);

float temperaturaC = DEVICE_DISCONNECTED_C;
uint8_t indiceTemperatura = 1;
bool pavaEncendida = false;
bool conversionEnCurso = false;
unsigned long finBienvenida = 0;
unsigned long inicioConversion = 0;
unsigned long ultimaLecturaValida = 0;
uint16_t esperaConversionMs = 750; // lo ajusta setup() segun la resolucion

// Etapas del proceso. El pulsador siempre devuelve a SELECCION, asi que desde
// cualquier punto se puede volver a elegir la temperatura.
enum EstadoProceso
{
  EST_REPOSO,      // apagado, esperando la primera pulsacion
  EST_SELECCION,   // eligiendo objetivo; arranca 4 s despues de soltar
  EST_CALENTANDO,  // rele ON hasta alcanzar el objetivo
  EST_AVISANDO,    // buzzer 5 s seguidos, rele ya apagado
  EST_MANTENIENDO, // histeresis durante 5 minutos
  EST_FINALIZANDO, // 30 beeps cortos y a dormir
  EST_FALLA        // el sensor dejo de responder: todo apagado
};

EstadoProceso estado = EST_REPOSO;
unsigned long ultimaPulsacion = 0;
unsigned long inicioMantenimiento = 0;
unsigned long proximoAvisoMant = 0;

float objetivoC()
{
  return TEMPERATURAS_SELECCIONABLES[indiceTemperatura];
}

// ----------------------------------------------------------------- BOTON ---
// Antirrebote por tiempo: la lectura tiene que quedarse quieta REBOTE_MS antes
// de darla por buena. Guarda el flanco de bajada para que no se pierda ninguna
// pulsacion entre dos vueltas del loop().
struct Boton
{
  uint8_t pin;
  bool estable; // HIGH = suelto (el pin tiene pull-up interno)
  bool ultimaLectura;
  unsigned long cambio;
};

Boton boton = {PIN_BOTON, HIGH, HIGH, 0};
bool flancoPulsacion = false;

void actualizarBoton()
{
  bool lectura = digitalRead(boton.pin);
  if (lectura != boton.ultimaLectura)
  {
    boton.cambio = millis();
    boton.ultimaLectura = lectura;
  }
  if ((millis() - boton.cambio) > REBOTE_MS && lectura != boton.estable)
  {
    boton.estable = lectura;
    if (boton.estable == LOW)
    {
      flancoPulsacion = true;
    }
  }
}

// Devuelve true una sola vez por pulsacion (consume el flanco).
bool huboPulsacion()
{
  bool hubo = flancoPulsacion;
  flancoPulsacion = false;
  return hubo;
}

bool botonApretado()
{
  return boton.estable == LOW;
}

// ---------------------------------------------------------------- BUZZER ---
// Secuenciador sin bloqueo: se le programa "N beeps de X ms con Y ms de
// silencio" y el loop() se encarga. Nunca hay delay() de por medio, asi los
// LEDs y el sensor siguen su ritmo mientras suena.
uint16_t beepsPendientes = 0;
unsigned long beepMsOn = 0;
unsigned long beepMsOff = 0;
unsigned long beepProximoCambio = 0;
bool beepEnAlto = false;
bool buzzerSostenido = false; // lo fuerza el dedo sobre el pulsador

void programarBeeps(uint16_t cantidad, unsigned long msOn, unsigned long msOff)
{
  beepsPendientes = cantidad;
  beepMsOn = msOn;
  beepMsOff = msOff;
  beepEnAlto = false;
  beepProximoCambio = millis(); // que empiece en la proxima vuelta
}

void cancelarBeeps()
{
  beepsPendientes = 0;
  beepEnAlto = false;
}

bool beepsEnCurso()
{
  return beepsPendientes > 0 || beepEnAlto;
}

void actualizarBuzzer()
{
  unsigned long ahora = millis();

  if ((long)(ahora - beepProximoCambio) >= 0)
  {
    if (beepEnAlto)
    {
      beepEnAlto = false;
      beepProximoCambio = ahora + beepMsOff;
    }
    else if (beepsPendientes > 0)
    {
      beepsPendientes--;
      beepEnAlto = true;
      beepProximoCambio = ahora + beepMsOn;
    }
  }

  // El pin se escribe solo cuando cambia, no en cada vuelta del loop.
  bool debeSonar = beepEnAlto || buzzerSostenido;
  static bool sonando = false;
  if (debeSonar != sonando)
  {
    sonando = debeSonar;
    digitalWrite(PIN_BUZZER, debeSonar ? BUZZER_ACTIVO : !BUZZER_ACTIVO);
  }
}

// ------------------------------------------------------------------ RELE ---
void ponerRele(bool encendido)
{
  if (pavaEncendida == encendido)
  {
    return;
  }
  pavaEncendida = encendido;
  digitalWrite(PIN_RELE, encendido ? RELE_ACTIVO : !RELE_ACTIVO);
  Serial.printf("[RELE] %s\n", encendido ? "ON" : "OFF");
}

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
// ejemplo de LGTB
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

// Escribe una linea completa de 16 caracteres en la fila indicada, y solo si
// el texto cambio: cada linea cuesta ~6 ms de I2C y no hay que gastarlos al
// pedo. El relleno con espacios borra lo que hubiera antes (u8x8 no tiene
// buffer, asi se evita el parpadeo de un clear() general).
void dibujarLineaOLED(uint8_t fila, uint8_t ranura, const char *texto)
{
  static char cache[4][17] = {{0}, {0}, {0}, {0}};
  char linea[17];
  snprintf(linea, sizeof(linea), "%-16s", texto);
  if (strcmp(linea, cache[ranura]) == 0)
  {
    return;
  }
  strcpy(cache[ranura], linea);
  u8x8.drawString(0, fila, linea);
}

// Cuarta linea del display: en que anda el equipo. Entra en 16 caracteres.
void textoEstado(char *destino, size_t largo)
{
  unsigned long ahora = millis();
  unsigned long restan;

  switch (estado)
  {
  case EST_REPOSO:
    snprintf(destino, largo, "Pulse p/elegir");
    break;

  case EST_SELECCION:
    // Cuenta regresiva hasta que arranca solo.
    restan = ahora - ultimaPulsacion;
    restan = (restan >= ESPERA_INICIO_MS) ? 0
                                          : (ESPERA_INICIO_MS - restan + 999) / 1000;
    snprintf(destino, largo, "Inicia en %lus", restan);
    break;

  case EST_CALENTANDO:
    snprintf(destino, largo, "Calentando...");
    break;

  case EST_AVISANDO:
    snprintf(destino, largo, "LISTO!");
    break;

  case EST_MANTENIENDO:
    restan = ahora - inicioMantenimiento;
    restan = (restan >= MANTENIMIENTO_MS) ? 0 : (MANTENIMIENTO_MS - restan) / 1000;
    snprintf(destino, largo, "Mantiene %lu:%02lu", restan / 60, restan % 60);
    break;

  case EST_FINALIZANDO:
    snprintf(destino, largo, "Terminando...");
    break;

  case EST_FALLA:
    snprintf(destino, largo, "FALLA SENSOR");
    break;
  }
}

void actualizarOLED()
{
  static bool pantallaLista = false;
  char texto[24];

  if (!pantallaLista)
  {
    u8x8.clearDisplay(); // borra el cartel de bienvenida
    u8x8.setFont(u8x8_font_7x14B_1x2_f);
    pantallaLista = true;
  }

  // Ojo con el simbolo de grado: va en octal ("\260" = 176). Escrito como
  // "\xB0C" el compilador se comeria la 'C' como parte del numero hexadecimal.
  if (temperaturaC == DEVICE_DISCONNECTED_C)
  {
    snprintf(texto, sizeof(texto), "Agua --.-\260C");
  }
  else
  {
    snprintf(texto, sizeof(texto), "Agua %4.1f\260C", temperaturaC);
  }
  dibujarLineaOLED(1, 0, texto);

  if (estado != EST_REPOSO)
  {
    snprintf(texto, sizeof(texto), "Llevar a %4.0f\260C", objetivoC());
    dibujarLineaOLED(2, 1, texto);
  }

  // snprintf(texto, sizeof(texto), "%s", pavaEncendida ? "ENCENDIDO" : "APAGADO");
  // dibujarLineaOLED(4, 2, texto);

  textoEstado(texto, sizeof(texto));
  dibujarLineaOLED(6, 3, texto);
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
    ultimaLecturaValida = millis();
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

// ------------------------------------------------------ MAQUINA DE ESTADOS ---
const char *nombreEstado()
{
  switch (estado)
  {
  case EST_REPOSO:
    return "REPOSO";
  case EST_SELECCION:
    return "SELECCION";
  case EST_CALENTANDO:
    return "CALENTANDO";
  case EST_AVISANDO:
    return "AVISANDO";
  case EST_MANTENIENDO:
    return "MANTENIENDO";
  case EST_FINALIZANDO:
    return "FINALIZANDO";
  case EST_FALLA:
    return "FALLA";
  }
  return "?";
}

// Todo lo que hay que hacer "al entrar" a cada etapa vive aca, en un solo
// lugar: asi ninguna transicion se olvida de apagar el rele o de cortar un
// beep que venia sonando.
void cambiarEstado(EstadoProceso nuevo)
{
  estado = nuevo;

  switch (estado)
  {
  case EST_REPOSO:
    ponerRele(false);
    break;

  case EST_SELECCION:
    ponerRele(false); // mientras se elige, la pava no calienta
    cancelarBeeps();
    ultimaPulsacion = millis();
    break;

  case EST_CALENTANDO:
    ponerRele(true);
    break;

  case EST_AVISANDO:
    ponerRele(false);
    programarBeeps(1, AVISO_LISTO_MS, 0); // un unico "beep" de 5 segundos
    break;

  case EST_MANTENIENDO:
    inicioMantenimiento = millis();
    proximoAvisoMant = millis(); // avisa apenas entra
    break;

  case EST_FINALIZANDO:
    ponerRele(false);
    programarBeeps(BEEPS_FINAL, BEEP_ON_MS, BEEP_OFF_MS);
    break;

  case EST_FALLA:
    ponerRele(false);
    programarBeeps(3, 600, 300);
    break;
  }

  Serial.printf("[ESTADO] %s | objetivo %.0f C\n", nombreEstado(), objetivoC());
}

void procesarEstado()
{
  unsigned long ahora = millis();

  // El buzzer acompania mientras el dedo siga apoyado, en cualquier etapa.
  buzzerSostenido = botonApretado();

  // El pulsador manda siempre: corta lo que este haciendo y vuelve a elegir.
  if (huboPulsacion())
  {
    if (estado != EST_SELECCION)
    {
      cambiarEstado(EST_SELECCION);
    }
    indiceTemperatura = (indiceTemperatura + 1) % NUM_TEMPERATURAS;
    ultimaPulsacion = ahora;
    Serial.printf("[BOTON] nuevo objetivo: %.0f C\n", objetivoC());
  }

  // Si el sensor se cayo no hay forma de saber cuando cortar: se apaga todo.
  // Sin esto, un cable flojo dejaria el rele pegado indefinidamente.
  bool sensorVivo = (temperaturaC != DEVICE_DISCONNECTED_C) &&
                    (ahora - ultimaLecturaValida) < FALLA_SENSOR_MS;
  if (!sensorVivo && (estado == EST_CALENTANDO || estado == EST_MANTENIENDO))
  {
    Serial.println("[FALLA] El DS18B20 dejo de responder: proceso abortado");
    cambiarEstado(EST_FALLA);
    return;
  }

  switch (estado)
  {
  case EST_REPOSO:
  case EST_FALLA:
    break; // solo se sale de aca pulsando el boton

  case EST_SELECCION:
    // Arranca 4 s despues de la ultima pulsacion, y con el boton suelto.
    if (!botonApretado() && (ahora - ultimaPulsacion) >= ESPERA_INICIO_MS)
    {
      cambiarEstado(EST_CALENTANDO);
    }
    break;

  case EST_CALENTANDO:
    if (temperaturaC >= objetivoC())
    {
      cambiarEstado(EST_AVISANDO);
    }
    break;

  case EST_AVISANDO:
    if (!beepsEnCurso())
    {
      cambiarEstado(EST_MANTENIENDO);
    }
    break;

  case EST_MANTENIENDO:
    if ((ahora - inicioMantenimiento) >= MANTENIMIENTO_MS)
    {
      cambiarEstado(EST_FINALIZANDO);
      break;
    }
    // Histeresis: recien vuelve a calentar cuando bajo HISTERESIS_C grados,
    // para no estar golpeando el rele cada decima de grado.
    if (pavaEncendida && temperaturaC >= objetivoC())
    {
      ponerRele(false);
    }
    else if (!pavaEncendida && temperaturaC <= (objetivoC() - HISTERESIS_C))
    {
      ponerRele(true);
    }
    // Y cada tanto, los 4 beeps que avisan que sigue manteniendo.
    if (!beepsEnCurso() && (long)(ahora - proximoAvisoMant) >= 0)
    {
      programarBeeps(BEEPS_MANTENIMIENTO, BEEP_ON_MS, BEEP_OFF_MS);
      proximoAvisoMant = ahora + PERIODO_AVISO_MANT_MS;
    }
    break;

  case EST_FINALIZANDO:
    if (!beepsEnCurso())
    {
      Serial.println("[SISTEMA] Proceso terminado, todo apagado");
      cambiarEstado(EST_REPOSO);
    }
    break;
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
  Serial.printf("[CONFIG] Boton temperatura: GPIO%d\n", PIN_BOTON);
  Serial.printf("[CONFIG] RELE: GPIO%d\n", PIN_RELE);
  Serial.printf("[CONFIG] OLED I2C: SDA GPIO%d, SCL GPIO%d\n", PIN_OLED_SDA, PIN_OLED_SCL);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  pinMode(PIN_RELE, OUTPUT);
  digitalWrite(PIN_RELE, !RELE_ACTIVO);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, !BUZZER_ACTIVO);
  Serial.println("[RELE] OFF al arrancar");

  //--display OLED--
  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  u8x8.setBusClock(400000); // antes de begin(): I2C rapido = menos bloqueo
  u8x8.begin();
  u8x8.setFont(u8x8_font_px437wyse700a_2x2_r);
  u8x8.drawString(1, 0, "ISET 57");
  u8x8.drawString(3, 3, "2026");
  finBienvenida = millis() + DURACION_BIENVENIDA_MS;
  // dejo preparada una font para siempre...
  u8x8.setFont(u8x8_font_7x14B_1x2_f);

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
  sensor.setWaitForConversion(false);                      // lectura no bloqueante: no frena los LEDs
  Serial.printf("[TEMP] Sensores encontrados: %d\n", sensor.getDeviceCount());
  Serial.println("[TEMP] Resolucion: 9 bits | lectura cada 1 segundo");
  Serial.printf("[TEMP] Objetivo inicial: %.0f C\n",
                TEMPERATURAS_SELECCIONABLES[indiceTemperatura]);
  iniciarLecturaTemperatura();
  // Aca NO se llama a actualizarOLED(): borraria el cartel de bienvenida al
  // instante. El primer refresco lo hace el loop() cuando vence finBienvenida.
  Serial.println("[SISTEMA] Control de temperatura iniciado");
}

void loop()
{
  // El antirrebote necesita mirar el pin en cada vuelta, no cada tanto.
  actualizarBoton();

  if (!conversionEnCurso)
  {
    EVERY_N_MILLIS(1000)
    {
      iniciarLecturaTemperatura();
    }
  }
  completarLecturaTemperatura();

  procesarEstado();
  actualizarBuzzer();

  // Cada 200 ms para que el boton y la cuenta regresiva se sientan vivos.
  // No cuesta I2C de mas: dibujarLineaOLED() solo escribe lo que cambio.
  EVERY_N_MILLIS(200)
  {
    // Hasta que se cumpla la bienvenida no se toca el display, asi queda a la
    // vista el "ISET 57". La resta con signo tolera el desborde de millis().
    if ((long)(millis() - finBienvenida) >= 0)
    {
      actualizarOLED();
    }
  }

  EVERY_N_MILLIS(30)
  {
    actualizarCuadroLeds();
  }
}