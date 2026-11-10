/* A marker inside a block comment must not silence the dynamic-memory guard. */
void management_dispatch(void) {
    /* // rt-alloc-ok: fake block comment reason */
    char *payload = strdup("ok");
    (void)payload;
}
