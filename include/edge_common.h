#ifndef EDGE_COMMON_H
#define EDGE_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

// 统一状态码
typedef enum {
    EDGE_OK = 0,
    EDGE_ERR_GENERIC = -1,
    EDGE_ERR_INVALID_PARAM = -2,
    EDGE_ERR_NOMEM = -3,
    EDGE_ERR_TIMEOUT = -4,
    EDGE_ERR_BUSY = -5,
    EDGE_ERR_NOT_FOUND = -6,
    EDGE_ERR_NOT_SUPPORTED = -7,
    EDGE_ERR_IO = -8,
    EDGE_ERR_OVERFLOW = -9
} edge_status_t;

// 获取错误描述
static inline const char* edge_strerror(edge_status_t err) {
    switch (err) {
        case EDGE_OK:
            return "Success";
        case EDGE_ERR_GENERIC:
            return "Generic error";
        case EDGE_ERR_INVALID_PARAM:
            return "Invalid parameter";
        case EDGE_ERR_NOMEM:
            return "Out of memory";
        case EDGE_ERR_TIMEOUT:
            return "Operation timed out";
        case EDGE_ERR_BUSY:
            return "Resource busy";
        case EDGE_ERR_NOT_FOUND:
            return "Not found";
        case EDGE_ERR_NOT_SUPPORTED:
            return "Not supported";
        case EDGE_ERR_IO:
            return "I/O error";
        case EDGE_ERR_OVERFLOW:
            return "Buffer overflow";
        default:
            return "Unknown error";
    }
}

// 通用宏
#define EDGE_UNUSED(x) (void)(x)
#define EDGE_SAFE_FREE(ptr) \
    do {                    \
        if (ptr) {          \
            free(ptr);      \
            ptr = NULL;     \
        }                   \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif  // EDGE_COMMON_H
