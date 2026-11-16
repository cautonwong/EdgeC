static void handle_frame(app_host_server_t *srv, cJSON *root) {
    cJSON *fn = cJSON_GetObjectItemCaseSensitive(root, "fn");
    uint64_t id = 0;
    if (strcmp(fn->valuestring, "point_resolve") == 0) {
        handle_resolve(srv, id, root);
    } else if (strcmp(fn->valuestring, "point_read") == 0) {
        handle_read(srv, id, root);
    } else if (strcmp(fn->valuestring, "point_publish") == 0) {
        handle_publish(srv, id, root);
    } else if (strcmp(fn->valuestring, "log") == 0) {
        handle_log(srv, id, root);
    } else if (strcmp(fn->valuestring, "now") == 0) {
        handle_now(srv, id);
    } else if (strcmp(fn->valuestring, "point_subscribe") == 0) {
        send_error(srv, id, "not_supported");
    } else if (strcmp(fn->valuestring, "setting_get") == 0 ||
               strcmp(fn->valuestring, "setting_watch") == 0 ||
               strcmp(fn->valuestring, "kv_get") == 0 ||
               strcmp(fn->valuestring, "kv_set") == 0) {
        send_error(srv, id, "not_supported");
    } else {
        send_error(srv, id, "invalid");
    }
}
