/* Fixture: a product file bypasses the aggregate root's execution port. */
extern int edge_api_write_tag(int bus, const char *proto, unsigned slave, unsigned reg,
                              double value, void *cb, void *ctx);
int bypass_fixture(int bus, unsigned reg, double value) {
    return edge_api_write_tag(bus, "ModbusRTU", 1, reg, value, 0, 0);
}
