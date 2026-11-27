void rest_endpoint(void) {
    edge_api_write_tag_ex(0, "ModbusRTU", 1, 2, 3.0, NULL, 0, NULL);
}
