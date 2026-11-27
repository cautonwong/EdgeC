int rest_write_tag(unsigned tag_id, double value) {
    return edge_api_write_tag(0, "ModbusRTU", 1, tag_id, value, NULL, NULL);
}

int rest_raw_io(void *tx) {
    return edge_api_raw_io(0, tx, NULL);
}
