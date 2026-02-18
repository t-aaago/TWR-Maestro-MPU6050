# Sistema TWR (Two-way Ranging) com ESP32 UWB DW1000

Este repositório contém o firmware para um sistema de localização em tempo real (RTLS) baseado em Ultra-Wideband (UWB). O sistema consiste em dois tipos de dispositivos: **Tags** (dispositivos móveis) e **Âncoras** (dispositivos fixos).

O sistema utiliza módulos DWM1000 e microcontroladores ESP32, e é desenvolvido sobre o framework ESP-IDF com o componente `arduino-esp32`.

## Arquitetura do Sistema

O sistema opera com base em uma topologia de rede estrela:

-   **Tags**: São os dispositivos móveis cuja posição se deseja determinar. A Tag inicia o processo de comunicação para medição de distância.
-   **Âncoras**: São dispositivos fixos posicionados em locais conhecidos. As Âncoras calculam a distância até as Tags e publicam essa informação em uma rede local via MQTT.

### Fluxo de Comunicação

A comunicação para medição de distância (ranging) é baseada em um protocolo de Two-Way Ranging (TWR) assimétrico, com acesso ao meio por TDMA (Time Division Multiple Access) para evitar colisões.

1.  **Blink (Descoberta)**: Periodicamente, a Tag envia uma mensagem `BLINK` para que novas âncoras na rede possam descobri-la.
2.  **Poll (Sondagem)**: A Tag transmite uma mensagem `POLL` em broadcast, contendo uma lista de até 4 âncoras com as quais deseja se comunicar e seus respectivos *time slots* para resposta.
3.  **Poll ACK (Confirmação)**: Cada Âncora que está na lista responde com uma mensagem `POLL_ACK` em seu *time slot* pré-definido. Isso garante que as respostas não colidam.
4.  **Range (Finalização)**: Após receber os `POLL_ACKs` (ou um timeout), a Tag envia uma mensagem final `RANGE` em broadcast. Esta mensagem contém todos os timestamps necessários (envio e recebimento) coletados das respostas das Âncoras.
5.  **Cálculo da Distância**: Cada Âncora, ao receber a mensagem `RANGE`, possui todos os timestamps necessários para calcular o tempo de voo (Time of Flight - ToF) e, consequentemente, a distância até a Tag.
6.  **Publicação MQTT**: A Âncora publica a distância calculada, juntamente com o ID da Tag, em um tópico MQTT específico.


## Componentes do Firmware

O repositório está dividido em dois projetos principais:

-   `tag_project`: Firmware para o dispositivo Tag.
-   `anchor_project`: Firmware para os dispositivos Âncora.

### Hardware

-   **Microcontrolador**: ESP32
-   **Módulo UWB**: DW1000
-   **Pinos de Conexão (padrão para ambos os projetos)**:
    -   **SPI**:
        -   `SCK`: 18
        -   `MISO`: 19
        -   `MOSI`: 23
    -   **DW1000**:
        -   `RST`: 27
        -   `IRQ`: 34
        -   `SS`: 4

---

## Projeto Tag (`tag_project`)

O firmware da Tag é responsável por orquestrar o processo de ranging.

### Funcionalidades

-   Inicia a comunicação UWB com as âncoras em intervalos regulares.
-   Gerencia uma lista de âncoras ativas.
-   Envia as mensagens `POLL` e `RANGE` conforme o protocolo.
-   Exibe informações sobre dispositivos descobertos e distâncias (se reportadas de volta) no monitor serial.

### Configuração

1.  Abra o arquivo `tag_project/main/inc/Defines.h`.
2.  Configure o `TAG_NUMBER` para um valor entre 1 e 5. Isso definirá o endereço UWB e o endereço MAC do dispositivo, conforme as definições no arquivo.

```c
// ============================================================================
// TAG SELECTION - CONFIGURE THE TAG NUMBER HERE
// ============================================================================
#define TAG_NUMBER 1
```

---

## Projeto Âncora (`anchor_project`)

O firmware da Âncora responde às solicitações da Tag, calcula a distância e publica os dados.

### Funcionalidades

-   Escuta por mensagens `POLL` das Tags.
-   Responde com `POLL_ACK` no time slot correto.
-   Calcula a distância para a Tag após receber a mensagem `RANGE`.
-   Valida a distância para garantir que esteja dentro de limites realistas.
-   Conecta-se a uma rede Wi-Fi.
-   Publica os dados de distância em um broker MQTT em formato JSON: `{"distance": "X.XXX"}`.

### Configuração

1.  Abra o arquivo `anchor_project/main/inc/Defines.h`.
2.  Configure o `ANCHOR_NUMBER` (1 a 5) para definir o endereço UWB, o MAC e o tópico MQTT da âncora.
3.  Insira as credenciais da sua rede Wi-Fi em `WIFI_SSID` and `WIFI_PASSWORD`.
4.  Configure o endereço do seu broker MQTT em `MQTT_BROKER`.
5.  **Calibração**: O parâmetro `antennaDelay` é crucial para a precisão da medição. O valor padrão é `16530`. Para alta precisão, este valor deve ser calibrado para cada âncora individualmente. A função `getAntennaDelayForAnchor` permite definir valores específicos por âncora.

```c
// ============================================================================
// ANCHOR SELECTION - CONFIGURE THE ANCHOR NUMBER HERE
// ============================================================================
#define ANCHOR_NUMBER 1

// ============================================================================
// Wifi Configuration
// ============================================================================
#define WIFI_SSID "SUA_REDE_WIFI"
#define WIFI_PASSWORD "SUA_SENHA_WIFI"

// ============================================================================
// MQTT Configuration
// ============================================================================
#define MQTT_BROKER "mqtt://seu.broker.mqtt:1883"
```

## Como Compilar e Gravar

Este projeto utiliza o **ESP-IDF (Espressif IoT Development Framework)**.

### Pré-requisitos

-   [ESP-IDF configurado](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html).
-   Ferramentas de linha de comando do ESP-IDF (`idf.py`) disponíveis no seu terminal.

### Compilando e Gravando a Tag

1.  Navegue até o diretório do projeto da Tag:
    ```sh
    cd tag_project
    ```
2.  Limpe o projeto (opcional):
    ```sh
    idf.py fullclean
    ```
3.  Compile o projeto:
    ```sh
    idf.py build
    ```
4.  Conecte a Tag via USB e grave o firmware (substitua `SeuPortaSerial` pela porta correta, ex: `/dev/ttyUSB0` ou `COM3`):
    ```sh
    idf.py -p SeuPortaSerial flash
    ```
5.  Para visualizar os logs:
    ```sh
    idf.py -p SeuPortaSerial monitor
    ```

### Compilando e Gravando a Âncora

1.  Navegue até o diretório do projeto da Âncora:
    ```sh
    cd anchor_project
    ```
2.  Siga os mesmos passos 2 a 5 descritos para a Tag. Lembre-se de configurar o `ANCHOR_NUMBER` e os dados de rede antes de compilar.

## Bibliotecas e Dependências

-   **[arduino-esp32](https://github.com/espressif/arduino-esp32)**: Componente para usar o core Arduino no ESP-IDF.
-   **[DW1000_library_pizzo00](https://github.com/pizzo00/UWB-Indoor-Localization_Arduino)**: Biblioteca para comunicação com o módulo UWB DW1000.