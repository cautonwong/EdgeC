#include "edge_sos_dscp.h"
#include <sys/socket.h>
#include <netinet/in.h>

int edge_sos_set_socket_dscp(int sockfd) {
    if (sockfd < 0) return -1;
    int tos = SOS_DSCP_EF_TOS;
    return setsockopt(sockfd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
}

bool edge_sos_is_emergency_event(uint32_t tag_id, double val) {
    // Tag 12001: Emergency SOS button, Tag 12002: Fall alert
    if (tag_id == 12001 && val >= 1.0) {
        return true;
    }
    if (tag_id == 12002 && val >= 0.85) {
        return true;
    }
    return false;
}
