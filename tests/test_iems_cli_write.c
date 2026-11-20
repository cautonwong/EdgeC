/* Release 构建带 -DNDEBUG，assert() 会被抹成空语句 → 测试变空壳 */
#undef NDEBUG
/**
 * @file test_iems_cli_write.c
 * @brief iems-cli `tag write` must reach the attached DataHub through a real
 *        process and reject what the static point table does not declare.
 *
 * DataHub 有值但静态元数据表外，MCP/CLI 写入只认 iems_get_tag_meta 的声明：
 * 4242 预先在 DataHub 里写有值，`tag write 4242` 仍必须被拒（-32000），
 * 且不能改动该槽位。
 */

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "datahub.h"
#include "iems_driver_adapter.h"

static const char *g_cli;

static int run_cli(char *const argv[], char *out, size_t out_size) {
    int fds[2];
    assert(pipe(fds) == 0);
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        dup2(fds[1], STDERR_FILENO);
        close(fds[1]);
        execv(g_cli, argv);
        _exit(127);
    }

    close(fds[1]);
    size_t used = 0;
    ssize_t n;
    while ((n = read(fds[0], out + used, out_size - 1 - used)) > 0) {
        used += (size_t)n;
        if (used == out_size - 1) {
            char discard[256];
            while (read(fds[0], discard, sizeof(discard)) > 0) {
            }
            break;
        }
    }
    out[used] = '\0';
    close(fds[0]);

    int status = 0;
    assert(waitpid(pid, &status, 0) == pid);
    assert(WIFEXITED(status));
    return WEXITSTATUS(status);
}

static int run_tag_write(const char *tag, const char *value, char *out, size_t out_size) {
    char *argv[] = {"iems-cli", "tag", "write", (char *)tag, (char *)value, NULL};
    return run_cli(argv, out, out_size);
}

static void test_write_rejects_tag_with_datahub_value_but_no_metadata(datahub_t *datahub, char *out,
                                                                      size_t out_size) {
    pipe_msg_t sentinel = {0};
    pipe_msg_init_scalar(&sentinel, 4242, 77.0, 1000);
    assert(datahub_write_shadow(datahub, &sentinel) == 0);

    int rc = run_tag_write("4242", "7", out, out_size);
    printf("[tag write 4242 7] rc=%d\n%s\n", rc, out);
    assert(rc != 0);
    assert(strstr(out, "-32000") != NULL);

    pipe_msg_t after = {0};
    assert(datahub_read_shadow(datahub, 4242, &after) == 0);
    assert(fabs(after.payload.val.f - 77.0) < 1e-3);
    printf("  --> DataHub value at 4242 untouched (%.3f)\n", after.payload.val.f);
}

int main(void) {
    g_cli = getenv("IEMS_CLI");
    assert(g_cli && g_cli[0]);

    uv_loop_t loop;
    assert(uv_loop_init(&loop) == 0);

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name), "iems_cli_write_%d", (int)getpid());
    char shm_path[128];
    snprintf(shm_path, sizeof(shm_path), "/edgeos_shadow_%s", shm_name);
    shm_unlink(shm_path);

    datahub_storage_t storage;
    datahub_t *datahub = (datahub_t *)&storage;
    /* 8192 槽：静态表外 4242 也落在 DataHub 容量内，拒绝只能来自元数据声明。 */
    assert(datahub_init(datahub, &loop, shm_name, 64, 8192, NULL, 0, NULL, NULL, NULL) == 0);
    datahub_set_tag_mapper_v2(datahub, iems_tag_map_fn);
    assert(setenv("EDGE_SHM_NAME", shm_name, 1) == 0);

    char out[4096];

    /* 1003 是非轮询点；CLI 写入的槽位必须能被产品侧按同一映射读回。 */
    int rc = run_tag_write("1003", "42", out, sizeof(out));
    printf("[tag write 1003 42] rc=%d\n%s\n", rc, out);
    assert(rc == 0);
    pipe_msg_t msg = {0};
    assert(datahub_read_shadow(datahub, 1003, &msg) == 0);
    assert(fabs(msg.payload.val.f - 42.0) < 1e-3);
    printf("  --> shadow 1003 reads back %.3f\n", msg.payload.val.f);

    rc = run_tag_write("9999", "7", out, sizeof(out));
    printf("[tag write 9999 7] rc=%d\n%s\n", rc, out);
    assert(rc != 0);
    assert(strstr(out, "-32000") != NULL);

    test_write_rejects_tag_with_datahub_value_but_no_metadata(datahub, out, sizeof(out));

    /* 没有可 attach 的段就没有写端口：必须报错退出，不能静默成功。 */
    char missing[64];
    snprintf(missing, sizeof(missing), "iems_cli_write_missing_%d", (int)getpid());
    assert(setenv("EDGE_SHM_NAME", missing, 1) == 0);
    rc = run_tag_write("1003", "42", out, sizeof(out));
    printf("[tag write without DataHub] rc=%d\n%s\n", rc, out);
    assert(rc != 0);
    assert(strstr(out, "Failed to attach") != NULL);

    datahub_destroy(datahub);
    shm_unlink(shm_path);
    while (uv_run(&loop, UV_RUN_NOWAIT) != 0) {
    }
    assert(uv_loop_close(&loop) == 0);
    printf("iems-cli tag write DataHub port OK\n");
    return 0;
}
