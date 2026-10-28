/* #411 probe #5: Foundation reaching into Compute's private header.
 * The old rule only matched qualified spellings like "edge_stream/...", so an
 * unqualified include of a private header slipped through. */
#include "stream_engine_priv.h"

void probe_leak(void) {}
