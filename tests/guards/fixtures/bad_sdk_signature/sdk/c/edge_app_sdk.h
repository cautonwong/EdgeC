const char *edge_app_point_resolve(edge_app_sdk_t *sdk, const char *name,
                                   uint32_t *handle, const char **access);
const char *edge_app_point_read(edge_app_sdk_t *sdk, uint64_t handle,
                                edge_app_sample_t *sample);
const char *edge_app_point_subscribe(edge_app_sdk_t *sdk, uint32_t handle, double deadband,
                                     uint32_t min_interval_ms, uint32_t max_interval_ms,
                                     uint32_t *subscription);
const char *edge_app_point_publish(edge_app_sdk_t *sdk, uint32_t handle,
                                   const edge_app_sample_t *sample);
const char *edge_app_log(edge_app_sdk_t *sdk, const char *level, const char *message);
const char *edge_app_now(edge_app_sdk_t *sdk, uint64_t *now_ms);
