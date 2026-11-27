int execute_write(unsigned native_id, double value) {
    return edge_api_write_tag_ex(0, "ModbusRTU", 1, native_id, value, NULL, 0, NULL);
}
