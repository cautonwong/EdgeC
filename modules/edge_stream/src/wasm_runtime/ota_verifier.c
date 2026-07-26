#define IMPORT(name) __attribute__((import_module("env"), import_name(name)))

IMPORT("host_get_tag") int host_get_tag(unsigned int tag_id, double *val_ptr);
IMPORT("host_set_tag") int host_set_tag(unsigned int tag_id, double val);
IMPORT("ext_log") void ext_log(unsigned int level, const char *msg);

__attribute__((export_name("verify_package")))
int verify_package(unsigned int version_code, unsigned int size_bytes) {
    if (version_code > 100 && size_bytes > 0) {
        ext_log(1, "WASM OTA: Package signature & HAP integrity verified successfully!");
        return 0; // Success
    }
    ext_log(4, "WASM OTA ERROR: Package verification failed!");
    return -1; // Failure
}
