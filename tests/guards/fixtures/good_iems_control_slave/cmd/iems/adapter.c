/*
 * 注释里的旧调用不算违规，扫描前必须先去注释。
 * edge_api_write_tag(bus_id, proto, 1, native_id, raw, NULL, NULL);
 */
int write_one(int bus_id, const char *proto, unsigned slave_id, unsigned native_id, double raw) {
    return edge_api_write_tag(bus_id, proto, slave_id, native_id, raw, 0, 0);
}
