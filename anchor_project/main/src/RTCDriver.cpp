/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * All rights reserved
 *****************************************************************************/

/*
RTC Driver - Implementação usando o RTC de hardware do ESP32
*/

#include "RTCDriver.h"
#include "Arduino.h"
#include <sys/time.h>

// Global RTC driver instance
RTCDriver rtcDriver;

// Helper function to get microseconds from ESP32 RTC
static uint64_t getTimeUs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

// Helper function to get milliseconds from ESP32 RTC
static uint32_t getTimeMs() {
  return (uint32_t)(getTimeUs() / 1000ULL);
}

// Constructor
RTCDriver::RTCDriver() : record_count(0), record_index(0), total_publications(0), timer_start_us(0), timer_elapsed_us(0), timer_active(false) {
  memset(records, 0, sizeof(records));
}

// Initialize the RTC driver
void RTCDriver::init() {
  record_count = 0;
  record_index = 0;
  total_publications = 0;
  timer_start_us = 0;
  timer_elapsed_us = 0;
  timer_active = false;
  memset(records, 0, sizeof(records));
  
  Serial.println("RTC Driver initialized.");
}

// Record a publication (automatically uses current RTC time)
void RTCDriver::recordPublication(uint16_t device_addr, float distance) {
  uint64_t current_time_us = getTimeUs();
  
  records[record_index].timestamp_us = current_time_us;
  records[record_index].device_addr = device_addr;
  records[record_index].distance = distance;

  // Timer logic: start or reset
  if (!timer_active) {
    // First measurement - start timer
    timer_start_us = current_time_us;
    timer_active = true;
    timer_elapsed_us = 0;
  } else {
    // Second and subsequent measurements - calculate elapsed and reset
    timer_elapsed_us = current_time_us - timer_start_us;
    timer_start_us = current_time_us;  // Reset for next cycle
  }

  record_index = (record_index + 1) % RTC_MAX_RECORDS;
  if (record_count < RTC_MAX_RECORDS) {
    record_count++;
  }
  total_publications++;
}

// Get publication count in a time window
uint16_t RTCDriver::getPublicationCountInWindow(uint32_t window_size_ms) {
  uint16_t count = 0;
  uint64_t current_time_us = getTimeUs();
  uint64_t window_start_us = current_time_us - (uint64_t)window_size_ms * 1000ULL;

  for (int i = 0; i < record_count; i++) {
    if (records[i].timestamp_us >= window_start_us && records[i].timestamp_us <= current_time_us) {
      count++;
    }
  }
  return count;
}

// Get average distance in a time window
float RTCDriver::getAverageDistanceInWindow(uint32_t window_size_ms) {
  float sum = 0.0f;
  uint16_t count = 0;
  uint64_t current_time_us = getTimeUs();
  uint64_t window_start_us = current_time_us - (uint64_t)window_size_ms * 1000ULL;

  for (int i = 0; i < record_count; i++) {
    if (records[i].timestamp_us >= window_start_us && records[i].timestamp_us <= current_time_us) {
      sum += records[i].distance;
      count++;
    }
  }
  return (count > 0) ? (sum / count) : 0.0f;
}

// Get total publications since boot
uint32_t RTCDriver::getTotalPublications() {
  return total_publications;
}

// Get publication frequency
float RTCDriver::getPublicationFrequency(uint32_t window_size_ms) {
  uint16_t count = getPublicationCountInWindow(window_size_ms);
  float frequency = (float)count / ((float)window_size_ms / 1000.0f);
  return frequency;
}

// Get current record count
uint16_t RTCDriver::getRecordCount() {
  return record_count;
}

// Get current time in microseconds
uint64_t RTCDriver::getCurrentTimeUs() {
  return getTimeUs();
}

// Get current time in milliseconds
uint32_t RTCDriver::getCurrentTimeMs() {
  return getTimeMs();
}

// Get elapsed time since timer start (in milliseconds)
uint32_t RTCDriver::getElapsedTimeMs() {
  if (timer_active && timer_elapsed_us > 0) {
    return (uint32_t)(timer_elapsed_us / 1000ULL);
  }
  return 0;
}

// Get elapsed time since timer start (in microseconds)
uint64_t RTCDriver::getElapsedTimeUs() {
  return timer_elapsed_us;
}

// Check if timer is active
bool RTCDriver::isTimerActive() {
  return timer_active;
}

// Reset timer
void RTCDriver::resetTimer() {
  timer_start_us = getTimeUs();
  timer_elapsed_us = 0;
  timer_active = true;
}

static void printRTCStatistics() {
  uint16_t recent_pubs = rtcDriver.getPublicationCountInWindow(RTC_WINDOW_SIZE_MS);
  float avg_distance = rtcDriver.getAverageDistanceInWindow(RTC_WINDOW_SIZE_MS);
  float frequency = rtcDriver.getPublicationFrequency(RTC_WINDOW_SIZE_MS);
  uint32_t total_pubs = rtcDriver.getTotalPublications();
  uint32_t elapsed_ms = rtcDriver.getElapsedTimeMs();

  Serial.println("\n========== RTC STATISTICS ==========");
  Serial.print("Total publications (boot): ");
  Serial.println(total_pubs);
  Serial.print("Recent publications (60s): ");
  Serial.println(recent_pubs);
  Serial.print("Publication frequency (Hz): ");
  Serial.println(frequency, 2);
  Serial.print("Timer elapsed (ms): ");
  Serial.println(elapsed_ms);
  Serial.print("Average distance (60s): ");
  Serial.println(avg_distance, 3);
  Serial.print("Active records: ");
  Serial.println(rtcDriver.getRecordCount());
  Serial.println("====================================\n");
}

