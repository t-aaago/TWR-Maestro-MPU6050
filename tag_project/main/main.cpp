/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * TAG UWB com Leitura de Acelerômetro (MPU6050) - Dual Core
 *****************************************************************************/

#include "Defines.h" // Seus defines (Pinos, Endereços, Modos)
#include <Wire.h>    // Para comunicação I2C com o MPU6050

// ============================================================================
// CONFIGURAÇÕES MPU6050
// ============================================================================
const int MPU_ADDR = 0x68;
const int ACCEL_XOUT_H = 0x3B;
const int PWR_MGMT_1 = 0x6B;

// Variáveis globais para guardar os dados do sensor
// Volatile garante que a leitura entre Cores seja atômica/atualizada
volatile int16_t ax, ay, az;
int16_t offset_ax = 0, offset_ay = 0, offset_az = 0;

// Handles das Tasks
TaskHandle_t handle_task_uwb;
TaskHandle_t handle_task_mpu;

// ============================================================================
// PROTÓTIPOS
// ============================================================================
void task_uwb_routine(void * parameter);
void task_mpu_routine(void * parameter);
void newRange(DW1000Device *device);
void newDevice(DW1000Device *device);
void inactiveDevice(DW1000Device *device);
void readAccelRaw(int16_t &x, int16_t &y, int16_t &z);
void calibrateMPU(int samples);

// ============================================================================
// FUNÇÕES AUXILIARES MPU (Hardware Abstraction)
// ============================================================================

void readAccelRaw(int16_t &x, int16_t &y, int16_t &z) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(ACCEL_XOUT_H);
    Wire.endTransmission(false); // Restart condition
    
    int bytesReceived = Wire.requestFrom(MPU_ADDR, 6, true);
    if(bytesReceived == 6) {
        x = (Wire.read() << 8 | Wire.read());
        y = (Wire.read() << 8 | Wire.read());
        z = (Wire.read() << 8 | Wire.read());
    }
}

void calibrateMPU(int samples) {
    long sum_ax = 0, sum_ay = 0, sum_az = 0;
    int16_t raw_x, raw_y, raw_z;
    
    Serial.println("[MPU] Calibrando...");
    
    for (int i = 0; i < samples; i++) {
        readAccelRaw(raw_x, raw_y, raw_z);
        sum_ax += raw_x; 
        sum_ay += raw_y; 
        sum_az += raw_z;
        delay(3); 
    }
    
    offset_ax = sum_ax / samples;
    offset_ay = sum_ay / samples;
    // O eixo Z deve medir 1G (16384 no range padrão)
    offset_az = (sum_az / samples) - 16384; 
    
    Serial.println("[MPU] Calibrado.");
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    Serial.println("--- Inicializando TAG UWB + MPU ---");

    // 1. Inicializa I2C e MPU6050
    // Pinos padrão do ESP32 (SDA=21, SCL=22) geralmente usados no kit UWB Pro
    Wire.begin(); 
    
    // Acordar o MPU6050 (escrever 0 no Power Management)
    Wire.beginTransmission(MPU_ADDR); 
    Wire.write(PWR_MGMT_1); 
    Wire.write(0); 
    Wire.endTransmission(true);
    
    calibrateMPU(100); // Calibração rápida na inicialização

    // 2. Inicializa SPI e DW1000
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    // Inicialização usando seus DEFINES
    // Nota: Cast (byte*)MODE é necessário pois MODE é constexpr no seu Defines.h
    DW1000Ranging.init(BoardType::DW1000_BOARD_TYPE, 
                       DW1000_TAG_SHORT_ADDRESS, 
                       DW1000_TAG_MAC_ADDRESS, 
                       true, 
                       (byte*)MODE, 
                       DW1000_PIN_RST, 
                       DW1000_PIN_SS, 
                       DW1000_PIN_IRQ);
                       

    DW1000Ranging.attachNewRange(newRange);
    DW1000Ranging.attachNewDevice(newDevice);
    DW1000Ranging.attachInactiveDevice(inactiveDevice);

    Serial.printf("Tag ID: %X | MAC: %s\n", DW1000_TAG_SHORT_ADDRESS, DW1000_TAG_MAC_ADDRESS);

    // 3. Criação das Tasks Dual Core
    
    // Task MPU -> Core 0 (Libera o Core 1 para o Rádio)
    xTaskCreatePinnedToCore(
        task_mpu_routine,   // Função
        "MpuTask",          // Nome
        4096,               // Stack size
        NULL,               // Parametros
        1,                  // Prioridade (Baixa)
        &handle_task_mpu,   // Handle
        0                   // Core 0
    );

    // Task UWB -> Core 1 (Tempo Real)
    xTaskCreatePinnedToCore(
        task_uwb_routine,   // Função
        "UwbTask",          // Nome
        8192,               // Stack size (DW1000 usa bastante pilha)
        NULL,               // Parametros
        5,                  // Prioridade (Alta)
        &handle_task_uwb,   // Handle
        1                   // Core 1
    );
}

void loop() {
    // Arduino Loop padrão deletado para liberar memória para as Tasks
    vTaskDelete(NULL);
}

// ============================================================================
// TASKS
// ============================================================================

// --- CORE 1: PROTOCOLO UWB ---
void task_uwb_routine(void * parameter) {
    Serial.println("[UWB] Task iniciada no Core 1");
    for (;;) {
        DW1000Ranging.loop();
        // Pequeno yield para o Watchdog, mas rápido o suficiente para UWB
        // taskYIELD(); // Use se precisar de velocidade máxima
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}

// --- CORE 0: LEITURA DE SENSORES ---
void task_mpu_routine(void * parameter) {
    Serial.println("[MPU] Task iniciada no Core 0");
    
    int16_t raw_ax, raw_ay, raw_az;

    for (;;) {
        // Leitura I2C (Bloqueante/Lenta) acontece aqui sem travar o UWB
        readAccelRaw(raw_ax, raw_ay, raw_az);
        
        // Aplica Offset
        ax = raw_ax - offset_ax;
        ay = raw_ay - offset_ay;
        az = raw_az - offset_az;
        
        // Passa o dado para a biblioteca DW1000
        // (Assumindo que sua lib customizada tem esse método público)
        DW1000Ranging.setAccelData(ax, ay, az);

        // Taxa de atualização do sensor (50Hz = 20ms)
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

// ============================================================================
// CALLBACKS DW1000
// ============================================================================

void newRange(DW1000Device *device) {
    // Debug mínimo para não poluir a Serial da Tag
    Serial.print("Ancora: ");
    Serial.print(device->getShortAddress(), HEX);
    Serial.print(" | Dist: ");
    Serial.println(device->getRange());
}

void newDevice(DW1000Device *device) {
    Serial.printf("Ancora Adicionada: %X\n", device->getShortAddress());
}

void inactiveDevice(DW1000Device *device) {
    Serial.printf("Ancora Removida: %X\n", device->getShortAddress());
}

// ============================================================================
// ESP-IDF MAIN COMPATIBILITY
// ============================================================================
extern "C" void app_main()
{
    // Garante compatibilidade com o snippet original da UFAM
    initArduino();
    setup();
    
    // O loop infinito do app_main
    while(true) {
        // Como deletamos o loop() do Arduino, apenas dormimos aqui
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}