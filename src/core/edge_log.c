#include "edge_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct edge_logger_s {
    bool console_only;
    char conf_path[256];
};

edge_log_t *edge_log_create(const char *conf_path, bool console_only) {
    edge_log_t *logger = calloc(1, sizeof(struct edge_logger_s));
    logger->console_only = console_only;
    if (conf_path) strncpy(logger->conf_path, conf_path, 255);
    return logger;
}

void edge_log_destroy(edge_log_t *logger) {
    if (logger) free(logger);
}

void edge_log_out(edge_log_t *logger, edge_log_cat_t cat, int level, const char *fmt, ...) {
    (void)logger;
    (void)cat;
    (void)level;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}
