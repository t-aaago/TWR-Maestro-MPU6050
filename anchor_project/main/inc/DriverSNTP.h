/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * All rights reserved
 *****************************************************************************/
/*
DriverSNTP - arquivo header do driver SNTP (Sistema de Tempo e Data)
*/
#pragma once

#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_sntp.h"



void initialize_sntp();
int get_timedate(char * timedate_buffer);
time_t compare_timedate(time_t datetime_to_compare);
