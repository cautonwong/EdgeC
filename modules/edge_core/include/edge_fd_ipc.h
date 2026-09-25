/**
 * @file edge_fd_ipc.h
 * @brief UNIX Domain Socket File Descriptor Passing IPC Helper
 */

#ifndef EDGE_FD_IPC_H
#define EDGE_FD_IPC_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Send a file descriptor across a UNIX domain socket
 * @param sock_fd UNIX domain socket file descriptor
 * @param fd_to_send File descriptor to transfer
 * @return 0 on success, -1 on error
 */
int edge_fd_send(int sock_fd, int fd_to_send);

/**
 * @brief Receive a file descriptor from a UNIX domain socket
 * @param sock_fd UNIX domain socket file descriptor
 * @return Received file descriptor >= 0 on success, -1 on error
 */
int edge_fd_recv(int sock_fd);

#ifdef __cplusplus
}
#endif

#endif /* EDGE_FD_IPC_H */
