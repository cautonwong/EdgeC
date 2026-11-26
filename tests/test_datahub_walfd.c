/**
 * @file test_datahub_walfd.c
 * @brief A zeroed hub must not treat stdin (fd 0) as WAL/shadow storage.
 */

#include <cmocka.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include "datahub_priv.h"
#include "datahub_shm.h"

static void make_name(char *out, size_t size, const char *tag) {
    snprintf(out, size, "walfd_%s_%d", tag, (int)getpid());
}

static void cleanup_shm(const char *name) {
    char shadow[256];
    char lock[272];
    char fifo[288];
    snprintf(shadow, sizeof(shadow), "/edgeos_shadow_%s", name);
    snprintf(lock, sizeof(lock), "%s.lock", shadow);
    snprintf(fifo, sizeof(fifo), "/tmp/edgeos_ev_%s", name);
    shm_unlink(shadow);
    shm_unlink(lock);
    unlink(fifo);
}

static int init_hub(datahub_t *hub, const char *name) {
    memset(hub, 0, sizeof(*hub));
    datahub_config_t config;
    memset(&config, 0, sizeof(config));
    config.backend_type = DATAHUB_BACKEND_SHM;
    snprintf(config.shm_name, sizeof(config.shm_name), "%s", name);
    config.shadow_capacity = 4;
    config.ring_slots = 8;
    return datahub_shm_init(hub, &config);
}

static int write_one(datahub_t *hub) {
    pipe_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.tag_id = 3;
    msg.type = PIPEMSG_TYPE_SCALAR;
    msg.payload.val.f = 1.5f;
    return datahub_write_shadow_idx(hub, 3, &msg);
}

/* fd 0 is a pipe: any WAL write lands in the pipe instead of stdin. */
static void test_canary_pipe_stdin(void **state) {
    (void)state;
    char name[64];
    make_name(name, sizeof(name), "canary");
    cleanup_shm(name);

    /* A daemon-like start may already have fd 0 closed; the canary still works. */
    int saved_stdin = dup(STDIN_FILENO);
    assert_true(saved_stdin >= 0 || errno == EBADF);
    int pipe_fds[2];
    assert_int_equal(pipe(pipe_fds), 0);
    assert_int_equal(dup2(pipe_fds[1], STDIN_FILENO), STDIN_FILENO);
    close(pipe_fds[1]);

    datahub_t hub;
    assert_int_equal(init_hub(&hub, name), 0);
    assert_int_equal(write_one(&hub), 0);
    datahub_shm_destroy(&hub);

    /* Closing fd 0 ends the pipe, so the read below sees EOF after any bytes. */
    if (saved_stdin >= 0) {
        assert_int_equal(dup2(saved_stdin, STDIN_FILENO), STDIN_FILENO);
        close(saved_stdin);
    } else {
        close(STDIN_FILENO);
    }
    char buf[128];
    ssize_t total = 0;
    ssize_t n;
    while ((n = read(pipe_fds[0], buf, sizeof(buf))) > 0) {
        total += n;
    }
    close(pipe_fds[0]);
    assert_int_equal(total, 0);

    cleanup_shm(name);
}

/* Child's stdin is a socket whose peer the parent closes before the append.
 * A WAL write to fd 0 would raise SIGPIPE and kill the child. */
static void test_socket_stdin_no_sigpipe(void **state) {
    (void)state;
    char name[64];
    make_name(name, sizeof(name), "socket");
    cleanup_shm(name);

    int sv[2];
    int sync_pipe[2];
    int ack_pipe[2];
    assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, sv), 0);
    assert_int_equal(pipe(sync_pipe), 0);
    assert_int_equal(pipe(ack_pipe), 0);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        close(sync_pipe[0]);
        close(ack_pipe[1]);
        close(sv[0]);
        if (dup2(sv[1], STDIN_FILENO) != STDIN_FILENO) _exit(10);
        close(sv[1]);
        signal(SIGPIPE, SIG_DFL);

        datahub_t hub;
        if (init_hub(&hub, name) != 0) _exit(11);
        if (write(sync_pipe[1], "x", 1) != 1) _exit(12);
        char ack;
        if (read(ack_pipe[0], &ack, 1) != 1) _exit(13);

        if (write_one(&hub) != 0) _exit(14);
        datahub_shm_destroy(&hub);
        _exit(0);
    }

    close(sync_pipe[1]);
    close(ack_pipe[0]);
    close(sv[1]);
    char byte;
    assert_int_equal(read(sync_pipe[0], &byte, 1), 1);
    close(sv[0]);
    assert_int_equal(write(ack_pipe[1], "y", 1), 1);
    close(ack_pipe[1]);

    int status = 0;
    assert_int_equal(waitpid(pid, &status, 0), pid);
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);
    close(sync_pipe[0]);
    cleanup_shm(name);
}

/* fd 0 is closed before init, so the first open inside init reuses it.
 * Init must still mark the fd fields -1 rather than adopt that descriptor. */
static void test_closed_stdin_child(void **state) {
    (void)state;
    char name[64];
    make_name(name, sizeof(name), "closed");
    cleanup_shm(name);

    pid_t pid = fork();
    assert_true(pid >= 0);
    if (pid == 0) {
        if (close(STDIN_FILENO) != 0 && errno != EBADF) _exit(20);
        datahub_t hub;
        if (init_hub(&hub, name) != 0) _exit(21);
        if (hub.wal_fd >= 0) _exit(22);
        if (write_one(&hub) != 0) _exit(23);
        datahub_shm_destroy(&hub);
        _exit(0);
    }

    int status = 0;
    assert_int_equal(waitpid(pid, &status, 0), pid);
    assert_true(WIFEXITED(status));
    assert_int_equal(WEXITSTATUS(status), 0);
    cleanup_shm(name);
}

/* A failed WAL write disables the WAL instead of retrying a broken target. */
static void test_wal_write_failure_disables(void **state) {
    (void)state;
    char name[64];
    make_name(name, sizeof(name), "walfail");
    cleanup_shm(name);

    datahub_t hub;
    assert_int_equal(init_hub(&hub, name), 0);
    assert_int_equal(datahub_enable_wal(&hub, "/dev/full"), 0);
    assert_true(hub.wal_fd >= 0);

    assert_int_equal(write_one(&hub), 0);
    assert_int_equal(hub.wal_fd, -1);
    assert_int_equal(atomic_load_explicit(&hub.wal_write_failures, memory_order_relaxed), 1);

    assert_int_equal(write_one(&hub), 0);
    assert_int_equal(atomic_load_explicit(&hub.wal_write_failures, memory_order_relaxed), 1);

    datahub_shm_destroy(&hub);
    cleanup_shm(name);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_canary_pipe_stdin),
        cmocka_unit_test(test_socket_stdin_no_sigpipe),
        cmocka_unit_test(test_closed_stdin_child),
        cmocka_unit_test(test_wal_write_failure_disables),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
