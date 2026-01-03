#include <stdio.h>
#include <stdlib.h>

#include "contracts.h"
#include "edge_log.h"
#include "edge_service.h"

static service_t *g_svc = NULL;

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    edge_log_init(NULL, false);
    if (edge_contract_self_test() != 0) return 1;

    printf("--- EdgeOS Smart Gateway [Container Mode] ---\n");

    service_options_t opts = {
        .product_name = "sg_v1", .db_path = "sg.db", .web_port = 38080, .rest_port = 38081};
    g_svc = service_new(&opts);

    return service_run(g_svc);
}
