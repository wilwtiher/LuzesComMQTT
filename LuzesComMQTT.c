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
// Joysticks
#define VRY_PIN 26 // Pino do Joystick Y
#define VRX_PIN 27 // Pino do Joystick X
// Buzzer
#define buzzer 10 // Pino do buzzer A

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

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void);

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Tratamento do request do usuário
void user_request(char **request);

// Função principal
int main()
{
    // Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    // Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
    gpio_led_bitdog();

    // Inicializar a matriz de LEDs
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    adc_init();
    adc_gpio_init(28); // GPIO 28 como entrada analógica

    // Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    // vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    // Inicializa o conversor ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(2);
    while (true)
    {
        /*
         * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
         * Este método deve ser chamado periodicamente a partir do ciclo principal
         * quando se utiliza um estilo de sondagem pico_cyw43_arch
         */
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
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

    // Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

// -------------------------------------- Funções ---------------------------------

// Inicializar os Pinos GPIO para acionamento dos LEDs da BitDogLab
void gpio_led_bitdog(void)
{
    // Configuração dos LEDs como saída
    gpio_init(led_BLUE);
    gpio_set_dir(led_BLUE, GPIO_OUT);
    gpio_put(led_BLUE, false);

    gpio_init(led_GREEN);
    gpio_set_dir(led_GREEN, GPIO_OUT);
    gpio_put(led_GREEN, false);

    gpio_init(led_RED);
    gpio_set_dir(led_RED, GPIO_OUT);
    gpio_put(led_RED, false);
}

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}

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

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);

    // Cria a resposta HTML
    char html[1024];

    // Instruções html do webserver
    snprintf(html, sizeof(html),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head><title>L</title>\n"
             "<style>\n"
             "body { background: #b5e5fb; font-family: Arial; text-align: center; margin-top: 50px; }\n"
             "input, select { font-size: 20px; margin: 10px; }\n"
             "</style>\n"
             "<script>\n"
             "function enviarNumero() {\n"
             "  let valor = document.getElementById('numero').value;\n"
             "  if (valor >= 0 && valor <= 255) fetch('/valor?numero=' + valor);\n"
             "  else alert('0 a 255');\n"
             "}\n"
             "function enviarSeletor(v) { fetch('/seletor?valor=' + v); }\n"
             "function toggleLDR(v) { fetch('/ldr?valor=' + v); }\n"
             "</script>\n"
             "</head>\n"
             "<body>\n"
             "<h2>Luz</h2>\n"
             "<input id=\"numero\" type=\"number\" min=\"0\" max=\"255\" placeholder=\"0-255\" onblur=\"enviarNumero()\">\n"
             "<br>\n"
             "<select onchange=\"enviarSeletor(this.value)\">\n"
             "  <option value=\"0\">0</option>\n"
             "  <option value=\"1\">1</option>\n"
             "  <option value=\"2\">2</option>\n"
             "</select>\n"
             "<br>\n"
             "<label>LDR:</label>\n"
             "<select onchange=\"toggleLDR(this.value)\">\n"
             "  <option value=\"1\">On</option>\n"
             "  <option value=\"0\">Off</option>\n"
             "</select>\n"
             "</body></html>");

    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    // libera memória alocada dinamicamente
    free(request);

    // libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}