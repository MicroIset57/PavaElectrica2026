#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Configuración Neopixel
#define PIN_NEOPIXEL 25
#define NUM_LEDS 14
#define LED_TYPE NEO_GRB + NEO_KHZ800

void testAllRed();
void testAllGreen();
void testAllBlue();
void testRainbow();
void testWave();
void testBlink();
uint32_t wheelColor(byte wheelPos);

// Crear objeto Neopixel
Adafruit_NeoPixel strip(NUM_LEDS, PIN_NEOPIXEL, LED_TYPE);

// Variables de test
int testMode = 0;
unsigned long lastModeChange = 0;
const unsigned long MODE_CHANGE_INTERVAL = 5000; // Cambiar modo cada 5 segundos

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== INICIANDO TEST NEOPIXEL ESP32 ===");
  Serial.printf("Pin: %d\n", PIN_NEOPIXEL);
  Serial.printf("Cantidad de LEDs: %d\n", NUM_LEDS);
  
  // Inicializar tira Neopixel
  strip.begin();
  strip.setBrightness(100); // Brillo 0-255
  strip.show(); // Apagar todos los LEDs
  
  Serial.println("Tira Neopixel inicializada correctamente");
  Serial.println("\nModos de prueba:");
  Serial.println("0. Todos rojo");
  Serial.println("1. Todos verde");
  Serial.println("2. Todos azul");
  Serial.println("3. Arcoíris");
  Serial.println("4. Efecto onda");
  Serial.println("5. LEDs parpadeantes");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Cambiar modo cada 5 segundos
  if (currentTime - lastModeChange >= MODE_CHANGE_INTERVAL) {
    testMode = (testMode + 1) % 6;
    lastModeChange = currentTime;
    Serial.printf("\n>>> Cambio a modo: %d\n", testMode);
  }
  
  // Ejecutar el modo de prueba actual
  switch(testMode) {
    case 0:
      testAllRed();
      break;
    case 1:
      testAllGreen();
      break;
    case 2:
      testAllBlue();
      break;
    case 3:
      testRainbow();
      break;
    case 4:
      testWave();
      break;
    case 5:
      testBlink();
      break;
  }
  
  delay(50);
}

// ============ FUNCIONES DE TEST ============

// Test 1: Todos los LEDs rojo
void testAllRed() {
  for(int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0)); // Rojo
  }
  strip.show();
}

// Test 2: Todos los LEDs verde
void testAllGreen() {
  for(int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0)); // Verde
  }
  strip.show();
}

// Test 3: Todos los LEDs azul
void testAllBlue() {
  for(int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 255)); // Azul
  }
  strip.show();
}

// Test 4: Efecto arcoíris rotativo
void testRainbow() {
  static unsigned long rainbowTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - rainbowTime >= 100) {
    rainbowTime = currentTime;
    static uint16_t rainbowOffset = 0;
    
    for(int i = 0; i < NUM_LEDS; i++) {
      uint32_t color = wheelColor((i * 256 / NUM_LEDS + rainbowOffset) & 255);
      strip.setPixelColor(i, color);
    }
    strip.show();
    rainbowOffset += 5;
  }
}

// Test 5: Efecto onda (LED se mueve)
void testWave() {
  static unsigned long waveTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - waveTime >= 150) {
    waveTime = currentTime;
    static int wavePos = 0;
    
    // Apagar todos
    for(int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    
    // Encender LED en posición actual
    strip.setPixelColor(wavePos, strip.Color(0, 255, 255)); // Cyan
    
    // LEDs adyacentes con menor intensidad
    if (wavePos > 0) {
      strip.setPixelColor(wavePos - 1, strip.Color(0, 100, 100));
    }
    if (wavePos < NUM_LEDS - 1) {
      strip.setPixelColor(wavePos + 1, strip.Color(0, 100, 100));
    }
    
    strip.show();
    wavePos = (wavePos + 1) % NUM_LEDS;
  }
}

// Test 6: LEDs parpadeantes
void testBlink() {
  static unsigned long blinkTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - blinkTime >= 300) {
    blinkTime = currentTime;
    static bool blinkOn = false;
    
    if (blinkOn) {
      // Encender en amarillo
      for(int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(255, 255, 0));
      }
    } else {
      // Apagar
      for(int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
    }
    strip.show();
    blinkOn = !blinkOn;
  }
}

// Función auxiliar para generar colores del arcoíris
uint32_t wheelColor(byte wheelPos) {
  wheelPos = 255 - wheelPos;
  if(wheelPos < 85) {
    return strip.Color(255 - wheelPos * 3, 0, wheelPos * 3);
  }
  if(wheelPos < 170) {
    wheelPos -= 85;
    return strip.Color(0, wheelPos * 3, 255 - wheelPos * 3);
  }
  wheelPos -= 170;
  return strip.Color(wheelPos * 3, 255 - wheelPos * 3, 0);
}