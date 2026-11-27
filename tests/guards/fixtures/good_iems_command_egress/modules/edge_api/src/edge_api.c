int edge_api_write_tag_ex(int bus_id, const char *protocol, unsigned slave_id,
                          unsigned native_id, double value, const char *desc,
                          unsigned desc_len, void *cb) {
    (void)bus_id; (void)protocol; (void)slave_id; (void)native_id;
    (void)value; (void)desc; (void)desc_len; (void)cb;
    return 0;
}

int edge_api_write_tag(int bus_id, const char *protocol, unsigned slave_id,
                       unsigned native_id, double value, const char *desc,
                       unsigned desc_len, void *cb) {
    return edge_api_write_tag_ex(bus_id, protocol, slave_id, native_id, value,
                                 desc, desc_len, cb);
}

int edge_api_raw_io(int bus_id, void *tx_buf, void *cb) {
    (void)bus_id; (void)tx_buf; (void)cb;
    return 0;
}
