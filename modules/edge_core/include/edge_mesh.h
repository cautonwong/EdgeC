#ifndef EDGE_MESH_H
#define EDGE_MESH_H

#include <stdint.h>
#include <stdbool.h>
#include "contract_data_types.h"
#include "datahub.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_MESH_MAGIC 0x4D455348 // "MESH"

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t node_id;
    uint64_t sequence_id;
    uint64_t mtt_timestamp;
    uint32_t tag_id;
    double value;
    uint32_t status;
} mesh_delta_pkt_t;

typedef struct {
    uint32_t node_id;
    uint64_t merkle_root;
    uint32_t tag_count;
} merkle_tree_node_t;

typedef struct {
    uint32_t local_node_id;
    datahub_t *hub;
    uint64_t next_seq;
} edge_mesh_t;

int edge_mesh_init(edge_mesh_t *mesh, uint32_t local_node_id, datahub_t *hub);
int edge_mesh_pack_delta(edge_mesh_t *mesh, uint32_t tag_id, double val, uint64_t mtt_ts, mesh_delta_pkt_t *out_pkt);
int edge_mesh_process_remote_delta(edge_mesh_t *mesh, const mesh_delta_pkt_t *pkt);
uint64_t edge_mesh_compute_merkle_root(edge_mesh_t *mesh);
int edge_mesh_reconcile(edge_mesh_t *local_mesh, uint64_t remote_merkle_root);

#ifdef __cplusplus
}
#endif

#endif // EDGE_MESH_H
