/**
 * @file main.c
 * @brief ESP32 WS2812 RGB LED Controller Demo
 * 
 * Controla una tira de LEDs RGB WS2812 (NeoPixels) en un ESP32
 * Comunicación interactiva por UART
 * 
 * Conexiones:
 *   ESP32 GPIO25 ---> LED WS2812 Data (DIN)
 *   ESP32 GND    ---> LED WS2812 GND
 *   5V PSU       ---> LED WS2812 +5V
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/rmt_tx.h"
#include "led_strip.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "WS2812_DEMO";

// ======================== CONFIGURACIÓN ========================
#define LED_STRIP_GPIO_PIN       CONFIG_LED_GPIO_PIN
#define NUM_LEDS                 CONFIG_NUM_LEDS
#define LED_UPDATE_SPEED_MS      CONFIG_LED_UPDATE_SPEED_MS
#define RMT_LED_CHANNEL          RMT_CHANNEL_0
#define UART_NUM                 (uart_port_t)CONFIG_UART_PORT
#define UART_TX_PIN              1
#define UART_RX_PIN              3
#define UART_BAUD_RATE           CONFIG_UART_BAUD_RATE

// Buffer para menú
#define INPUT_BUFFER_SIZE 64

// ======================== VARIABLES GLOBALES ========================
led_strip_handle_t led_strip = NULL;
static bool demo_running = false;

// ======================== ESTRUCTURAS ========================
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_color_t;

// ======================== FUNCIONES AUXILIARES ========================

/**
 * @brief Inicializa la tira de LEDs WS2812
 */
static esp_err_t init_led_strip(void) {
    ESP_LOGI(TAG, "Inicializando tira de LEDs WS2812...");
    
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_PIN,
        .max_leds = NUM_LEDS,
        .led_model = LED_MODEL_WS2812,
        .flags.with_dma = true,
    };
    
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = true,
    };
    
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "✓ Tira de LEDs inicializada: %d LEDs en GPIO %d", NUM_LEDS, LED_STRIP_GPIO_PIN);
    
    return ESP_OK;
}

/**
 * @brief Apaga todos los LEDs
 */
static void clear_leds(void) {
    for (int i = 0; i < NUM_LEDS; i++) {
        led_strip_set_pixel(led_strip, i, 0, 0, 0);
    }
    led_strip_refresh(led_strip);
}

/**
 * @brief Establece todos los LEDs a un color específico
 */
static void set_all_color(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < NUM_LEDS; i++) {
        led_strip_set_pixel(led_strip, i, r, g, b);
    }
    led_strip_refresh(led_strip);
}

/**
 * @brief Convierte HSV a RGB
 */
static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    h = h % 360;
    float hf = h / 60.0f;
    float sf = s / 255.0f;
    float vf = v / 255.0f;
    
    float c = vf * sf;
    float x = c * (1.0f - fabs(fmod(hf, 2.0f) - 1.0f));
    float m = vf - c;
    
    float rf = 0, gf = 0, bf = 0;
    
    if (hf < 1.0f) {
        rf = c; gf = x; bf = 0;
    } else if (hf < 2.0f) {
        rf = x; gf = c; bf = 0;
    } else if (hf < 3.0f) {
        rf = 0; gf = c; bf = x;
    } else if (hf < 4.0f) {
        rf = 0; gf = x; bf = c;
    } else if (hf < 5.0f) {
        rf = x; gf = 0; bf = c;
    } else {
        rf = c; gf = 0; bf = x;
    }
    
    *r = (uint8_t)((rf + m) * 255);
    *g = (uint8_t)((gf + m) * 255);
    *b = (uint8_t)((bf + m) * 255);
}

/**
 * @brief Efecto: Color Rojo Fijo
 */
static void effect_solid_red(void) {
    printf("Mostrando color Rojo...\n");
    set_all_color(255, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(3000));
}

/**
 * @brief Efecto: Color Verde Fijo
 */
static void effect_solid_green(void) {
    printf("Mostrando color Verde...\n");
    set_all_color(0, 255, 0);
    vTaskDelay(pdMS_TO_TICKS(3000));
}

/**
 * @brief Efecto: Color Azul Fijo
 */
static void effect_solid_blue(void) {
    printf("Mostrando color Azul...\n");
    set_all_color(0, 0, 255);
    vTaskDelay(pdMS_TO_TICKS(3000));
}

/**
 * @brief Efecto: Arco Iris
 */
static void effect_rainbow(void) {
    printf("Efecto Arco Iris (10 ciclos)...\n");
    
    for (int cycle = 0; cycle < 10; cycle++) {
        for (int shift = 0; shift < 360; shift += 3) {
            for (int i = 0; i < NUM_LEDS; i++) {
                uint16_t hue = (shift + i * 360 / NUM_LEDS) % 360;
                uint8_t r, g, b;
                hsv_to_rgb(hue, 255, 200, &r, &g, &b);
                led_strip_set_pixel(led_strip, i, r, g, b);
            }
            led_strip_refresh(led_strip);
            vTaskDelay(pdMS_TO_TICKS(LED_UPDATE_SPEED_MS / 3));
        }
    }
}

/**
 * @brief Efecto: Persecución Bicolor
 */
static void effect_chase(void) {
    printf("Persecución Bicolor (Rojo-Verde)...\n");
    
    for (int cycle = 0; cycle < 5; cycle++) {
        for (int i = 0; i < NUM_LEDS * 2; i++) {
            for (int j = 0; j < NUM_LEDS; j++) {
                if ((j + i) % 2 == 0) {
                    led_strip_set_pixel(led_strip, j, 255, 0, 0);   // Rojo
                } else {
                    led_strip_set_pixel(led_strip, j, 0, 255, 0);   // Verde
                }
            }
            led_strip_refresh(led_strip);
            vTaskDelay(pdMS_TO_TICKS(150));
        }
    }
}

/**
 * @brief Efecto: Pulsación
 */
static void effect_pulse(void) {
    printf("Pulsación Cian...\n");
    
    for (int cycle = 0; cycle < 3; cycle++) {
        // Fade in
        for (int brightness = 0; brightness <= 255; brightness += 5) {
            uint8_t r = (0 * brightness) / 255;
            uint8_t g = (255 * brightness) / 255;
            uint8_t b = (255 * brightness) / 255;
            set_all_color(r, g, b);
            vTaskDelay(pdMS_TO_TICKS(30));
        }
        
        // Fade out
        for (int brightness = 255; brightness >= 0; brightness -= 5) {
            uint8_t r = (0 * brightness) / 255;
            uint8_t g = (255 * brightness) / 255;
            uint8_t b = (255 * brightness) / 255;
            set_all_color(r, g, b);
            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }
}

/**
 * @brief Efecto: Persecución Teatral
 */
static void effect_theater_chase(void) {
    printf("Persecución Teatral (Blanco)...\n");
    
    for (int q = 0; q < 3; q++) {
        for (int repeat = 0; repeat < 2; repeat++) {
            for (int i = 0; i < NUM_LEDS; i++) {
                if ((i + q) % 3 == 0) {
                    led_strip_set_pixel(led_strip, i, 255, 255, 255);  // Blanco
                } else {
                    led_strip_set_pixel(led_strip, i, 0, 0, 0);        // Apagado
                }
            }
            led_strip_refresh(led_strip);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

/**
 * @brief Efecto: Chispas Aleatorias
 */
static void effect_sparkles(void) {
    printf("Chispas Aleatorias (5 segundos)...\n");
    
    uint32_t start_time = xTaskGetTickCount();
    uint32_t duration_ticks = pdMS_TO_TICKS(5000);
    
    while ((xTaskGetTickCount() - start_time) < duration_ticks) {
        clear_leds();
        
        for (int i = 0; i < 5; i++) {
            int idx = rand() % NUM_LEDS;
            uint8_t r = 100 + (rand() % 156);
            uint8_t g = 100 + (rand() % 156);
            uint8_t b = 100 + (rand() % 156);
            led_strip_set_pixel(led_strip, idx, r, g, b);
        }
        
        led_strip_refresh(led_strip);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ======================== CONFIGURACIÓN UART ========================

/**
 * @brief Inicializa la comunicación UART
 */
static void init_uart(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, 256, 0, 0, NULL, 0);
    
    ESP_LOGI(TAG, "✓ UART inicializado en puerto %d a %d baud", UART_NUM, UART_BAUD_RATE);
}

/**
 * @brief Muestra el menú de demostración
 */
static void show_menu(void) {
    printf("\n");
    printf("==================================================\n");
    printf("      ESP32 WS2812 RGB LED DEMO\n");
    printf("==================================================\n");
    printf("1. Color Rojo Fijo\n");
    printf("2. Color Verde Fijo\n");
    printf("3. Color Azul Fijo\n");
    printf("4. Arco Iris\n");
    printf("5. Persecución Bicolor\n");
    printf("6. Pulsación\n");
    printf("7. Persecución Teatral\n");
    printf("8. Chispas Aleatorias\n");
    printf("9. Apagar LEDs\n");
    printf("0. Reiniciar ESP32\n");
    printf("==================================================\n");
    printf("\nIngresa una opción (0-9): ");
    fflush(stdout);
}

/**
 * @brief Lee una línea desde UART
 */
static int read_line(char *buffer, int max_length) {
    int index = 0;
    while (index < max_length - 1) {
        int len = uart_read_bytes(UART_NUM, (uint8_t *)buffer + index, 1, pdMS_TO_TICKS(100));
        if (len == 1) {
            if (buffer[index] == '\n' || buffer[index] == '\r') {
                buffer[index] = '\0';
                printf("\n");
                return index;
            }
            printf("%c", buffer[index]);  // Echo del carácter
            index++;
        }
    }
    buffer[index] = '\0';
    return index;
}

/**
 * @brief Ejecuta el efecto seleccionado
 */
static void run_demo(int option) {
    switch (option) {
        case 1:
            effect_solid_red();
            break;
        case 2:
            effect_solid_green();
            break;
        case 3:
            effect_solid_blue();
            break;
        case 4:
            effect_rainbow();
            break;
        case 5:
            effect_chase();
            break;
        case 6:
            effect_pulse();
            break;
        case 7:
            effect_theater_chase();
            break;
        case 8:
            effect_sparkles();
            break;
        case 9:
            printf("Apagando LEDs...\n");
            clear_leds();
            break;
        case 0:
            printf("Reiniciando ESP32...\n");
            esp_restart();
            break;
        default:
            printf("Opción inválida. Por favor ingresa un número entre 0 y 9\n");
    }
}

// ======================== TAREA PRINCIPAL ========================

/**
 * @brief Tarea principal del programa
 */
static void demo_task(void *arg) {
    char buffer[INPUT_BUFFER_SIZE];
    
    printf("\n");
    printf("=================================================\n");
    printf("✓ ESP32 inicializado correctamente\n");
    printf("✓ %d LEDs WS2812 configurados en GPIO %d\n", NUM_LEDS, LED_STRIP_GPIO_PIN);
    printf("=================================================\n");
    
    // Luz de bienvenida
    clear_leds();
    vTaskDelay(pdMS_TO_TICKS(500));
    set_all_color(0, 100, 0);  // Verde
    vTaskDelay(pdMS_TO_TICKS(500));
    clear_leds();
    
    demo_running = true;
    
    while (demo_running) {
        show_menu();
        
        // Leer entrada del usuario
        if (read_line(buffer, INPUT_BUFFER_SIZE) > 0) {
            int option = atoi(buffer);
            
            if (option >= 0 && option <= 9) {
                run_demo(option);
            } else {
                printf("Opción inválida. Por favor ingresa un número entre 0 y 9\n");
            }
        }
        
        clear_leds();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    vTaskDelete(NULL);
}

// ======================== PUNTO DE ENTRADA ========================

void app_main(void) {
    ESP_LOGI(TAG, "======================================");
    ESP_LOGI(TAG, "PavaElectrica 2026 - WS2812 LED");
    ESP_LOGI(TAG, "======================================");
    
    // Inicializar UART
    init_uart();
    
    // Inicializar tira de LEDs
    ESP_ERROR_CHECK(init_led_strip());
    
    // Crear tarea demo
    xTaskCreate(demo_task, "demo_task", 4096, NULL, 5, NULL);
}
