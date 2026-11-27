void dcu_write(unsigned tag_id, double value) {
    edge_api_write_tag(0, "ModbusRTU", 1, tag_id, value, NULL, NULL);
}
