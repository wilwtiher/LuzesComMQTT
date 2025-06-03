/**
 * AULA IoT - Embarcatech - Ricardo Prates - 004 - Webserver Raspberry Pi Pico w - wlan
 *
 * Material de suporte
 *
 * https://www.raspberrypi.com/documentation/pico-sdk/networking.html#group_pico_cyw43_arch_1ga33cca1c95fc0d7512e7fef4a59fd7475
 */

#include <stdio.h>  // Biblioteca padrão para entrada e saída
#include <string.h> // Biblioteca manipular strings
#include <stdlib.h> // funções para realizar várias operações, incluindo alocação de memória dinâmica (malloc)

#include "pico/stdlib.h"     // Biblioteca da Raspberry Pi Pico para funções padrão (GPIO, temporização, etc.)
#include "hardware/adc.h"    // Biblioteca da Raspberry Pi Pico para manipulação do conversor ADC
#include "pico/cyw43_arch.h" // Biblioteca para arquitetura Wi-Fi da Pico com CYW43
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "pico/time.h"

#include "lwip/pbuf.h"  // Lightweight IP stack - manipulação de buffers de pacotes de rede
#include "lwip/tcp.h"   // Lightweight IP stack - fornece funções e estruturas para trabalhar com o protocolo TCP
#include "lwip/netif.h" // Lightweight IP stack - fornece funções e estruturas para trabalhar com interfaces de rede (netif)

// Credenciais WIFI - Tome cuidado se publicar no github!
#define WIFI_SSID "TAWLS"
#define WIFI_PASSWORD "0123456789"

// Para o I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// Matriz de LEDS
#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
// Armazenar a cor (Entre 0 e 255 para intensidade)
int led_r = 5;  // Intensidade do vermelho
int led_g = 5;  // Intensidade do verde
int led_b = 5;  // Intensidade do azul
int sled_r = 5; // Intensidade do vermelho salvar
int sled_g = 5; // Intensidade do verde salvar
int sled_b = 5; // Intensidade do azul salvar
// Pinos
// LEDS
#define led_RED 13   // Red=13, Blue=12, Green=11
#define led_BLUE 12  // Red=13, Blue=12, Green=11
#define led_GREEN 11 // Red=13, Blue=12, Green=11
// Botoes
#define botao_pinA 5 // Botão A = 5, Botão B = 6 , BotãoJoy = 22
#define botao_pinB 6 // Botão A = 5, Botão B = 6 , BotãoJoy = 22

// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN // GPIO do CI CYW43

// Variáveis globais
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)
bool display = true;
bool LEDS = true;
bool Verde = false;
bool cor = true;
bool alarme = false;
bool LDR = true;
static volatile int8_t contador = 0; // Variável para qual frame será chamado da matriz de LEDs
int16_t displayX = 0;
int16_t displayY = 0;
absolute_time_t last_request_time;
// Variável para os frames da matriz de LEDs
bool led_buffer[4][NUM_PIXELS] = {
    {0, 0, 0, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 1, 0, 0,
     0, 0, 0, 0, 0,
     0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0,
     0, 0, 1, 0, 0,
     0, 1, 1, 1, 0,
     0, 0, 1, 0, 0,
     0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0,
     0, 1, 1, 1, 0,
     0, 1, 1, 1, 0,
     0, 1, 1, 1, 0,
     0, 0, 0, 0, 0},
    {0, 0, 1, 0, 0,
     0, 1, 1, 1, 0,
     1, 1, 1, 1, 1,
     0, 1, 1, 1, 0,
     0, 0, 1, 0, 0}};

// Funções para matriz LEDS
static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}
void set_one_led(uint8_t r, uint8_t g, uint8_t b)
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (led_buffer[contador /*variavel do arrey do buffer*/][i])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0); // Desliga os LEDs com zero no buffer
        }
    }
}

// Função principal
int main()
{
    // Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    // Inicializar a matriz de LEDs
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    adc_init();
    adc_gpio_init(28); // GPIO 28 como entrada analógica

    // Inicializa o conversor ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(2);
    while (true)
    {
        if (LDR)
        {
            uint16_t valorLDR = adc_read();
            uint16_t intensidade = (valorLDR * 0.03); // Normalizado para 0–120
            led_r = intensidade;
            led_g = intensidade;
            led_b = intensidade;
            set_one_led(led_r, led_g, led_b);
            printf("Valor ldr: %d\n", intensidade);
            sleep_ms(150); // Atualiza menos
        }
        sleep_ms(50); // Reduz o uso da CPU
    }
    return 0;
}

// -------------------------------------- Funções ---------------------------------

// Tratamento do request do usuário - digite aqui
void user_request(char **request)
{
    absolute_time_t now = get_absolute_time();
    int elapsed = absolute_time_diff_us(last_request_time, now) / 1000; // ms
    if (elapsed < 200)
    {
        // Ignora requisição se for muito cedo
        printf("Request ignorado: intervalo de %d ms insuficiente\n", elapsed);
        return;
    }
    last_request_time = now;
    if (strstr(*request, "GET /valor?numero=") != NULL)
    {
        // Extrai o valor da URL
        char *valorStr = strstr(*request, "/valor?numero=") + strlen("/valor?numero=");
        int valor = atoi(valorStr);
        printf("Valor recebido: %d\n", valor);

        // Lógica de controle com o valor
        sled_r = valor;
        sled_g = valor;
        sled_b = valor;
        led_r = sled_r;
        led_g = sled_g;
        led_b = sled_b;
        set_one_led(led_r, led_g, led_b);
    }
    else if (strstr(*request, "GET /seletor?valor=") != NULL)
    {
        // Extrai o valor do seletor da URL
        char *valorStr = strstr(*request, "/seletor?valor=") + strlen("/seletor?valor=");
        int valor = atoi(valorStr);
        printf("Valor do seletor: %d\n", valor);
        // Aqui você pode tratar o valor selecionado
        contador = valor;
        set_one_led(led_r, led_g, led_b);
    }
    else if (strstr(*request, "GET /ldr?valor=") != NULL)
    {
        char *valorStr = strstr(*request, "/ldr?valor=") + strlen("/ldr?valor=");
        int valor = atoi(valorStr);
        LDR = (valor == 1);
        printf("Modo LDR: %d\n", LDR);
        if (!LDR)
        {
            led_r = sled_r;
            led_g = sled_g;
            led_b = sled_b;
            set_one_led(led_r, led_g, led_b);
        }
    }
};