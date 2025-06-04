# Aluno: Wilton Lacerda Silva Júnior
## Matrícula: TIC370100193
# Video explicativo: https://youtu.be/ieBLRupWZZY
# Projeto MQTT Luzes
O objetivo do projeto é aprimorar o conhecimento do MQTT utilizando algum projeto idealizado. O projeto idealizado foi o anteriormente trabalhado de automatização de luzes para residências
## Funcionalidades

- **MATRIZ DE LEDs**
   - A matriz de LEDs representará as lâmpadas de uma casa.
- **LDR**
   - O LDR servirá como sensor de luz para automatizar as luzes caso seja solicitado.
- **INTEGRAÇÃO MQTT**
   - A integração com o MQTT permite uma interface e utilização do programa com outros aparelhos, ajudando a utilizar de maneira mais simples e ter outros tipos de retornos.

# Requisitos
## Hardware:

- Raspberry Pi Pico W.
- 1 matriz de LEDs 5x5 na porta 7.
- Servidor MQTT.
- 1 LDR no pino 28.

## Software:

- Ambiente de desenvolvimento VS Code com extensão Pico SDK.
- Cliente MQTT externo para utilização

# Instruções de uso
## Configure o ambiente:
- Certifique-se de que o Pico SDK está instalado e configurado no VS Code.
- Compile o código utilizando a extensão do Pico SDK.
- Altere as portas MQTT para sua própria rede e servidor MQTT
## Teste:
- Utilize a placa BitDogLab para o teste. Caso não tenha, conecte os hardwares informados acima nos pinos correspondentes.

# Explicação do projeto:
## Contém:
- O projeto terá uma forma de comunicação direta com o usuário: o meio MQTT, com clientes MQTT.
- Sensor LDR para automatização na intensidade da luz.
- Também contará com uma saída visual na placa, a matriz de LEDs.

## Funcionalidades:
- O programa mostrará uma representação de luzes da residência na matriz de led.
- O programa acessara a internet via rede local.
- O programa conectará ao servidor MQTT presente na rede local.
- O MQTT ajudará a controlar as luzes da residência via cliente MQTT.
- O usuário poderá passar comandos para o microcontrolador via MQTT.
- O usuário terá retorno via MQTT.

## Tópicos MQTT usados:
- /ldr: Alterá a automação com LDR.
- /iluminacao: retorno do valor do LDR.
- /ping: Retorna no /uptime o tempo ligado, e serva para passar a intensidade das lâmpadas.
- /print: Imprime na USB o valor passado, e recebe um valor de 0 a 3 para alterar a quantidade de lâmpadas ligadas. Respectivamente 1, 5, 9, 13.
