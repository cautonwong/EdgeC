/**
 * @file edge_wasm_imports.h
 * @brief Import whitelist for WebAssembly modules loaded by the app platform.
 *
 * The validator reads the import section of a module and accepts only the host
 * exports the runtime registers under the "env" module. Everything else is
 * rejected before the module reaches the engine.
 */
#ifndef EDGE_WASM_IMPORTS_H
#define EDGE_WASM_IMPORTS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EDGE_WASM_IMPORT_REASON_SIZE 160

/**
 * Validate every import of the module. Returns 0 when the module is valid and
 * imports only whitelisted host functions, -1 with a reason otherwise:
 * "invalid_wasm" for a malformed module, "denied_import: <module>.<name>" for a
 * disallowed import.
 */
int edge_wasm_imports_validate(const void *wasm, size_t len, char *reason, size_t reason_size);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_WASM_IMPORTS_H */
