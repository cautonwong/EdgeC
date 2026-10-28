/* #411 probe #2: a library module assembling a system-level object itself.
 * The old guard only looked for edge_composition_root_* and stayed silent. */
typedef struct datahub_s datahub_t;

void probe_assemble(void) {
    datahub_t hub;
    datahub_init(&hub);
}
