#ifndef WEB_SERVER_H
#define WEB_SERVER_H

typedef struct uv_loop_s uv_loop_t;

// Opaque handle
typedef struct web_server_s web_server_t;

/**
 * @brief Open a request path as a file under doc_root.
 *
 * The descriptor comes from walking request_path one component at a time with
 * openat(O_NOFOLLOW) from a directory fd for doc_root, so no symlink component
 * can redirect the open. Only origin-form paths ("/...") that name a regular
 * file under doc_root are accepted.
 *
 * @return A read-only fd on success, -1 when the path is rejected or missing.
 */
int web_server_open_path(const char *doc_root, const char *request_path);

/**
 * @brief API Handler callback type
 * @param user_data User context
 * @param path Request path
 * @param body Request body (if any)
 * @return JSON response (caller will free)
 */
typedef char *(*web_api_handler_pt)(void *user_data, const char *path, const char *body);

/**
 * @brief Init the Static Web Server (HMI).
 *
 * @param loop The loop to run on (provided by Northbound Mgr).
 * @param port The port to listen on (e.g., 8080).
 * @param doc_root Path to the index.html to serve.
 */
web_server_t *web_server_create(uv_loop_t *loop, int port, const char *doc_root);
/** Actual bound port; pass 0 to create() to get an ephemeral port. */
int web_server_port(const web_server_t *srv);

/**
 * @brief Set API handler to enable /api routing on web port
 */
void web_server_set_api_handler(web_server_t *srv, web_api_handler_pt handler, void *user_data);

void web_server_destroy(web_server_t *srv);

#endif
