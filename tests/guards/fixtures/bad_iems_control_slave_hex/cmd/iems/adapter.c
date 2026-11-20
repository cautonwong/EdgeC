int write_one(int bus_id, const char *proto, unsigned native_id, double raw) {
    return edge_api_write_tag(bus_id, proto, 0x1u, native_id, raw, 0, 0);
}
