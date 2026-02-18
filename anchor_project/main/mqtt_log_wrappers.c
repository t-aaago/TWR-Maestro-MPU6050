/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * All rights reserved
 *****************************************************************************/

/*
 * Wrappers para funções de log do ESP-IDF
 * Necessários para compatibilidade com Arduino ESP32
 * Estes wrappers são chamados pelo WiFi quando há conflito de log
 */

#include <stdarg.h>
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// Wrapper para esp_log_write
void __wrap_esp_log_write(esp_log_level_t level, const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    esp_log_writev(level, tag, format, args);
    va_end(args);
}

// Wrapper para esp_log_writev
void __wrap_esp_log_writev(esp_log_level_t level, const char* tag, const char* format, va_list args) {
    esp_log_writev(level, tag, format, args);
}

#ifdef __cplusplus
}
#endif

