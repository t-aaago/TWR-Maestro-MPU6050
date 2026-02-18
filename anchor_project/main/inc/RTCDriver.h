/******************************************************************************
 * Copyright © 2025 HUB I4.0 - Universidade do Estado do Amazonas
 * All rights reserved
 *****************************************************************************/
/*
RTC Driver - Implementação usando o RTC de hardware do ESP32
*/

#pragma once

#include <cstdint>
#include <cstring>
#include <sys/time.h>

// ============================================================================
// RTC CONFIGURATION
// ============================================================================
#define RTC_MAX_RECORDS 100          // Maximum publication records to store
#define RTC_WINDOW_SIZE_MS 60000     // Time window for statistics (60 seconds)

// ============================================================================
// RTC STRUCTURE AND FUNCTIONS
// ============================================================================

struct PublicationRecord {
  uint64_t timestamp_us;    // Microseconds (from ESP32 hardware RTC)
  uint16_t device_addr;     // Device address
  float distance;           // Distance value published
};

class RTCDriver {
private:
  PublicationRecord records[RTC_MAX_RECORDS];
  uint16_t record_count;
  uint16_t record_index;
  uint32_t total_publications;
  uint64_t timer_start_us;            // Timer start time (first measurement)
  uint64_t timer_elapsed_us;          // Elapsed time since timer start
  bool timer_active;                  // Timer is running

public:
  // Constructor
  RTCDriver();

  // Initialize the RTC driver (sets up ESP32 RTC)
  void init();

  // Record a publication (uses ESP32 hardware RTC timestamp)
  void recordPublication(uint16_t device_addr, float distance);

  // Get publication count in a time window (milliseconds from now)
  uint16_t getPublicationCountInWindow(uint32_t window_size_ms);

  // Get average distance of publications in a time window
  float getAverageDistanceInWindow(uint32_t window_size_ms);

  // Get total publications since boot
  uint32_t getTotalPublications();

  // Get publication frequency (publications per second in window)
  float getPublicationFrequency(uint32_t window_size_ms);

  // Get current record count
  uint16_t getRecordCount();

  // Get current time in milliseconds
  uint32_t getCurrentTimeMs();

  // Get current time in microseconds
  uint64_t getCurrentTimeUs();

  // Get elapsed time since timer start (in milliseconds)
  uint32_t getElapsedTimeMs();

  // Get elapsed time since timer start (in microseconds)
  uint64_t getElapsedTimeUs();

  // Check if timer is active
  bool isTimerActive();

  // Reset timer (called automatically on each new distance)
  void resetTimer();
};

// Global RTC driver instance
extern RTCDriver rtcDriver;
