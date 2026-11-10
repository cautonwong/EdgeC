/* A bare rt-alloc-ok marker must not silence the dynamic-memory guard. */
void management_dispatch(void) {
    // rt-alloc-ok:
    char *payload = strdup("ok");
    (void)payload;
}
