#ifndef HAL_INTERNAL_H
#define HAL_INTERNAL_H

#include <stdint.h>

/* Internal functions exposed for testing */
void Hal_SetReloadsCounter(const uint32_t value);
void Hal_SetNsPerTickQ32(const uint64_t value);

#endif