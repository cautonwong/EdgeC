#include "edge_mesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "edge_log.h"

int edge_mesh_init(edge_mesh_t *mesh, uint32_t local_node_id, datahub_t *hub) {
    if (!mesh || !hub) return -1;
    memset(mesh, 0, sizeof(edge_mesh_t));
    mesh->local_node_id = local_node_id;
    mesh->hub = hub;
    mesh->next_seq = 1;
    LOG_INFO(LOG_CAT_CORE, "[Mesh] Node %u initialized mesh state", local_node_id);
    return 0;
}

int edge_mesh_pack_delta(edge_mesh_t *mesh, uint32_t tag_id, double val, uint64_t mtt_ts, mesh_delta_pkt_t *out_pkt) {
    if (!mesh || !out_pkt) return -1;
    out_pkt->magic = EDGE_MESH_MAGIC;
    out_pkt->node_id = mesh->local_node_id;
    out_pkt->sequence_id = mesh->next_seq++;
    out_pkt->mtt_timestamp = mtt_ts;
    out_pkt->tag_id = tag_id;
    out_pkt->value = val;
    out_pkt->status = 0;
    return 0;
}

int edge_mesh_process_remote_delta(edge_mesh_t *mesh, const mesh_delta_pkt_t *pkt) {
    if (!mesh || !pkt || pkt->magic != EDGE_MESH_MAGIC || !mesh->hub) return -1;

    // 自动判定节点自身包过滤
    if (pkt->node_id == mesh->local_node_id) return 0;

    // CRDT / Last-Write-Wins with MTT Authority 物理时间戳对比裁决
    pipe_msg_t existing;
    int index = datahub_read_shadow(mesh->hub, pkt->tag_id, &existing);
    if (index >= 0) {
        if (existing.ts > pkt->mtt_timestamp) {
            LOG_DEBUG(LOG_CAT_CORE, "[Mesh] Drop stale delta pkt (ts=%llu < local=%llu)",
                      (unsigned long long)pkt->mtt_timestamp, (unsigned long long)existing.ts);
            return 0;
        }
    }

    // 物理写入 DataHub 影子表
    pipe_msg_t msg = {
        .tag_id = pkt->tag_id,
        .ts = pkt->mtt_timestamp,
        .type = PIPEMSG_TYPE_SCALAR,
        .payload = {.val = {.f = pkt->value}}
    };
    datahub_write_shadow(mesh->hub, &msg);
    LOG_DEBUG(LOG_CAT_CORE, "[Mesh] Applied CRDT delta for tag %u = %f from Node %u",
              pkt->tag_id, pkt->value, pkt->node_id);
    return 0;
}

uint64_t edge_mesh_compute_merkle_root(edge_mesh_t *mesh) {
    if (!mesh || !mesh->hub || !mesh->hub->shadow_table) return 0;

    uint64_t hash = 14695981039346656037ULL; // FNV-1a offset basis
    uint32_t count = mesh->hub->shadow_capacity;

    for (uint32_t i = 0; i < count; i++) {
        pipe_msg_t *msg = &mesh->hub->shadow_table[i];
        if (msg->tag_id != 0) {
            uint64_t val_bits;
            memcpy(&val_bits, &msg->payload.val.f, sizeof(double));
            hash ^= (uint64_t)msg->tag_id;
            hash *= 1099511628211ULL;
            hash ^= val_bits;
            hash *= 1099511628211ULL;
            hash ^= msg->ts;
            hash *= 1099511628211ULL;
        }
    }
    return hash;
}

int edge_mesh_reconcile(edge_mesh_t *local_mesh, uint64_t remote_merkle_root) {
    if (!local_mesh) return -1;
    uint64_t local_root = edge_mesh_compute_merkle_root(local_mesh);
    if (local_root == remote_merkle_root) {
        return 0; // 完全一致
    }
    LOG_WARN(LOG_CAT_CORE, "[Mesh] Merkle root mismatch (local=%llx != remote=%llx), triggering partial sync",
             (unsigned long long)local_root, (unsigned long long)remote_merkle_root);
    return 1;
}
