/**
 * @file test_snapshot_csv.c
 * @brief Byte-level snapshot CSV vectors, rotation, backup marker and quota.
 */

#define _GNU_SOURCE

#include <cmocka.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "edge_digest.h"
#include "edge_snapshot_csv.h"
#include "snapshot_csv_adapter.h"

#define TEST_PATH_MAX 512
#define TEST_MARKER ".edgeos-snapshot-complete"

static const edge_snapshot_column_t kColumns[4] = {
    {"voltage", EDGE_SNAPSHOT_CELL_FLOAT},
    {"count", EDGE_SNAPSHOT_CELL_INT},
    {"reserved", EDGE_SNAPSHOT_CELL_EMPTY},
    {"P(kW)", EDGE_SNAPSHOT_CELL_FLOAT},
};

static const char kHeader[] =
    "\xEF\xBB\xBF"
    "time,voltage,count,reserved,P(kW)\n";

static void reset_seams(void) {
    edge_snapshot_retention_set_usage_percent(0);
    edge_snapshot_retention_set_force_copy(0);
}

static void rm_rf(const char *path) {
    if (unlink(path) == 0 || errno == ENOENT) return;
    if (errno != EISDIR && errno != EPERM) return;
    int dirfd = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (dirfd < 0) return;
    DIR *dir = fdopendir(dirfd);
    if (!dir) {
        (void)close(dirfd);
        return;
    }
    struct dirent *entry = NULL;
    char child[TEST_PATH_MAX];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        int len = snprintf(child, sizeof(child), "%s/%s", path, entry->d_name);
        if (len > 0 && (size_t)len < sizeof(child)) rm_rf(child);
    }
    closedir(dir);
    (void)rmdir(path);
}

static char *temp_root(void) {
    char template[] = "test_snapshot_csv.XXXXXX";
    char *made = mkdtemp(template);
    assert_non_null(made);
    char *copy = strdup(made);
    assert_non_null(copy);
    return copy;
}

static void join(char *out, size_t size, const char *a, const char *b) {
    int len = snprintf(out, size, "%s/%s", a, b);
    assert_true(len > 0 && (size_t)len < size);
}

static void make_dir(const char *path) {
    assert_int_equal(mkdir(path, 0755), 0);
}

static void write_file(const char *path, const void *data, size_t len) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert_true(fd >= 0);
    const unsigned char *bytes = data;
    size_t offset = 0;
    while (offset < len) {
        ssize_t written = write(fd, bytes + offset, len - offset);
        assert_true(written > 0);
        offset += (size_t)written;
    }
    assert_int_equal(close(fd), 0);
}

static void append_file(const char *path, const void *data, size_t len) {
    int fd = open(path, O_WRONLY | O_APPEND);
    assert_true(fd >= 0);
    const unsigned char *bytes = data;
    size_t offset = 0;
    while (offset < len) {
        ssize_t written = write(fd, bytes + offset, len - offset);
        assert_true(written > 0);
        offset += (size_t)written;
    }
    assert_int_equal(close(fd), 0);
}

static unsigned char *read_file(const char *path, size_t *len) {
    int fd = open(path, O_RDONLY);
    assert_true(fd >= 0);
    struct stat st;
    assert_int_equal(fstat(fd, &st), 0);
    assert_true(st.st_size >= 0);
    size_t size = (size_t)st.st_size;
    unsigned char *data = malloc(size ? size : 1);
    assert_non_null(data);
    size_t offset = 0;
    while (offset < size) {
        ssize_t got = read(fd, data + offset, size - offset);
        assert_true(got > 0);
        offset += (size_t)got;
    }
    assert_int_equal(close(fd), 0);
    *len = size;
    return data;
}

static void assert_file_bytes(const char *path, const void *expected, size_t len) {
    size_t actual_len = 0;
    unsigned char *actual = read_file(path, &actual_len);
    assert_int_equal(actual_len, len);
    assert_memory_equal(actual, expected, len);
    free(actual);
}

static void assert_missing(const char *path) {
    struct stat st;
    assert_int_equal(lstat(path, &st), -1);
    assert_int_equal(errno, ENOENT);
}

static struct tm tm_at(int year, int month, int day, int hour, int minute, int second) {
    struct tm value;
    memset(&value, 0, sizeof(value));
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_hour = hour;
    value.tm_min = minute;
    value.tm_sec = second;
    return value;
}

static void open_columns(const char *local, const struct tm *now, const char *stem,
                         const edge_snapshot_column_t *columns, size_t count,
                         edge_snapshot_writer_port_t *port, char *reason, size_t reason_size) {
    assert_int_equal(
        snapshot_csv_writer_open(local, now, stem, columns, count, port, reason, reason_size),
        EDGE_HISTORY_OK);
}

static void open_default(const char *local, const struct tm *now, const char *stem,
                         edge_snapshot_writer_port_t *port, char *reason, size_t reason_size) {
    open_columns(local, now, stem, kColumns, 4, port, reason, reason_size);
}

static void write_marker_file(const char *dir, const char *name, const char *sha_override) {
    char path[TEST_PATH_MAX];
    char marker[TEST_PATH_MAX];
    char sha[65];
    unsigned long long size = 0;
    join(path, sizeof(path), dir, name);
    assert_int_equal(edge_sha256_file(path, sha, &size), 0);
    if (sha_override) {
        snprintf(sha, sizeof(sha), "%s", sha_override);
        struct stat st;
        assert_int_equal(stat(path, &st), 0);
        size = (unsigned long long)st.st_size;
    }
    join(marker, sizeof(marker), dir, TEST_MARKER);
    static const char header[] = "edgeos-snapshot/1\n";
    write_file(marker, header, sizeof(header) - 1);
    char line[TEST_PATH_MAX];
    int len = snprintf(line, sizeof(line), "%s %llu %s\n", sha, size, name);
    assert_true(len > 0 && (size_t)len < sizeof(line));
    append_file(marker, line, (size_t)len);
}

static void make_marked_dir(const char *backup, const char *date) {
    char dir[TEST_PATH_MAX];
    char file[TEST_PATH_MAX];
    join(dir, sizeof(dir), backup, date);
    make_dir(dir);
    join(file, sizeof(file), dir, "d.csv");
    write_file(file, date, strlen(date));
    write_marker_file(dir, "d.csv", NULL);
}

static void set_dir_mtime(const char *path, time_t seconds) {
    struct timespec times[2];
    times[0].tv_sec = seconds;
    times[0].tv_nsec = 0;
    times[1] = times[0];
    assert_int_equal(utimensat(AT_FDCWD, path, times, 0), 0);
}

static edge_snapshot_retention_policy_t policy_at(const char *local, const char *backup,
                                                  uint32_t keep, uint32_t threshold,
                                                  uint32_t max_delete) {
    edge_snapshot_retention_policy_t policy;
    memset(&policy, 0, sizeof(policy));
    policy.local_root = local;
    policy.backup_root = backup;
    policy.keep_local_days = keep;
    policy.usage_threshold_percent = threshold;
    policy.max_delete_dirs = max_delete;
    return policy;
}

static void test_new_file_bytes(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_writer_port_t port;
    open_default(local, &now, "dev1", &port, reason, sizeof(reason));
    edge_snapshot_cell_t cells[4];
    memset(cells, 0, sizeof(cells));
    cells[0].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[0].real = 230.5;
    cells[1].kind = EDGE_SNAPSHOT_CELL_INT;
    cells[1].integer = 42;
    cells[2].kind = EDGE_SNAPSHOT_CELL_EMPTY;
    cells[3].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[3].real = -0.0;
    assert_int_equal(port.append(port.ctx, &now, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(port.flush(port.ctx, reason, sizeof(reason)), EDGE_HISTORY_OK);
    snapshot_csv_writer_close(&port);
    const char expected[] =
        "\xEF\xBB\xBF"
        "time,voltage,count,reserved,P(kW)\n"
        "2026-11-23 12:00:15, 230.50,42, , -0.00\n";
    char path[TEST_PATH_MAX];
    char date_dir[TEST_PATH_MAX];
    join(date_dir, sizeof(date_dir), local, "2026-11-23");
    join(path, sizeof(path), date_dir, "dev1.csv");
    assert_file_bytes(path, expected, sizeof(expected) - 1);
    rm_rf(root);
    free(root);
}

static void test_append_existing(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    struct tm first = tm_at(2026, 11, 23, 12, 0, 15);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_writer_port_t port;
    open_default(local, &first, "dev1", &port, reason, sizeof(reason));
    edge_snapshot_cell_t cells[4];
    memset(cells, 0, sizeof(cells));
    cells[0].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[0].real = 230.5;
    cells[1].kind = EDGE_SNAPSHOT_CELL_INT;
    cells[1].integer = 42;
    cells[2].kind = EDGE_SNAPSHOT_CELL_EMPTY;
    cells[3].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[3].real = -0.0;
    assert_int_equal(port.append(port.ctx, &first, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    snapshot_csv_writer_close(&port);

    struct tm second = tm_at(2026, 11, 23, 12, 0, 16);
    open_default(local, &second, "dev1", &port, reason, sizeof(reason));
    cells[0].real = 231.0;
    cells[1].integer = 43;
    cells[3].real = 1.25;
    assert_int_equal(port.append(port.ctx, &second, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    snapshot_csv_writer_close(&port);
    const char expected[] =
        "\xEF\xBB\xBF"
        "time,voltage,count,reserved,P(kW)\n"
        "2026-11-23 12:00:15, 230.50,42, , -0.00\n"
        "2026-11-23 12:00:16, 231.00,43, , 1.25\n";
    char path[TEST_PATH_MAX];
    char date_dir[TEST_PATH_MAX];
    join(date_dir, sizeof(date_dir), local, "2026-11-23");
    join(path, sizeof(path), date_dir, "dev1.csv");
    assert_file_bytes(path, expected, sizeof(expected) - 1);
    rm_rf(root);
    free(root);
}

static void test_header_escape(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_column_t columns[3] = {
        {"a,b", EDGE_SNAPSHOT_CELL_FLOAT},
        {"q\"uote", EDGE_SNAPSHOT_CELL_INT},
        {"", EDGE_SNAPSHOT_CELL_EMPTY},
    };
    edge_snapshot_writer_port_t port;
    open_columns(local, &now, "dev1", columns, 3, &port, reason, sizeof(reason));
    snapshot_csv_writer_close(&port);
    const char expected[] =
        "\xEF\xBB\xBF"
        "time,\"a,b\",\"q\"\"uote\",\n";
    char path[TEST_PATH_MAX];
    char date_dir[TEST_PATH_MAX];
    join(date_dir, sizeof(date_dir), local, "2026-11-23");
    join(path, sizeof(path), date_dir, "dev1.csv");
    assert_file_bytes(path, expected, sizeof(expected) - 1);

    edge_snapshot_column_t bad[1] = {{"bad\n", EDGE_SNAPSHOT_CELL_INT}};
    edge_snapshot_writer_port_t rejected;
    assert_int_equal(
        snapshot_csv_writer_open(local, &now, "dev1", bad, 1, &rejected, reason, sizeof(reason)),
        EDGE_HISTORY_INVALID);
    assert_file_bytes(path, expected, sizeof(expected) - 1);
    const char *bad_stems[] = {"../x", "a/b", ".hidden", ".", ".."};
    for (size_t i = 0; i < sizeof(bad_stems) / sizeof(bad_stems[0]); i++) {
        assert_int_equal(snapshot_csv_writer_open(local, &now, bad_stems[i], kColumns, 4, &rejected,
                                                  reason, sizeof(reason)),
                         EDGE_HISTORY_INVALID);
    }
    rm_rf(root);
    free(root);
}

static void test_half_line_repair(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    char path[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    join(dir, sizeof(dir), local, "2026-11-23");
    make_dir(dir);
    join(path, sizeof(path), dir, "dev1.csv");
    const char first[] = "2026-11-23 12:00:15, 230.50,42, , -0.00\n";
    char seeded[TEST_PATH_MAX];
    int len =
        snprintf(seeded, sizeof(seeded), "%s%s%s", kHeader, first, "2026-11-23 12:00:16, 231");
    assert_true(len > 0 && (size_t)len < sizeof(seeded));
    write_file(path, seeded, (size_t)len);

    struct tm now = tm_at(2026, 11, 23, 12, 0, 16);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_writer_port_t port;
    open_default(local, &now, "dev1", &port, reason, sizeof(reason));
    snapshot_csv_writer_close(&port);
    char repaired[TEST_PATH_MAX];
    len = snprintf(repaired, sizeof(repaired), "%s%s", kHeader, first);
    assert_true(len > 0 && (size_t)len < sizeof(repaired));
    assert_file_bytes(path, repaired, (size_t)len);

    open_default(local, &now, "dev1", &port, reason, sizeof(reason));
    edge_snapshot_cell_t cells[4];
    memset(cells, 0, sizeof(cells));
    cells[0].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[0].real = 231.0;
    cells[1].kind = EDGE_SNAPSHOT_CELL_INT;
    cells[1].integer = 43;
    cells[2].kind = EDGE_SNAPSHOT_CELL_EMPTY;
    cells[3].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[3].real = 1.25;
    assert_int_equal(port.append(port.ctx, &now, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    snapshot_csv_writer_close(&port);
    const char second[] = "2026-11-23 12:00:16, 231.00,43, , 1.25\n";
    len = snprintf(repaired, sizeof(repaired), "%s%s%s", kHeader, first, second);
    assert_true(len > 0 && (size_t)len < sizeof(repaired));
    assert_file_bytes(path, repaired, (size_t)len);
    rm_rf(root);
    free(root);
}

static void test_truncated_header_repair(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    char path[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    join(dir, sizeof(dir), local, "2026-11-23");
    make_dir(dir);
    join(path, sizeof(path), dir, "dev1.csv");
    write_file(path, kHeader, 10);
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_writer_port_t port;
    open_default(local, &now, "dev1", &port, reason, sizeof(reason));
    snapshot_csv_writer_close(&port);
    assert_file_bytes(path, kHeader, sizeof(kHeader) - 1);

    open_default(local, &now, "dev1", &port, reason, sizeof(reason));
    edge_snapshot_cell_t cells[4];
    memset(cells, 0, sizeof(cells));
    cells[0].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[0].real = 230.5;
    cells[1].kind = EDGE_SNAPSHOT_CELL_INT;
    cells[1].integer = 42;
    cells[2].kind = EDGE_SNAPSHOT_CELL_EMPTY;
    cells[3].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[3].real = -0.0;
    assert_int_equal(port.append(port.ctx, &now, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    snapshot_csv_writer_close(&port);
    const char expected[] =
        "\xEF\xBB\xBF"
        "time,voltage,count,reserved,P(kW)\n"
        "2026-11-23 12:00:15, 230.50,42, , -0.00\n";
    assert_file_bytes(path, expected, sizeof(expected) - 1);
    rm_rf(root);
    free(root);
}

static void test_header_mismatch(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    char path[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    join(dir, sizeof(dir), local, "2026-11-23");
    make_dir(dir);
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_writer_port_t port;

    const char other[] =
        "\xEF\xBB\xBF"
        "time,other\n";
    join(path, sizeof(path), dir, "dev1.csv");
    write_file(path, other, sizeof(other) - 1);
    assert_int_equal(
        snapshot_csv_writer_open(local, &now, "dev1", kColumns, 4, &port, reason, sizeof(reason)),
        EDGE_HISTORY_CONFLICT);
    assert_file_bytes(path, other, sizeof(other) - 1);

    const char no_bom[] = "time,voltage,count,reserved,P(kW)\n";
    join(path, sizeof(path), dir, "dev2.csv");
    write_file(path, no_bom, sizeof(no_bom) - 1);
    assert_int_equal(
        snapshot_csv_writer_open(local, &now, "dev2", kColumns, 4, &port, reason, sizeof(reason)),
        EDGE_HISTORY_CORRUPT);
    assert_file_bytes(path, no_bom, sizeof(no_bom) - 1);

    const char partial_other[] =
        "\xEF\xBB\xBF"
        "time,ot";
    join(path, sizeof(path), dir, "dev3.csv");
    write_file(path, partial_other, sizeof(partial_other) - 1);
    assert_int_equal(
        snapshot_csv_writer_open(local, &now, "dev3", kColumns, 4, &port, reason, sizeof(reason)),
        EDGE_HISTORY_CONFLICT);
    assert_file_bytes(path, partial_other, sizeof(partial_other) - 1);
    rm_rf(root);
    free(root);
}

static void test_archived_dir_rejected(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    char marker[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    join(dir, sizeof(dir), local, "2026-11-23");
    make_dir(dir);
    join(marker, sizeof(marker), dir, TEST_MARKER);
    write_file(marker, "edgeos-snapshot/1\n", 18);
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_writer_port_t port;
    assert_int_equal(
        snapshot_csv_writer_open(local, &now, "dev1", kColumns, 4, &port, reason, sizeof(reason)),
        EDGE_HISTORY_CONFLICT);
    char csv[TEST_PATH_MAX];
    join(csv, sizeof(csv), dir, "dev1.csv");
    assert_missing(csv);
    assert_file_bytes(marker, "edgeos-snapshot/1\n", 18);
    rm_rf(root);
    free(root);
}

static void test_cell_validation(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    char path[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    join(dir, sizeof(dir), local, "2026-11-23");
    make_dir(dir);
    join(path, sizeof(path), dir, "dev1.csv");
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_writer_port_t port;
    open_default(local, &now, "dev1", &port, reason, sizeof(reason));
    edge_snapshot_cell_t cells[4];
    memset(cells, 0, sizeof(cells));
    cells[0].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[0].real = 230.5;
    cells[1].kind = EDGE_SNAPSHOT_CELL_INT;
    cells[1].integer = 42;
    cells[2].kind = EDGE_SNAPSHOT_CELL_EMPTY;
    cells[3].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[3].real = -0.0;
    assert_int_equal(port.append(port.ctx, &now, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(port.flush(port.ctx, reason, sizeof(reason)), EDGE_HISTORY_OK);
    assert_int_equal(port.append(port.ctx, &now, cells, 3, reason, sizeof(reason)),
                     EDGE_HISTORY_INVALID);
    assert_int_equal(port.append(port.ctx, &now, NULL, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_INVALID);
    cells[1].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    assert_int_equal(port.append(port.ctx, &now, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_INVALID);
    cells[1].kind = EDGE_SNAPSHOT_CELL_INT;
    struct tm tomorrow = tm_at(2026, 11, 24, 12, 0, 15);
    assert_int_equal(port.append(port.ctx, &tomorrow, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_INVALID);
    snapshot_csv_writer_close(&port);
    const char expected[] =
        "\xEF\xBB\xBF"
        "time,voltage,count,reserved,P(kW)\n"
        "2026-11-23 12:00:15, 230.50,42, , -0.00\n";
    assert_file_bytes(path, expected, sizeof(expected) - 1);
    rm_rf(root);
    free(root);
}

static void test_fail_after_rollback(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    char path[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    join(dir, sizeof(dir), local, "2026-11-23");
    make_dir(dir);
    join(path, sizeof(path), dir, "dev1.csv");
    struct tm first = tm_at(2026, 11, 23, 12, 0, 15);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_writer_port_t port;
    open_default(local, &first, "dev1", &port, reason, sizeof(reason));
    edge_snapshot_cell_t cells[4];
    memset(cells, 0, sizeof(cells));
    cells[0].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[0].real = 230.5;
    cells[1].kind = EDGE_SNAPSHOT_CELL_INT;
    cells[1].integer = 42;
    cells[2].kind = EDGE_SNAPSHOT_CELL_EMPTY;
    cells[3].kind = EDGE_SNAPSHOT_CELL_FLOAT;
    cells[3].real = -0.0;
    assert_int_equal(port.append(port.ctx, &first, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(port.flush(port.ctx, reason, sizeof(reason)), EDGE_HISTORY_OK);
    struct stat before;
    assert_int_equal(stat(path, &before), 0);
    unsigned long long payload = (unsigned long long)before.st_size - (sizeof(kHeader) - 1);

    struct tm second = tm_at(2026, 11, 23, 12, 0, 16);
    cells[0].real = 231.0;
    cells[1].integer = 43;
    cells[3].real = 1.25;
    snapshot_csv_writer_set_fail_after(&port, payload + 5);
    assert_int_equal(port.append(port.ctx, &second, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_NO_SPACE);
    struct stat after;
    assert_int_equal(stat(path, &after), 0);
    assert_int_equal(after.st_size, before.st_size);
    size_t len = 0;
    unsigned char *bytes = read_file(path, &len);
    assert_int_equal(bytes[len - 1], '\n');
    free(bytes);

    snapshot_csv_writer_set_fail_after(&port, 0);
    assert_int_equal(port.append(port.ctx, &second, cells, 4, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    snapshot_csv_writer_close(&port);
    const char expected[] =
        "\xEF\xBB\xBF"
        "time,voltage,count,reserved,P(kW)\n"
        "2026-11-23 12:00:15, 230.50,42, , -0.00\n"
        "2026-11-23 12:00:16, 231.00,43, , 1.25\n";
    assert_file_bytes(path, expected, sizeof(expected) - 1);
    rm_rf(root);
    free(root);
}

static void test_float_format(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    char path[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    join(dir, sizeof(dir), local, "2026-11-23");
    make_dir(dir);
    join(path, sizeof(path), dir, "dev1.csv");
    edge_snapshot_column_t column[1] = {{"v", EDGE_SNAPSHOT_CELL_FLOAT}};
    struct tm now = tm_at(2026, 11, 23, 12, 0, 0);
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_writer_port_t port;
    open_columns(local, &now, "dev1", column, 1, &port, reason, sizeof(reason));
    double values[] = {230.5, -0.0, 0.0, 1.005, 123456.789};
    char expected[TEST_PATH_MAX];
    size_t offset = 0;
    const char header[] =
        "\xEF\xBB\xBF"
        "time,v\n";
    memcpy(expected + offset, header, sizeof(header) - 1);
    offset += sizeof(header) - 1;
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        struct tm row = tm_at(2026, 11, 23, 12, 0, (int)i);
        edge_snapshot_cell_t cell;
        memset(&cell, 0, sizeof(cell));
        cell.kind = EDGE_SNAPSHOT_CELL_FLOAT;
        cell.real = values[i];
        assert_int_equal(port.append(port.ctx, &row, &cell, 1, reason, sizeof(reason)),
                         EDGE_HISTORY_OK);
        int len = snprintf(expected + offset, sizeof(expected) - offset,
                           "2026-11-23 12:00:%02zu, %.2f\n", i, values[i]);
        assert_true(len > 0 && (size_t)len < sizeof(expected) - offset);
        offset += (size_t)len;
    }
    snapshot_csv_writer_close(&port);
    assert_file_bytes(path, expected, offset);
    rm_rf(root);
    free(root);
}

static void test_rotation_move(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char backup[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    join(backup, sizeof(backup), root, "backup");
    make_dir(local);
    make_dir(backup);
    const char *dates[] = {"2026-11-21", "2026-11-22", "2026-11-23"};
    for (size_t i = 0; i < 3; i++) {
        char dir[TEST_PATH_MAX];
        char file[TEST_PATH_MAX];
        join(dir, sizeof(dir), local, dates[i]);
        make_dir(dir);
        join(file, sizeof(file), dir, "dev1.csv");
        char content[32];
        int len = snprintf(content, sizeof(content), "csv-%s", dates[i]);
        write_file(file, content, (size_t)len);
    }
    char old_file[TEST_PATH_MAX];
    char old_dir[TEST_PATH_MAX];
    join(old_dir, sizeof(old_dir), local, dates[0]);
    join(old_file, sizeof(old_file), old_dir, "dev1.csv");
    char stale_tmp[TEST_PATH_MAX];
    join(stale_tmp, sizeof(stale_tmp), old_dir, ".edgeos-snapshot-complete.tmp");
    write_file(stale_tmp, "stale", 5);
    struct stat before;
    assert_int_equal(stat(old_file, &before), 0);
    size_t old_len = 0;
    unsigned char *old_bytes = read_file(old_file, &old_len);
    char sha[65];
    unsigned long long hash_size = 0;
    assert_int_equal(edge_sha256_file(old_file, sha, &hash_size), 0);

    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, backup, 0, 0, 0);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(result.moved_dirs, 2);
    assert_int_equal(result.deduped_local_dirs, 0);
    assert_int_equal(result.conflicts, 0);
    assert_int_equal(result.failed_dirs, 0);
    assert_missing(old_dir);
    char second_dir[TEST_PATH_MAX];
    join(second_dir, sizeof(second_dir), local, dates[1]);
    assert_missing(second_dir);
    char today_dir[TEST_PATH_MAX];
    join(today_dir, sizeof(today_dir), local, dates[2]);
    struct stat st;
    assert_int_equal(stat(today_dir, &st), 0);

    char backup_dir[TEST_PATH_MAX];
    char backup_file[TEST_PATH_MAX];
    join(backup_dir, sizeof(backup_dir), backup, dates[0]);
    join(backup_file, sizeof(backup_file), backup_dir, "dev1.csv");
    assert_file_bytes(backup_file, old_bytes, old_len);
    struct stat moved;
    assert_int_equal(stat(backup_file, &moved), 0);
    assert_int_equal(moved.st_mtim.tv_sec, before.st_mtim.tv_sec);
    assert_int_equal(moved.st_mtim.tv_nsec, before.st_mtim.tv_nsec);
    free(old_bytes);

    char marker[TEST_PATH_MAX];
    join(marker, sizeof(marker), backup_dir, TEST_MARKER);
    char expected_marker[TEST_PATH_MAX];
    int len = snprintf(expected_marker, sizeof(expected_marker),
                       "edgeos-snapshot/1\n%s %llu dev1.csv\n", sha, hash_size);
    assert_true(len > 0 && (size_t)len < sizeof(expected_marker));
    assert_file_bytes(marker, expected_marker, (size_t)len);
    char marker_tmp[TEST_PATH_MAX];
    join(marker_tmp, sizeof(marker_tmp), backup_dir, ".edgeos-snapshot-complete.tmp");
    assert_missing(marker_tmp);
    size_t marker_len = 0;
    unsigned char *marker_bytes = read_file(marker, &marker_len);

    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(result.moved_dirs, 0);
    assert_int_equal(result.deduped_local_dirs, 0);
    assert_file_bytes(marker, marker_bytes, marker_len);
    free(marker_bytes);
    rm_rf(root);
    free(root);
}

static void test_backup_dedupe_and_conflict(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char backup[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    join(backup, sizeof(backup), root, "backup");
    make_dir(local);
    make_dir(backup);
    const char *same = "same-bytes";
    const char *local_only = "local-bytes";
    const char *backup_only = "backup-bytes";
    char local_dir[TEST_PATH_MAX];
    char backup_dir[TEST_PATH_MAX];
    char file[TEST_PATH_MAX];
    join(local_dir, sizeof(local_dir), local, "2026-11-21");
    make_dir(local_dir);
    join(file, sizeof(file), local_dir, "dev1.csv");
    write_file(file, same, strlen(same));
    join(backup_dir, sizeof(backup_dir), backup, "2026-11-21");
    make_dir(backup_dir);
    join(file, sizeof(file), backup_dir, "dev1.csv");
    write_file(file, same, strlen(same));

    join(local_dir, sizeof(local_dir), local, "2026-11-22");
    make_dir(local_dir);
    join(file, sizeof(file), local_dir, "dev1.csv");
    write_file(file, local_only, strlen(local_only));
    struct stat local_before;
    assert_int_equal(stat(file, &local_before), 0);
    join(backup_dir, sizeof(backup_dir), backup, "2026-11-22");
    make_dir(backup_dir);
    join(file, sizeof(file), backup_dir, "dev1.csv");
    write_file(file, backup_only, strlen(backup_only));

    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, backup, 0, 0, 0);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_CONFLICT);
    assert_int_equal(result.deduped_local_dirs, 1);
    assert_int_equal(result.conflicts, 1);
    join(local_dir, sizeof(local_dir), local, "2026-11-21");
    assert_missing(local_dir);
    join(local_dir, sizeof(local_dir), local, "2026-11-22");
    struct stat kept;
    assert_int_equal(stat(local_dir, &kept), 0);
    join(file, sizeof(file), local_dir, "dev1.csv");
    assert_file_bytes(file, local_only, strlen(local_only));
    struct stat local_after;
    assert_int_equal(stat(file, &local_after), 0);
    assert_int_equal(local_after.st_mtim.tv_sec, local_before.st_mtim.tv_sec);
    assert_int_equal(local_after.st_mtim.tv_nsec, local_before.st_mtim.tv_nsec);
    join(backup_dir, sizeof(backup_dir), backup, "2026-11-22");
    join(file, sizeof(file), backup_dir, "dev1.csv");
    assert_file_bytes(file, backup_only, strlen(backup_only));
    rm_rf(root);
    free(root);
}

static void test_backup_unavailable(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    join(dir, sizeof(dir), local, "2026-11-21");
    make_dir(dir);
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];

    edge_snapshot_retention_policy_t policy = policy_at(local, "", 0, 0, 0);
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(result.backup_unavailable, 1);
    assert_int_equal(result.moved_dirs, 0);
    struct stat st;
    assert_int_equal(stat(dir, &st), 0);

    char missing[TEST_PATH_MAX];
    join(missing, sizeof(missing), root, "missing");
    policy = policy_at(local, missing, 0, 0, 0);
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_IO);
    assert_int_equal(result.backup_unavailable, 1);
    assert_int_equal(result.moved_dirs, 0);
    assert_int_equal(stat(dir, &st), 0);
    rm_rf(root);
    free(root);
}

static void test_same_root_rejected(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    join(dir, sizeof(dir), local, "2026-11-21");
    make_dir(dir);
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, local, 0, 0, 0);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_INVALID);
    struct stat st;
    assert_int_equal(stat(dir, &st), 0);
    rm_rf(root);
    free(root);
}

static void test_unknown_dirs_untouched(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char backup[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    join(backup, sizeof(backup), root, "backup");
    make_dir(local);
    make_dir(backup);
    const char *unknown[] = {"compressed", "misc", "2026-12-31"};
    for (size_t i = 0; i < 3; i++) {
        char dir[TEST_PATH_MAX];
        join(dir, sizeof(dir), local, unknown[i]);
        make_dir(dir);
    }
    char today_dir[TEST_PATH_MAX];
    join(today_dir, sizeof(today_dir), local, "2026-11-23");
    make_dir(today_dir);

    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, backup, 0, 0, 0);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(result.moved_dirs, 0);
    assert_int_equal(result.skipped_unknown, 3);
    for (size_t i = 0; i < 3; i++) {
        char dir[TEST_PATH_MAX];
        struct stat st;
        join(dir, sizeof(dir), local, unknown[i]);
        assert_int_equal(stat(dir, &st), 0);
    }
    struct stat st;
    assert_int_equal(stat(today_dir, &st), 0);
    rm_rf(root);
    free(root);
}

static void test_force_copy(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char backup[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    join(backup, sizeof(backup), root, "backup");
    make_dir(local);
    make_dir(backup);
    char local_dir[TEST_PATH_MAX];
    char file[TEST_PATH_MAX];
    join(local_dir, sizeof(local_dir), local, "2026-11-21");
    make_dir(local_dir);
    join(file, sizeof(file), local_dir, "dev1.csv");
    const char content[] = "csv-for-copy";
    write_file(file, content, sizeof(content) - 1);
    char sha[65];
    unsigned long long size = 0;
    assert_int_equal(edge_sha256_file(file, sha, &size), 0);

    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, backup, 0, 0, 0);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_retention_set_force_copy(1);
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(result.moved_dirs, 1);
    assert_missing(local_dir);
    char backup_dir[TEST_PATH_MAX];
    char backup_file[TEST_PATH_MAX];
    join(backup_dir, sizeof(backup_dir), backup, "2026-11-21");
    join(backup_file, sizeof(backup_file), backup_dir, "dev1.csv");
    assert_file_bytes(backup_file, content, sizeof(content) - 1);
    char marker[TEST_PATH_MAX];
    join(marker, sizeof(marker), backup_dir, TEST_MARKER);
    char expected_marker[TEST_PATH_MAX];
    int len = snprintf(expected_marker, sizeof(expected_marker),
                       "edgeos-snapshot/1\n%s %llu dev1.csv\n", sha, size);
    assert_true(len > 0 && (size_t)len < sizeof(expected_marker));
    assert_file_bytes(marker, expected_marker, (size_t)len);

    char stale[TEST_PATH_MAX];
    join(stale, sizeof(stale), backup, ".tmp-2026-11-20-999");
    make_dir(stale);
    join(file, sizeof(file), stale, "junk");
    write_file(file, "junk", 4);
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_missing(stale);
    assert_int_equal(result.moved_dirs, 0);
    edge_snapshot_retention_set_force_copy(0);
    rm_rf(root);
    free(root);
}

static void test_quota_order(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char backup[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    join(backup, sizeof(backup), root, "backup");
    make_dir(local);
    make_dir(backup);
    time_t base = 1764000000;
    for (int i = 1; i <= 12; i++) {
        char date[16];
        snprintf(date, sizeof(date), "2026-11-%02d", i);
        make_marked_dir(backup, date);
        char dir[TEST_PATH_MAX];
        join(dir, sizeof(dir), backup, date);
        set_dir_mtime(dir, base - (time_t)i * 3600);
    }
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, backup, 0, 90, 10);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_retention_set_usage_percent(91);
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_backup_dirs, 10);
    assert_int_equal(result.usage_known, 1);
    assert_int_equal(result.usage_percent_after, 91);
    assert_int_equal(result.still_over_threshold, 1);
    for (int i = 1; i <= 10; i++) {
        char date[16];
        char dir[TEST_PATH_MAX];
        snprintf(date, sizeof(date), "2026-11-%02d", i);
        join(dir, sizeof(dir), backup, date);
        assert_missing(dir);
    }
    for (int i = 11; i <= 12; i++) {
        char date[16];
        char dir[TEST_PATH_MAX];
        struct stat st;
        snprintf(date, sizeof(date), "2026-11-%02d", i);
        join(dir, sizeof(dir), backup, date);
        assert_int_equal(stat(dir, &st), 0);
    }
    rm_rf(root);
    free(root);
}

static void test_quota_stop(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char backup[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    join(backup, sizeof(backup), root, "backup");
    make_dir(local);
    make_dir(backup);
    for (int i = 1; i <= 12; i++) {
        char date[16];
        snprintf(date, sizeof(date), "2026-11-%02d", i);
        make_marked_dir(backup, date);
    }
    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, backup, 0, 90, 10);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_retention_set_usage_percent(80);
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_backup_dirs, 0);
    assert_int_equal(result.usage_percent_after, 80);
    assert_int_equal(result.still_over_threshold, 0);
    edge_snapshot_retention_set_usage_percent(91);
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_int_equal(result.deleted_backup_dirs, 10);
    assert_int_equal(result.still_over_threshold, 1);
    rm_rf(root);
    free(root);
}

static void test_quota_verification(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char backup[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    join(backup, sizeof(backup), root, "backup");
    make_dir(local);
    make_dir(backup);

    char dir[TEST_PATH_MAX];
    char file[TEST_PATH_MAX];
    join(dir, sizeof(dir), backup, "2026-11-05");
    make_dir(dir);
    join(file, sizeof(file), dir, "d.csv");
    write_file(file, "no-marker", 9);

    join(dir, sizeof(dir), backup, "2026-11-06");
    make_dir(dir);
    join(file, sizeof(file), dir, "d.csv");
    write_file(file, "bad-hash", 8);
    const char *zeros = "0000000000000000000000000000000000000000000000000000000000000000";
    write_marker_file(dir, "d.csv", zeros);

    make_marked_dir(backup, "2026-11-07");
    make_marked_dir(backup, "2026-11-23");
    make_marked_dir(backup, "2026-11-24");

    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, backup, 0, 90, 10);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_retention_set_usage_percent(91);
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_CONFLICT);
    assert_int_equal(result.deleted_backup_dirs, 1);
    assert_int_equal(result.conflicts, 1);
    assert_true(result.skipped_unknown >= 1);
    join(dir, sizeof(dir), backup, "2026-11-07");
    assert_missing(dir);
    const char *kept[] = {"2026-11-05", "2026-11-06", "2026-11-23", "2026-11-24"};
    for (size_t i = 0; i < sizeof(kept) / sizeof(kept[0]); i++) {
        struct stat st;
        join(dir, sizeof(dir), backup, kept[i]);
        assert_int_equal(stat(dir, &st), 0);
    }
    rm_rf(root);
    free(root);
}

static void test_quota_interrupted_cleanup(void **state) {
    (void)state;
    reset_seams();
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char backup[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    join(backup, sizeof(backup), root, "backup");
    make_dir(local);
    make_dir(backup);
    char stale[TEST_PATH_MAX];
    char file[TEST_PATH_MAX];
    join(stale, sizeof(stale), backup, ".deleting-2026-11-01-555");
    make_dir(stale);
    join(file, sizeof(file), stale, "inner");
    write_file(file, "inner", 5);

    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, backup, 0, 90, 10);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_retention_set_usage_percent(80);
    assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                     EDGE_HISTORY_OK);
    assert_missing(stale);
    assert_int_equal(result.deleted_backup_dirs, 0);
    assert_int_equal(result.still_over_threshold, 0);
    rm_rf(root);
    free(root);
}

static void assert_non_regular_marker_case(int use_fifo) {
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char backup[TEST_PATH_MAX];
    char dir[TEST_PATH_MAX];
    char file[TEST_PATH_MAX];
    char marker[TEST_PATH_MAX];
    char target[TEST_PATH_MAX];
    join(local, sizeof(local), root, "local");
    join(backup, sizeof(backup), root, "backup");
    join(target, sizeof(target), root, "marker-target");
    make_dir(local);
    make_dir(backup);
    write_file(target, "target", sizeof("target") - 1);

    make_marked_dir(backup, "2026-11-06");
    make_marked_dir(backup, "2026-11-07");
    join(dir, sizeof(dir), backup, "2026-11-05");
    make_dir(dir);
    join(file, sizeof(file), dir, "d.csv");
    write_file(file, "backup-csv", sizeof("backup-csv") - 1);
    join(marker, sizeof(marker), dir, TEST_MARKER);
    if (use_fifo) {
        assert_int_equal(mkfifo(marker, 0644), 0);
    } else {
        assert_int_equal(symlink(target, marker), 0);
    }

    join(dir, sizeof(dir), local, "2026-11-20");
    make_dir(dir);
    join(file, sizeof(file), dir, "d.csv");
    write_file(file, "moved-csv", sizeof("moved-csv") - 1);
    join(dir, sizeof(dir), local, "2026-11-21");
    make_dir(dir);
    join(file, sizeof(file), dir, "d.csv");
    write_file(file, "kept-csv", sizeof("kept-csv") - 1);
    join(marker, sizeof(marker), dir, TEST_MARKER);
    if (use_fifo) {
        assert_int_equal(mkfifo(marker, 0644), 0);
    } else {
        assert_int_equal(symlink(target, marker), 0);
    }

    struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
    edge_snapshot_retention_policy_t policy = policy_at(local, backup, 0, 90, 2);
    edge_snapshot_retention_result_t result;
    char reason[EDGE_HISTORY_REASON_SIZE];
    edge_snapshot_retention_set_usage_percent(91);
    /* A blocking marker open must trip SIGALRM instead of hanging the suite. */
    alarm(10);
    edge_history_result_t rc =
        edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason));
    alarm(0);
    assert_int_equal(rc, EDGE_HISTORY_CONFLICT);
    assert_int_equal(result.conflicts, 2);
    assert_int_equal(result.deleted_backup_dirs, 2);
    assert_int_equal(result.moved_dirs, 1);

    struct stat st;
    join(dir, sizeof(dir), backup, "2026-11-05");
    assert_int_equal(stat(dir, &st), 0);
    assert_true(S_ISDIR(st.st_mode));
    join(dir, sizeof(dir), backup, "2026-11-06");
    assert_missing(dir);
    join(dir, sizeof(dir), backup, "2026-11-07");
    assert_missing(dir);
    join(dir, sizeof(dir), local, "2026-11-21");
    assert_int_equal(stat(dir, &st), 0);
    join(dir, sizeof(dir), local, "2026-11-20");
    assert_missing(dir);
    join(dir, sizeof(dir), backup, "2026-11-20");
    assert_int_equal(stat(dir, &st), 0);

    rm_rf(root);
    free(root);
}

static void test_fifo_marker_no_block(void **state) {
    (void)state;
    reset_seams();
    assert_non_regular_marker_case(1);
}

static void test_symlink_marker(void **state) {
    (void)state;
    reset_seams();
    assert_non_regular_marker_case(0);
}

static void test_invalid_local_time(void **state) {
    (void)state;
    char *root = temp_root();
    char local[TEST_PATH_MAX];
    char reason[EDGE_HISTORY_REASON_SIZE];
    join(local, sizeof(local), root, "local");
    make_dir(local);
    const int years[] = {INT_MAX, INT_MIN, 8100}; /* 8100 == year 10000, above 9999 */
    for (size_t i = 0; i < sizeof(years) / sizeof(years[0]); i++) {
        struct tm now = tm_at(2026, 11, 23, 12, 0, 15);
        now.tm_year = years[i];
        edge_snapshot_writer_port_t port;
        assert_int_equal(snapshot_csv_writer_open(local, &now, "dev1", kColumns, 4, &port, reason,
                                                  sizeof(reason)),
                         EDGE_HISTORY_INVALID);
        assert_string_equal(reason, "local time is invalid");
        edge_snapshot_retention_policy_t policy = policy_at(local, NULL, 0, 0, 0);
        edge_snapshot_retention_result_t result;
        assert_int_equal(edge_snapshot_maintain(&now, &policy, &result, reason, sizeof(reason)),
                         EDGE_HISTORY_INVALID);
        assert_string_equal(reason, "local time is invalid");
    }
    rm_rf(root);
    free(root);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_new_file_bytes),
        cmocka_unit_test(test_append_existing),
        cmocka_unit_test(test_header_escape),
        cmocka_unit_test(test_half_line_repair),
        cmocka_unit_test(test_truncated_header_repair),
        cmocka_unit_test(test_header_mismatch),
        cmocka_unit_test(test_archived_dir_rejected),
        cmocka_unit_test(test_cell_validation),
        cmocka_unit_test(test_fail_after_rollback),
        cmocka_unit_test(test_float_format),
        cmocka_unit_test(test_rotation_move),
        cmocka_unit_test(test_backup_dedupe_and_conflict),
        cmocka_unit_test(test_backup_unavailable),
        cmocka_unit_test(test_same_root_rejected),
        cmocka_unit_test(test_unknown_dirs_untouched),
        cmocka_unit_test(test_force_copy),
        cmocka_unit_test(test_quota_order),
        cmocka_unit_test(test_quota_stop),
        cmocka_unit_test(test_quota_verification),
        cmocka_unit_test(test_quota_interrupted_cleanup),
        cmocka_unit_test(test_invalid_local_time),
        cmocka_unit_test(test_fifo_marker_no_block),
        cmocka_unit_test(test_symlink_marker),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
