#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

typedef struct
{
    uint16_t anchor_id;
    uint16_t tag_id;

    float distance;
    int16_t ax;
    int16_t ay;
    int16_t az;
    
    int64_t timestamp_us;
    
    float rp_power;
    float fp_power;
    float eta;
    float quality;
} range_pkg_t;


#endif // TYPES_H