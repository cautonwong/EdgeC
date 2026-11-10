/* A bare stack-ok marker must not silence the guard. */
void unbounded_scratch(void) {
    char buffer[1024]; // stack-ok:
    buffer[0] = 0;
}
