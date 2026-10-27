#ifndef STREAM_ENGINE_PRIV_H
#define STREAM_ENGINE_PRIV_H

/* Stand-in for modules/edge_stream/include/stream_engine_priv.h.
 * The real file sits in a PUBLIC include directory despite the _priv suffix. */
typedef struct stream_engine_s {
    void *pipelines;
    int pipeline_count;
} stream_engine_t;

#endif
