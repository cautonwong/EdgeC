#ifndef EDGE_SOS_DSCP_H
#define EDGE_SOS_DSCP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// DSCP EF (Expedited Forwarding = 46 -> ToS 0xB8)
#define SOS_DSCP_EF_TOS 0xB8

int edge_sos_set_socket_dscp(int sockfd);
bool edge_sos_is_emergency_event(uint32_t tag_id, double val);

#ifdef __cplusplus
}
#endif

#endif // EDGE_SOS_DSCP_H
