/* Fixture: the execution adapter is the one legal caller in cmd/. */
extern int edge_api_write_tag(int bus, const char *proto, unsigned slave, unsigned reg,
                              double value, void *cb, void *ctx);
int iems_exec_fixture(int bus, unsigned reg, double value) {
    return edge_api_write_tag(bus, "ModbusRTU", 1, reg, value, 0, 0);
}
