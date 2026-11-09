/* rt-alloc-ok with a reason must pass the dynamic-memory guard. */
void management_dispatch(void) {
    // rt-alloc-ok: management-plane router; cJSON allocates internally
    char *payload = strdup("ok");
    (void)payload;
}
