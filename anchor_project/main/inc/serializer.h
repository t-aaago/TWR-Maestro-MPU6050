#ifndef SERIALIZER_H
#define SERIALIZER_H 

#include <stddef.h>
#include <stdint.h>
#include "types.h"


typedef size_t (*serializefn) (const range_pkg_t* data, uint8_t* out_buffer, size_t max_len);

typedef struct
{
    serializefn serialize;
} serializer_t;

#endif // SERIALIZER_H