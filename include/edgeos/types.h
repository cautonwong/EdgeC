/**
 * @file types.h
 * @brief EdgeOS Public SDK Standard Data Types and Message Structures
 */

#ifndef EDGEOS_SDK_TYPES_H
#define EDGEOS_SDK_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "contract_data_types.h"
#include "shmring.h"

#ifdef __cplusplus
extern "C" {
#endif

// SDK Convenience Aliases
typedef PipeMsg edgeos_pipe_msg_t;
typedef ShmRing edgeos_shm_ring_t;

#ifdef __cplusplus
}
#endif

#endif /* EDGEOS_SDK_TYPES_H */
