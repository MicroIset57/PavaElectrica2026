#include <Arduino.h>
#include <Wire.h>              // el OLED usa I2C
#include <U8g2lib.h>           // "
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
const unsigned long HOLD_JUEGO_MS = 5000;          // boton mantenido desde el
                                                   // reposo -> abre el juego
const unsigned long HOLD_SILENCIO_MS = 800;        // a partir de aca el boton
                                                   // mantenido deja de pitar

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
// U8g2 en modo full buffer ("_F_"): el dibujo se arma en 1 KB de RAM y se
// manda entero con sendBuffer(). Cuesta mas I2C que u8x8, pero permite pintar
// pixel por pixel, que es lo que necesita el juego del dino.
// U8G2_R2 = pantalla rotada 180 grados (reemplaza al viejo setFlipMode(1)).
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R2, U8X8_PIN_NONE);
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
  EST_FALLA,       // el sensor dejo de responder: todo apagado
  EST_JUEGO        // modo aparte: el juego del dino, rele apagado
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
unsigned long inicioPulsacion = 0;   // cuando se apoyo el dedo
bool botonConsumido = false;         // la pulsacion ya se uso para otra cosa
                                     // (abrir o cerrar el juego): que no pite
bool pulsacionDesdeReposo = false;   // la pulsacion en curso empezo en reposo
uint8_t indiceTemperaturaPrevio = 0; // objetivo de antes de esa pulsacion

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
      inicioPulsacion = millis(); // desde aca se mide la pulsacion sostenida
    }
    else
    {
      // Dedo afuera: lo que valia para esta pulsacion se descarta.
      botonConsumido = false;
      pulsacionDesdeReposo = false;
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

// Cuanto hace que el dedo esta apoyado, 0 si esta suelto.
unsigned long msBotonSostenido()
{
  return botonApretado() ? (millis() - inicioPulsacion) : 0;
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

// Con u8g2 ya no hay grilla de celdas: se dibuja en pixeles, de 0 a 127 a lo
// ancho y de 0 a 63 a lo alto. La "y" de un texto es su linea de base (donde
// se apoyan las letras), no el techo.
//   FONT_GRANDE: 26 px de alto, solo digitos y los signos . - : (el numero)
//   FONT_CHICA:  7x14 px, la de los carteles; trae el grado en el 176
//   FONT_MINI:   5x7 px, la mas chica, para el tanteador del juego
#define FONT_GRANDE u8g2_font_logisoso26_tn
#define FONT_CHICA u8g2_font_7x14B_tf
#define FONT_MINI u8g2_font_5x7_tf

const uint8_t Y_NUMERO = 30;    // base del numero grande
const uint8_t Y_ETIQUETAS = 22; // base de "Agua" y "\260C", contra el numero
const uint8_t Y_OBJETIVO = 47;  // base de "Llevar a 100 grados"
const uint8_t Y_ESTADO = 63;    // base del pie

// Lo prende el juego al salir: la pantalla quedo dibujada con otra cosa, asi
// que hay que redibujarla aunque el texto sea el mismo que antes de entrar.
bool oledForzarRedibujo = true;

// Escribe centrado a lo ancho, con la font que ya este elegida.
void dibujarCentrado(uint8_t y, const char *texto)
{
  int16_t x = ((int16_t)OLED_WIDTH - (int16_t)u8g2.getStrWidth(texto)) / 2;
  u8g2.drawStr(x < 0 ? 0 : x, y, texto);
}

// Banda superior: el numero en grande con "Agua" y el grado en chica a los
// costados, todo centrado como un bloque. Si no entrara (el peor caso es
// "100.0") se cae la etiqueta "Agua" antes que recortar el numero.
void dibujarBandaTemperatura(const char *numero)
{
  u8g2.setFont(FONT_GRANDE);
  int16_t anchoNumero = u8g2.getStrWidth(numero);
  u8g2.setFont(FONT_CHICA);
  int16_t anchoEtiqueta = u8g2.getStrWidth("Agua ");
  int16_t anchoGrado = u8g2.getStrWidth("\260C");

  bool conEtiqueta = (anchoEtiqueta + anchoNumero + anchoGrado) <= OLED_WIDTH;
  int16_t ancho = anchoNumero + anchoGrado + (conEtiqueta ? anchoEtiqueta : 0);
  int16_t x = ((int16_t)OLED_WIDTH - ancho) / 2;
  if (x < 0)
  {
    x = 0;
  }

  if (conEtiqueta)
  {
    x += u8g2.drawStr(x, Y_ETIQUETAS, "Agua "); // drawStr devuelve lo que ocupo
  }
  u8g2.setFont(FONT_GRANDE);
  x += u8g2.drawStr(x, Y_NUMERO, numero);
  u8g2.setFont(FONT_CHICA);
  u8g2.drawStr(x, Y_ETIQUETAS, "\260C");
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
  char banda[12];
  char objetivo[24];
  char pie[24];

  if (temperaturaC == DEVICE_DISCONNECTED_C)
  {
    snprintf(banda, sizeof(banda), "--.-");
  }
  else
  {
    snprintf(banda, sizeof(banda), "%.1f", temperaturaC);
  }

  // El objetivo solo se muestra cuando hay un proceso en juego.
  // Ojo con el simbolo de grado: va en octal ("\260" = 176). Escrito como
  // "\xB0C" el compilador se comeria la 'C' como parte del numero hexadecimal.
  // El %3.0f deja el ancho fijo, asi la linea no se corre al pasar de 90 a 100.
  if (estado == EST_REPOSO)
  {
    objetivo[0] = '\0';
  }
  else
  {
    snprintf(objetivo, sizeof(objetivo), "Llevar a %3.0f\260C", objetivoC());
  }

  textoEstado(pie, sizeof(pie));

  // Mientras se mantiene el pulsador desde el reposo, el pie avisa que se
  // esta por abrir el juego del dino y cuanto falta.
  unsigned long sostenida = msBotonSostenido();
  if (pulsacionDesdeReposo && sostenida >= HOLD_SILENCIO_MS &&
      sostenida < HOLD_JUEGO_MS)
  {
    snprintf(pie, sizeof(pie), "Juego en %lu...",
             (HOLD_JUEGO_MS - sostenida + 999) / 1000);
  }

  // Mandar el buffer es la pantalla entera: 1 KB por I2C, unos 25 ms. Si iba
  // a quedar igual que la anterior, no se gastan.
  static char cacheBanda[12] = "";
  static char cacheObjetivo[24] = "";
  static char cachePie[24] = "";
  if (!oledForzarRedibujo && strcmp(banda, cacheBanda) == 0 &&
      strcmp(objetivo, cacheObjetivo) == 0 && strcmp(pie, cachePie) == 0)
  {
    return;
  }
  oledForzarRedibujo = false;
  strcpy(cacheBanda, banda);
  strcpy(cacheObjetivo, objetivo);
  strcpy(cachePie, pie);

  u8g2.clearBuffer();
  dibujarBandaTemperatura(banda);
  u8g2.setFont(FONT_CHICA);
  if (objetivo[0] != '\0')
  {
    dibujarCentrado(Y_OBJETIVO, objetivo);
  }
  dibujarCentrado(Y_ESTADO, pie);
  u8g2.sendBuffer();
}

// ======================================================== JUEGO DEL DINO ===
// Corredor infinito: el dino salta los cactus que vienen de la derecha, cada
// vez mas rapido. Se abre manteniendo el pulsador HOLD_JUEGO_MS desde el
// reposo. Es un modo aparte de la maquina de estados: el rele queda apagado y
// el sensor se sigue leyendo igual que siempre.
void cambiarEstado(EstadoProceso nuevo); // el juego vuelve solo al reposo

// Sprites en formato XBM: cada byte son 8 pixeles horizontales y el bit menos
// significativo es el de la izquierda. El dino mide 16x16 (2 bytes por fila) y
// los cactus 6 y 8 de ancho (1 byte por fila). Dibujos propios, hechos a mano
// sobre una grilla de papel.
static const uint8_t DINO_CORRE1[] PROGMEM = {
    0x00, 0x7E, 0x00, 0xFF, 0x00, 0xFB, 0x00, 0xFF,
    0x00, 0x1F, 0x00, 0x0F, 0x01, 0x0F, 0x83, 0x0F,
    0xDF, 0x0F, 0xFE, 0x0F, 0xFC, 0x0F, 0xF8, 0x0F,
    0xF8, 0x07, 0x30, 0x03, 0x30, 0x00, 0x78, 0x00,
};
static const uint8_t DINO_CORRE2[] PROGMEM = {
    0x00, 0x7E, 0x00, 0xFF, 0x00, 0xFB, 0x00, 0xFF,
    0x00, 0x1F, 0x00, 0x0F, 0x01, 0x0F, 0x83, 0x0F,
    0xDF, 0x0F, 0xFE, 0x0F, 0xFC, 0x0F, 0xF8, 0x0F,
    0xF8, 0x07, 0x30, 0x03, 0x00, 0x03, 0x80, 0x07,
};
static const uint8_t DINO_MUERTO[] PROGMEM = {
    0x00, 0x7E, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
    0x00, 0x1F, 0x00, 0x0F, 0x01, 0x0F, 0x83, 0x0F,
    0xDF, 0x0F, 0xFE, 0x0F, 0xFC, 0x0F, 0xF8, 0x0F,
    0xF8, 0x07, 0x30, 0x03, 0x30, 0x03, 0x38, 0x07,
};
static const uint8_t CACTUS_CHICO[] PROGMEM = {
    0x0C, 0x0C, 0x0D, 0x2D,
    0x2D, 0x2F, 0x3C, 0x0C,
    0x0C, 0x0C, 0x0C, 0x0C,
};
static const uint8_t CACTUS_GRANDE[] PROGMEM = {
    0x18, 0x18, 0x18, 0x19,
    0x99, 0x99, 0x99, 0x8F,
    0xF8, 0x18, 0x18, 0x18,
    0x18, 0x18, 0x18, 0x18,
};

const uint8_t DINO_ANCHO = 16;
const uint8_t DINO_ALTO = 16;
const uint8_t DINO_X = 10;          // el dino no se mueve: se mueve el mundo
const uint8_t SUELO_Y = 53;         // fila donde apoyan las patas y los cactus
const uint8_t JUEGO_MS_CUADRO = 40; // 25 cuadros por segundo

// Fisica, en pixeles por cuadro. Con estos numeros el salto dura ~0,9 s y
// sube 26 px: le sobran 10 sobre el cactus grande.
const float GRAVEDAD = 0.43f;
const float IMPULSO_SALTO = 4.75f;
const float FRENO_SALTO = 0.55f;      // soltar antes = saltito; aguantar = largo
const float VEL_INICIAL = 2.0f;       // px por cuadro: 50 px/s
const float VEL_MAXIMA = 5.5f;        // el techo, si no se vuelve injugable
const float VEL_INCREMENTO = 0.0015f; // tarda ~1 minuto en llegar al techo

const unsigned long SALIR_JUEGO_MS = 2500;     // boton mantenido -> se cierra
const unsigned long ABANDONO_JUEGO_MS = 30000; // perdido y sin tocar -> reposo

struct TipoObstaculo
{
  const uint8_t *bitmap;
  uint8_t ancho;
  uint8_t alto;
  uint8_t repeticiones; // el cactus doble es el chico dibujado dos veces
  uint8_t paso;         // a que distancia va el segundo
};

const TipoObstaculo OBSTACULOS_POSIBLES[] = {
    {CACTUS_CHICO, 6, 12, 1, 0},
    {CACTUS_GRANDE, 8, 16, 1, 0},
    {CACTUS_CHICO, 6, 12, 2, 7},
};
const uint8_t NUM_TIPOS_OBSTACULO =
    sizeof(OBSTACULOS_POSIBLES) / sizeof(OBSTACULOS_POSIBLES[0]);

uint8_t anchoObstaculo(uint8_t tipo)
{
  const TipoObstaculo &t = OBSTACULOS_POSIBLES[tipo];
  return t.ancho + (t.repeticiones - 1) * t.paso;
}

enum FaseJuego
{
  JUEGO_LISTO,     // cartel inicial, esperando el primer toque
  JUEGO_CORRIENDO, // la partida
  JUEGO_PERDIDO    // choco: cartel con el puntaje
};

struct Obstaculo
{
  float x;
  uint8_t tipo;
  bool activo;
};

FaseJuego faseJuego = JUEGO_LISTO;
Obstaculo obstaculos[3];
float dinoAltura = 0; // pixeles por encima del suelo
float dinoVelY = 0;
bool saltoCortado = false;
float velocidadJuego = VEL_INICIAL;
float distanciaJuego = 0;
uint16_t puntosJuego = 0;
uint16_t recordJuego = 0;   // se mantiene entre partidas, hasta cortar la luz
uint16_t proximoHito = 100; // cada 100 puntos, dos pitidos
bool juegoSaltoPedido = false;
bool juegoEsperaSoltar = true; // al entrar, el dedo todavia esta apoyado
unsigned long juegoMomentoPerdido = 0;

// Deja todo como al principio de una partida. No toca juegoSaltoPedido: el
// mismo toque que arranca el juego cuenta tambien como el primer salto.
void reiniciarPartida(FaseJuego fase)
{
  faseJuego = fase;
  dinoAltura = 0;
  dinoVelY = 0;
  saltoCortado = false;
  velocidadJuego = VEL_INICIAL;
  distanciaJuego = 0;
  puntosJuego = 0;
  proximoHito = 100;
  for (uint8_t i = 0; i < 3; i++)
  {
    obstaculos[i].activo = false;
  }
}

// Dibuja un sprite pixel por pixel, recortando lo que se sale por los
// costados. u8g2 maneja las coordenadas sin signo, asi que el cactus saliendo
// por la izquierda (x negativo) hay que resolverlo a mano.
void dibujarSprite(const uint8_t *bitmap, int16_t x, int16_t y,
                   uint8_t ancho, uint8_t alto)
{
  uint8_t bytesPorFila = (ancho + 7) / 8;
  for (uint8_t fila = 0; fila < alto; fila++)
  {
    for (uint8_t col = 0; col < ancho; col++)
    {
      int16_t px = x + col;
      if (px < 0 || px >= OLED_WIDTH)
      {
        continue;
      }
      uint8_t dato = pgm_read_byte(bitmap + fila * bytesPorFila + (col >> 3));
      if (dato & (1 << (col & 7)))
      {
        u8g2.drawPixel(px, y + fila);
      }
    }
  }
}

// Suelta un cactus nuevo por el borde derecho, pero solo si el anterior ya se
// corrio lo suficiente. El hueco minimo es lo que el dino avanza durante un
// salto entero, asi nunca aparece una pared imposible de pasar.
void generarObstaculo()
{
  int8_t libre = -1;
  float derecha = 0;
  for (uint8_t i = 0; i < 3; i++)
  {
    if (obstaculos[i].activo)
    {
      float fin = obstaculos[i].x + anchoObstaculo(obstaculos[i].tipo);
      if (fin > derecha)
      {
        derecha = fin;
      }
    }
    else if (libre < 0)
    {
      libre = i;
    }
  }
  if (libre < 0)
  {
    return;
  }

  float hueco = 46.0f + velocidadJuego * 15.0f + random(0, 45);
  if (derecha > (float)OLED_WIDTH - hueco)
  {
    return;
  }

  obstaculos[libre].tipo = random(0, NUM_TIPOS_OBSTACULO);
  obstaculos[libre].x = OLED_WIDTH;
  obstaculos[libre].activo = true;
}

// Cajas rectangulares. La del dino va recortada 2 px por lado: perdona los
// roces del hocico y de la cola, que es lo que hace que se sienta justo.
bool hayChoque()
{
  int16_t dinoY = SUELO_Y - DINO_ALTO - (int16_t)dinoAltura;
  int16_t dx1 = DINO_X + 2;
  int16_t dx2 = DINO_X + DINO_ANCHO - 3;
  int16_t dinoPie = dinoY + DINO_ALTO - 1;

  for (uint8_t i = 0; i < 3; i++)
  {
    if (!obstaculos[i].activo)
    {
      continue;
    }
    int16_t ox1 = (int16_t)obstaculos[i].x + 1;
    int16_t ox2 = (int16_t)obstaculos[i].x + anchoObstaculo(obstaculos[i].tipo) - 2;
    int16_t techo = SUELO_Y - OBSTACULOS_POSIBLES[obstaculos[i].tipo].alto;
    // Los cactus llegan hasta el piso: alcanza con mirar si las patas del
    // dino quedaron por debajo del techo del cactus mientras se cruzan.
    if (dx2 >= ox1 && dx1 <= ox2 && dinoPie >= techo)
    {
      return true;
    }
  }
  return false;
}

void dibujarJuego()
{
  // Piedritas del piso: posiciones fijas que se corren con la distancia. Son
  // ellas las que dan la sensacion de velocidad, porque el dino no se mueve.
  static const uint8_t PIEDRAS[] = {5, 19, 28, 44, 57, 66, 81, 95, 104, 119};
  char texto[28];

  u8g2.clearBuffer();

  // Tanteador: el record arriba a la izquierda, lo que va a la derecha.
  u8g2.setFont(FONT_MINI);
  snprintf(texto, sizeof(texto), "HI %04u", recordJuego);
  u8g2.drawStr(2, 7, texto);
  snprintf(texto, sizeof(texto), "%04u", puntosJuego);
  u8g2.drawStr(OLED_WIDTH - u8g2.getStrWidth(texto) - 2, 7, texto);

  u8g2.drawHLine(0, SUELO_Y, OLED_WIDTH);
  int16_t corrimiento = (int32_t)distanciaJuego % OLED_WIDTH;
  for (uint8_t i = 0; i < sizeof(PIEDRAS); i++)
  {
    int16_t px = (int16_t)PIEDRAS[i] - corrimiento;
    if (px < 0)
    {
      px += OLED_WIDTH;
    }
    u8g2.drawPixel(px, SUELO_Y + 3 + (i & 1));
  }

  for (uint8_t i = 0; i < 3; i++)
  {
    if (!obstaculos[i].activo)
    {
      continue;
    }
    const TipoObstaculo &t = OBSTACULOS_POSIBLES[obstaculos[i].tipo];
    for (uint8_t r = 0; r < t.repeticiones; r++)
    {
      dibujarSprite(t.bitmap, (int16_t)obstaculos[i].x + r * t.paso,
                    SUELO_Y - t.alto, t.ancho, t.alto);
    }
  }

  // Las patas cambian cada 6 px recorridos: cuanto mas rapido corre el mundo,
  // mas rapido las mueve. En el cartel inicial trota con el reloj.
  const uint8_t *cuadro = DINO_MUERTO;
  if (faseJuego == JUEGO_LISTO)
  {
    cuadro = ((millis() / 150) & 1) ? DINO_CORRE1 : DINO_CORRE2;
  }
  else if (faseJuego == JUEGO_CORRIENDO)
  {
    cuadro = (((uint32_t)distanciaJuego / 6) & 1) ? DINO_CORRE1 : DINO_CORRE2;
  }
  dibujarSprite(cuadro, DINO_X, SUELO_Y - DINO_ALTO - (int16_t)dinoAltura,
                DINO_ANCHO, DINO_ALTO);

  if (faseJuego == JUEGO_LISTO)
  {
    u8g2.setFont(FONT_CHICA);
    dibujarCentrado(20, "DINO SALTARIN");
    u8g2.setFont(FONT_MINI);
    dibujarCentrado(32, "boton = saltar");
    dibujarCentrado(42, "3 s = salir");
  }

  if (faseJuego == JUEGO_PERDIDO)
  {
    // Cartel encima de la escena congelada: primero se apaga el rectangulo
    // (color 0 = pixel negro) y despues se le dibuja el marco.
    u8g2.setDrawColor(0);
    u8g2.drawBox(4, 8, OLED_WIDTH - 8, 48);
    u8g2.setDrawColor(1);
    u8g2.drawFrame(4, 8, OLED_WIDTH - 8, 48);
    u8g2.setFont(FONT_CHICA);
    dibujarCentrado(24, "PERDISTE");
    u8g2.setFont(FONT_MINI);
    snprintf(texto, sizeof(texto), "%u pts - record %u", puntosJuego, recordJuego);
    dibujarCentrado(36, texto);
    dibujarCentrado(46, "boton = otra vez");
    dibujarCentrado(54, "3 s = salir");
  }

  u8g2.sendBuffer();
}

// Un cuadro del juego. Lo llama el loop() cada JUEGO_MS_CUADRO.
void actualizarJuego()
{
  unsigned long ahora = millis();

  // El dedo viene apoyado desde que se abrio el juego: hasta que no lo
  // suelten no se le hace caso. Si no, el mismo toque que abrio el juego lo
  // cerraria en el acto, porque ya lleva cinco segundos apretado.
  if (juegoEsperaSoltar)
  {
    if (botonApretado())
    {
      dibujarJuego();
      return;
    }
    juegoEsperaSoltar = false;
    juegoSaltoPedido = false;
  }

  // Mantener el boton cierra el juego, en cualquier fase.
  if (msBotonSostenido() >= SALIR_JUEGO_MS)
  {
    botonConsumido = true; // que no pite el buzzer al volver
    juegoSaltoPedido = false;
    oledForzarRedibujo = true; // la pantalla quedo con el juego dibujado
    Serial.printf("[JUEGO] Cerrado. Mejor puntaje: %u\n", recordJuego);
    cambiarEstado(EST_REPOSO);
    return;
  }

  switch (faseJuego)
  {
  case JUEGO_LISTO:
    // El primer toque arranca, y en el cuadro siguiente ya es un salto.
    if (juegoSaltoPedido)
    {
      reiniciarPartida(JUEGO_CORRIENDO);
    }
    break;

  case JUEGO_CORRIENDO:
    if (dinoAltura > 0.0f)
    {
      // Salto de altura variable: si se suelta el boton mientras todavia
      // sube, se le corta el envion (una sola vez por salto).
      if (dinoVelY > 0 && !botonApretado() && !saltoCortado)
      {
        dinoVelY *= FRENO_SALTO;
        saltoCortado = true;
      }
      dinoVelY -= GRAVEDAD;
      dinoAltura += dinoVelY;
      if (dinoAltura <= 0)
      {
        dinoAltura = 0;
        dinoVelY = 0;
      }
    }
    else if (juegoSaltoPedido)
    {
      dinoVelY = IMPULSO_SALTO;
      dinoAltura = 0.01f; // ya despego
      saltoCortado = false;
      programarBeeps(1, 10, 0); // un clic, nada mas
    }
    juegoSaltoPedido = false;

    velocidadJuego += VEL_INCREMENTO;
    if (velocidadJuego > VEL_MAXIMA)
    {
      velocidadJuego = VEL_MAXIMA;
    }
    distanciaJuego += velocidadJuego;
    puntosJuego = (uint16_t)(distanciaJuego / 10.0f);

    for (uint8_t i = 0; i < 3; i++)
    {
      if (!obstaculos[i].activo)
      {
        continue;
      }
      obstaculos[i].x -= velocidadJuego;
      if (obstaculos[i].x + anchoObstaculo(obstaculos[i].tipo) < 0)
      {
        obstaculos[i].activo = false;
      }
    }
    generarObstaculo();

    if (puntosJuego >= proximoHito)
    {
      proximoHito += 100;
      programarBeeps(2, 40, 60); // cada 100 puntos, dos pitidos
    }

    if (hayChoque())
    {
      faseJuego = JUEGO_PERDIDO;
      juegoMomentoPerdido = ahora;
      if (puntosJuego > recordJuego)
      {
        recordJuego = puntosJuego;
      }
      programarBeeps(2, 180, 100);
      Serial.printf("[JUEGO] Perdiste con %u puntos (record %u)\n",
                    puntosJuego, recordJuego);
    }
    break;

  case JUEGO_PERDIDO:
    if (juegoSaltoPedido)
    {
      reiniciarPartida(JUEGO_CORRIENDO);
      juegoSaltoPedido = false; // el toque de la revancha no salta
    }
    else if ((ahora - juegoMomentoPerdido) >= ABANDONO_JUEGO_MS)
    {
      oledForzarRedibujo = true;
      cambiarEstado(EST_REPOSO); // nadie lo agarro mas: vuelve a la pava
      return;
    }
    break;
  }

  dibujarJuego();
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
  case EST_JUEGO:
    return "JUEGO";
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

  case EST_JUEGO:
    ponerRele(false); // el juego no calienta nada
    cancelarBeeps();
    buzzerSostenido = false;
    botonConsumido = true;    // el dedo que abrio el juego no debe pitar
    juegoEsperaSoltar = true; // ni saltar hasta que lo suelten
    juegoSaltoPedido = false;
    randomSeed(micros()); // que los cactus no salgan siempre en el mismo orden
    reiniciarPartida(JUEGO_LISTO);
    break;
  }

  Serial.printf("[ESTADO] %s | objetivo %.0f C\n", nombreEstado(), objetivoC());
}

void procesarEstado()
{
  unsigned long ahora = millis();

  // Una pulsacion larga desde el reposo esta pidiendo el juego: apenas se
  // nota que la estan manteniendo se la da por consumida, asi el buzzer no
  // pita los cinco segundos enteros.
  if (pulsacionDesdeReposo && msBotonSostenido() >= HOLD_SILENCIO_MS)
  {
    botonConsumido = true;
  }

  // El buzzer acompania mientras el dedo siga apoyado, en cualquier etapa.
  buzzerSostenido = botonApretado() && !botonConsumido;

  // En el juego el pulsador es del juego: no cambia la temperatura ni saca
  // del modo. El cuadro lo dibuja el loop(), 25 veces por segundo.
  if (estado == EST_JUEGO)
  {
    if (huboPulsacion())
    {
      juegoSaltoPedido = true;
    }
    return;
  }

  // Cinco segundos apretado desde el reposo: se abre el juego del dino.
  if (pulsacionDesdeReposo && msBotonSostenido() >= HOLD_JUEGO_MS)
  {
    pulsacionDesdeReposo = false;
    indiceTemperatura = indiceTemperaturaPrevio; // esa pulsacion no elegia
    Serial.println("[JUEGO] Abierto con el pulsador mantenido");
    cambiarEstado(EST_JUEGO);
    return;
  }

  // El pulsador manda siempre: corta lo que este haciendo y vuelve a elegir.
  if (huboPulsacion())
  {
    if (estado == EST_REPOSO)
    {
      // Puede ser el arranque de la pulsacion larga que abre el juego: se
      // guarda el objetivo de antes, por si hay que devolverlo.
      pulsacionDesdeReposo = true;
      indiceTemperaturaPrevio = indiceTemperatura;
    }
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
  case EST_JUEGO: // no llega: el juego ya devolvio mas arriba
    break;        // solo se sale de aca pulsando el boton

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
  u8g2.setBusClock(400000); // antes de begin(): I2C rapido = menos bloqueo
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, OLED_WIDTH, OLED_HEIGHT);
  u8g2.setFont(u8g2_font_logisoso16_tr);
  dibujarCentrado(28, "ISET 57");
  dibujarCentrado(52, "2026");
  u8g2.sendBuffer();
  finBienvenida = millis() + DURACION_BIENVENIDA_MS;
  // Ya no se deja una font fija: actualizarOLED() la cambia segun la linea.

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
    if (estado != EST_JUEGO && (long)(millis() - finBienvenida) >= 0)
    {
      actualizarOLED();
    }
  }

  // El juego pide su propio ritmo: 25 cuadros por segundo. Mandar la pantalla
  // entera son ~25 ms de I2C, asi que ese es el techo razonable.
  EVERY_N_MILLIS(JUEGO_MS_CUADRO)
  {
    if (estado == EST_JUEGO)
    {
      actualizarJuego();
    }
  }

  EVERY_N_MILLIS(30)
  {
    actualizarCuadroLeds();
  }
}