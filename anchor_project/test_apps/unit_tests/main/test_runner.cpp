#include "unity.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Declaração das funções de ligação
extern void force_link_json();
extern void force_link_network();

extern "C" void app_main(void) {
    // 1. Invoca os ficheiros para o Linker os fundir no binário
    force_link_json();
    force_link_network();

    // 2. Aguarda estabilização do terminal serial
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // 3. Executa a bateria de testes destrutivos
    UNITY_BEGIN();
    unity_run_all_tests();
    UNITY_END();
}