/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * All rights reserved
 *****************************************************************************/
/*
DriverSNTP - arquivo fonte do driver SNTP (Sistema de Tempo e Data)
*/

#include "DriverSNTP.h"
uint8_t milisec = 0;

char server_sntp[] = "a.st1.ntp.br"; //"pool.ntp.org" "192.168.0.182" "172.31.111.97" "172.31.111.98" "a.st1.ntp.br"

void initialize_sntp()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, (char*) server_sntp); 
    sntp_init();
    setenv("TZ", "GMT+4", 1);
    tzset();
    // wait for time to be set
    int retry = 0;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        ESP_LOGI("SNTP DEBUG", "Waiting for system time to be set... (%d)[%s]", retry++, (char*) server_sntp);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}


int get_timedate(char * timedate_buffer)
{
    milisec++;
    char milisec_p[6];
    struct tm timeinfo;
    time_t now;

    time(&now);

    localtime_r(&now, &timeinfo);
    strftime(timedate_buffer, 21, "%Y-%m-%dT%H:%M:%S", &timeinfo);
    sprintf(milisec_p, ".%03uZ", milisec);
    strcat(timedate_buffer, milisec_p);



    return timeinfo.tm_sec;
}

time_t compare_timedate(time_t datetime_to_compare){
    time_t now;

    time(&now);

    return datetime_to_compare - now;
}
