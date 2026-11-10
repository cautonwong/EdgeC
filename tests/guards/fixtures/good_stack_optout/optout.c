/* A stack-ok marker with a reason must pass the guard. */
void bounded_scratch(void) {
    char buffer[1024]; // stack-ok: bounded one-shot path scratch, not an RT callback
    buffer[0] = 0;
}
