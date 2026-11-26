#include "dcu_comm_info.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dcu_698_object_model.h"
#include "dcu_router.h"
#include "dlt698_codec.h"
#include "edge_slave_plane.h"

#define COMM_NET_SYSFS "/sys/class/net"

static dcu_comm_info_t g_info;

static int _hex_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static bool _parse_mac(const char *s, uint8_t mac[6]) {
    if (strlen(s) < 17) return false;
    for (size_t i = 0; i < 6; i++) {
        int hi = _hex_nibble(s[i * 3]);
        int lo = _hex_nibble(s[i * 3 + 1]);
        if (hi < 0 || lo < 0) return false;
        if (i < 5 && s[i * 3 + 2] != ':') return false;
        mac[i] = (uint8_t)((hi << 4) | lo);
    }
    return s[17] == '\n' || s[17] == '\0';
}

static int _read_small(const char *path, char *buf, size_t cap) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;
    ssize_t n = read(fd, buf, cap - 1);
    close(fd);
    if (n <= 0) return -1;
    buf[n] = '\0';
    return 0;
}

static bool _wired_name(const char *ifname) {
    /* 物理以太网命名（eth*、systemd 的 en*）；无线与虚拟口由 _ethernet_mac 的
     * sysfs 判据排除，这里只做名字预筛。 */
    return strncmp(ifname, "eth", 3) == 0 || strncmp(ifname, "en", 2) == 0;
}

static bool _ethernet_mac(const char *net_root, const char *ifname, uint8_t mac[6]) {
    char path[256];
    char buf[64];

    snprintf(path, sizeof(path), "%s/%s/type", net_root, ifname);
    if (_read_small(path, buf, sizeof(buf)) != 0) return false;
    /* ARPHRD_ETHER=1；无线口、bridge、veth 也是 1，下面继续按 sysfs 判据排除。 */
    if (buf[0] != '1' || (buf[1] != '\n' && buf[1] != '\0')) return false;

    /* 无线口的 MAC 报成以太网口 MAC 是错误归属。 */
    snprintf(path, sizeof(path), "%s/%s/wireless", net_root, ifname);
    if (access(path, F_OK) == 0) return false;

    /* Linux bridge 也是 type=1，名字还可能以 en/eth 开头。 */
    snprintf(path, sizeof(path), "%s/%s/bridge", net_root, ifname);
    if (access(path, F_OK) == 0) return false;

    /* 真实网卡有总线 device 链接；veth/bridge/bond/VLAN 等虚拟口没有。 */
    snprintf(path, sizeof(path), "%s/%s/device", net_root, ifname);
    if (access(path, F_OK) != 0) return false;

    snprintf(path, sizeof(path), "%s/%s/address", net_root, ifname);
    if (_read_small(path, buf, sizeof(buf)) != 0) return false;
    if (!_parse_mac(buf, mac)) return false;

    /* I/G 位为 1 是组播/广播地址，不是接口的单播 MAC。 */
    if (mac[0] & 0x01) return false;

    uint8_t nonzero = 0;
    for (size_t i = 0; i < 6; i++) nonzero = (uint8_t)(nonzero | mac[i]);
    return nonzero != 0;
}

int dcu_comm_info_scan(const char *net_root, dcu_comm_info_t *out) {
    if (!out) return 0;
    memset(out, 0, sizeof(*out));
    if (!net_root) return 0;

    /* shortcut: 4510 先取 eth0，否则按接口名字典序取最小的物理以太网口；板卡多口按
     * 板卡配置来源再映射（eth2/eth10 端口序号排序暂不实现）。 */
    uint8_t mac[6];
    if (_ethernet_mac(net_root, "eth0", mac)) {
        out->mac_present = true;
        memcpy(out->mac, mac, sizeof(mac));
        return 0;
    }

    DIR *dir = opendir(net_root);
    if (!dir) return 0;

    char best[256] = {0}; /* name_max 255 + NUL；dirent.d_name 最多 256 字节 */
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        if (!_wired_name(entry->d_name)) continue;
        if (strcmp(entry->d_name, "eth0") == 0) continue;
        if (strlen(entry->d_name) >= sizeof(best)) continue;
        if (!_ethernet_mac(net_root, entry->d_name, mac)) continue;
        if (best[0] == '\0' || strcmp(entry->d_name, best) < 0) {
            snprintf(best, sizeof(best), "%s", entry->d_name);
            out->mac_present = true;
            memcpy(out->mac, mac, sizeof(mac));
        }
    }
    closedir(dir);
    return 0;
}

int dcu_comm_info_read(dcu_comm_info_t *out) {
    return dcu_comm_info_scan(COMM_NET_SYSFS, out);
}

void dcu_comm_info_install(const dcu_comm_info_t *info) {
    if (info) {
        g_info = *info;
    } else {
        memset(&g_info, 0, sizeof(g_info));
    }
}

void dcu_comm_info_reset(void) {
    dcu_comm_info_install(NULL);
}

int dcu_comm_info_encode_axdr(uint32_t oad, uint8_t *buf, size_t cap, size_t *len) {
    if (!buf || !len) return EDGE_SLAVE_SRC_TYPE_MISMATCH;

    uint16_t oi = (uint16_t)(oad >> 16);
    uint8_t attr_id = (uint8_t)((oad >> 8) & 0xFF);
    bool wireless = oi >= OI_WIRELESS_COMM_FIRST && oi <= OI_WIRELESS_COMM_LAST;
    bool ethernet = oi >= OI_ETHERNET_COMM_FIRST && oi <= OI_ETHERNET_COMM_LAST;
    if (!wireless && !ethernet) return EDGE_SLAVE_SRC_NOT_FOUND;

    if (oi == OI_ETHERNET_COMM_FIRST && attr_id == 5 && g_info.mac_present) {
        if (cap < 2 + sizeof(g_info.mac)) return EDGE_SLAVE_SRC_TYPE_MISMATCH;
        buf[0] = DLT698_TAG_OCTET_STRING;
        buf[1] = (uint8_t)sizeof(g_info.mac);
        memcpy(buf + 2, g_info.mac, sizeof(g_info.mac));
        *len = 2 + sizeof(g_info.mac);
        return 0;
    }

    uint8_t class_id = wireless ? IC_WIRELESS_PUBLIC : IC_ETHERNET;
    return dcu_698_class_defines_attr(class_id, attr_id) ? EDGE_SLAVE_SRC_CLASS_MISMATCH
                                                         : EDGE_SLAVE_SRC_UNDEFINED;
}
