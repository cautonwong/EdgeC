/**
 * @file snapshot_csv_adapter.c
 * @brief Snapshot CSV writer, date-directory rotation and backup quota.
 */

#define _GNU_SOURCE
#include "snapshot_csv_adapter.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <unistd.h>

#include "edge_digest.h"

#define SNAP_PATH_MAX 512
#define SNAP_HEADER_MAX (64u * 1024u)
#define SNAP_DATE_LEN 10
#define SNAP_LINE_MAX 512
#define SNAP_COPY_BUF 512
#define SNAP_DEFAULT_MAX_DELETE 10u
#define SNAP_MARKER_NAME ".edgeos-snapshot-complete"
#define SNAP_MARKER_TMP_NAME ".edgeos-snapshot-complete.tmp"
#define SNAP_MARKER_HEADER "edgeos-snapshot/1\n"
#define SNAP_DELETING_PREFIX ".deleting-"
#define SNAP_COPY_PREFIX ".tmp-"

static uint32_t g_usage_percent;
static int g_force_copy;

void edge_snapshot_retention_set_usage_percent(uint32_t percent) {
    g_usage_percent = percent;
}

void edge_snapshot_retention_set_force_copy(int on) {
    g_force_copy = on ? 1 : 0;
}

static edge_history_result_t fail(edge_history_result_t rc, char *reason, size_t reason_size,
                                  const char *text) {
    if (reason && reason_size) snprintf(reason, reason_size, "%s", text);
    return rc;
}

static edge_history_result_t fs_fail(char *reason, size_t reason_size, const char *what) {
    int saved = errno;
    if (reason && reason_size) snprintf(reason, reason_size, "%s: %s", what, strerror(saved));
    return (saved == ENOSPC || saved == EDQUOT) ? EDGE_HISTORY_NO_SPACE : EDGE_HISTORY_IO;
}

static int write_all_fd(int fd, const void *data, size_t len) {
    const unsigned char *bytes = data;
    size_t offset = 0;
    while (offset < len) {
        ssize_t written = write(fd, bytes + offset, len - offset);
        if (written < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (written == 0) {
            errno = EIO;
            return -1;
        }
        offset += (size_t)written;
    }
    return 0;
}

static int join_path2(char *out, size_t size, const char *a, const char *b) {
    int len = snprintf(out, size, "%s/%s", a, b);
    return len > 0 && (size_t)len < size ? 0 : -1;
}

static int fsync_dir(const char *path) {
    int fd = open(path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return -1;
    int rc = fsync(fd);
    int saved = errno;
    if (close(fd) != 0 && rc == 0) {
        rc = -1;
        saved = errno;
    }
    errno = saved;
    return rc;
}

static int remove_tree(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return errno == ENOENT ? 0 : -1;
    if (!S_ISDIR(st.st_mode)) {
        if (unlink(path) != 0 && errno != ENOENT) return -1;
        return 0;
    }
    DIR *dir = opendir(path);
    if (!dir) return -1;
    int rc = 0;
    struct dirent *entry = NULL;
    char child[SNAP_PATH_MAX];
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (join_path2(child, sizeof(child), path, entry->d_name) != 0 || remove_tree(child) != 0) {
            rc = -1;
            break;
        }
    }
    if (closedir(dir) != 0 && rc == 0) rc = -1;
    if (rc == 0 && rmdir(path) != 0 && errno != ENOENT) rc = -1;
    return rc;
}

/* ------------------------------------------------------------------------- */
/* Calendar dates                                                            */
/* ------------------------------------------------------------------------- */

static int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

static int days_in_month(int year, int month) {
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && is_leap_year(year)) return 29;
    return days[month - 1];
}

static int valid_date(int year, int month, int day) {
    if (year < 1000 || year > 9999 || month < 1 || month > 12) return 0;
    return day >= 1 && day <= days_in_month(year, month);
}

static int snapshot_date_from_tm(const struct tm *now, char out[SNAP_DATE_LEN + 1]) {
    if (now->tm_mon < 0 || now->tm_mon > 11) return -1;
    if (now->tm_hour < 0 || now->tm_hour > 23) return -1;
    if (now->tm_min < 0 || now->tm_min > 59) return -1;
    if (now->tm_sec < 0 || now->tm_sec > 59) return -1;
    if (now->tm_year < 0 - 1900 || now->tm_year > 9999 - 1900) return -1;
    int year = now->tm_year + 1900;
    int month = now->tm_mon + 1;
    if (!valid_date(year, month, now->tm_mday)) return -1;
    out[0] = (char)('0' + (year / 1000) % 10);
    out[1] = (char)('0' + (year / 100) % 10);
    out[2] = (char)('0' + (year / 10) % 10);
    out[3] = (char)('0' + year % 10);
    out[4] = '-';
    out[5] = (char)('0' + month / 10);
    out[6] = (char)('0' + month % 10);
    out[7] = '-';
    out[8] = (char)('0' + now->tm_mday / 10);
    out[9] = (char)('0' + now->tm_mday % 10);
    out[10] = '\0';
    return 0;
}

static int parse_date(const char *name) {
    if (strlen(name) != SNAP_DATE_LEN) return -1;
    for (int i = 0; i < SNAP_DATE_LEN; i++) {
        if (i == 4 || i == 7) {
            if (name[i] != '-') return -1;
        } else if (name[i] < '0' || name[i] > '9') {
            return -1;
        }
    }
    int year =
        (name[0] - '0') * 1000 + (name[1] - '0') * 100 + (name[2] - '0') * 10 + (name[3] - '0');
    int month = (name[5] - '0') * 10 + (name[6] - '0');
    int day = (name[8] - '0') * 10 + (name[9] - '0');
    return valid_date(year, month, day) ? 0 : -1;
}

/* ------------------------------------------------------------------------- */
/* Snapshot CSV writer                                                       */
/* ------------------------------------------------------------------------- */

typedef struct {
    int fd;
    char date[SNAP_DATE_LEN + 1];
    edge_snapshot_column_t *columns;
    size_t column_count;
    unsigned long long fail_after;
    unsigned long long payload_written;
} snapshot_csv_writer_t;

static int label_needs_quotes(const char *label) {
    return strchr(label, ',') != NULL || strchr(label, '"') != NULL;
}

static int build_header(const edge_snapshot_column_t *columns, size_t count, char **out,
                        size_t *out_len, char *reason, size_t reason_size) {
    size_t total = 3 + 4 + 1;
    for (size_t i = 0; i < count; i++) {
        const char *label = columns[i].label;
        if (!label) return fail(EDGE_HISTORY_INVALID, reason, reason_size, "column label is NULL");
        if (columns[i].kind != EDGE_SNAPSHOT_CELL_INT &&
            columns[i].kind != EDGE_SNAPSHOT_CELL_FLOAT &&
            columns[i].kind != EDGE_SNAPSHOT_CELL_EMPTY) {
            return fail(EDGE_HISTORY_INVALID, reason, reason_size, "column kind is unknown");
        }
        if (strchr(label, '\r') || strchr(label, '\n')) {
            return fail(EDGE_HISTORY_INVALID, reason, reason_size,
                        "column label contains CR or LF");
        }
        size_t length = strlen(label);
        size_t quotes = 0;
        if (label_needs_quotes(label)) {
            for (size_t j = 0; j < length; j++) {
                if (label[j] == '"') quotes++;
            }
            total += 2;
        }
        total += 1 + length + quotes;
        if (total > SNAP_HEADER_MAX) {
            return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot header is too large");
        }
    }
    char *header = malloc(total + 1);
    if (!header) return fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
    size_t offset = 0;
    header[offset++] = (char)0xEF;
    header[offset++] = (char)0xBB;
    header[offset++] = (char)0xBF;
    memcpy(header + offset, "time", 4);
    offset += 4;
    for (size_t i = 0; i < count; i++) {
        const char *label = columns[i].label;
        header[offset++] = ',';
        if (label_needs_quotes(label)) {
            header[offset++] = '"';
            for (const char *p = label; *p; p++) {
                if (*p == '"') header[offset++] = '"';
                header[offset++] = *p;
            }
            header[offset++] = '"';
        } else {
            size_t length = strlen(label);
            memcpy(header + offset, label, length);
            offset += length;
        }
    }
    header[offset++] = '\n';
    header[offset] = '\0';
    *out = header;
    *out_len = offset;
    return 0;
}

static int truncate_to_newline(int fd, off_t size, off_t floor) {
    char buffer[SNAP_COPY_BUF];
    off_t end = size;
    while (end > floor) {
        off_t chunk = end - floor;
        if (chunk > (off_t)sizeof(buffer)) chunk = (off_t)sizeof(buffer);
        ssize_t got = pread(fd, buffer, (size_t)chunk, end - chunk);
        if (got != (ssize_t)chunk) return -1;
        for (ssize_t i = got - 1; i >= 0; i--) {
            if (buffer[i] == '\n') return ftruncate(fd, end - chunk + i + 1);
        }
        end -= chunk;
    }
    return ftruncate(fd, floor);
}

static edge_history_result_t prepare_existing(int fd, const char *header, size_t header_len,
                                              char *reason, size_t reason_size) {
    off_t size = lseek(fd, 0, SEEK_END);
    if (size < 0) return fs_fail(reason, reason_size, "cannot size snapshot file");
    char *prefix = malloc(header_len);
    if (!prefix) return fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
    size_t amount = (size_t)size < header_len ? (size_t)size : header_len;
    ssize_t got = pread(fd, prefix, amount, 0);
    edge_history_result_t rc = EDGE_HISTORY_OK;
    if (got < 0 || (size_t)got != amount) {
        rc = fs_fail(reason, reason_size, "cannot read snapshot header");
    } else if ((size_t)size < header_len) {
        if (memcmp(prefix, header, amount) == 0) {
            if (ftruncate(fd, 0) != 0 || lseek(fd, 0, SEEK_SET) < 0 ||
                write_all_fd(fd, header, header_len) != 0 || fsync(fd) != 0) {
                rc = fs_fail(reason, reason_size, "cannot rewrite snapshot header");
            }
        } else if (amount >= 3 && memcmp(prefix, "\xEF\xBB\xBF", 3) == 0) {
            rc = fail(EDGE_HISTORY_CONFLICT, reason, reason_size,
                      "snapshot header differs from the configured columns");
        } else {
            rc = fail(EDGE_HISTORY_CORRUPT, reason, reason_size,
                      "snapshot file does not start with a BOM");
        }
    } else if (memcmp(prefix, header, header_len) == 0) {
        if ((off_t)header_len < size) {
            char last = 0;
            if (pread(fd, &last, 1, size - 1) != 1) {
                rc = fs_fail(reason, reason_size, "cannot read snapshot tail");
            } else if (last != '\n') {
                if (truncate_to_newline(fd, size, (off_t)header_len) != 0) {
                    rc = fs_fail(reason, reason_size, "cannot repair snapshot half row");
                }
            }
        }
        if (rc == EDGE_HISTORY_OK && lseek(fd, 0, SEEK_END) < 0) {
            rc = fs_fail(reason, reason_size, "cannot seek snapshot file");
        }
    } else if (memcmp(prefix, "\xEF\xBB\xBF", 3) == 0) {
        rc = fail(EDGE_HISTORY_CONFLICT, reason, reason_size,
                  "snapshot header differs from the configured columns");
    } else {
        rc = fail(EDGE_HISTORY_CORRUPT, reason, reason_size,
                  "snapshot file does not start with a BOM");
    }
    free(prefix);
    return rc;
}

static void writer_destroy(snapshot_csv_writer_t *writer) {
    if (!writer) return;
    if (writer->fd >= 0) {
        (void)fsync(writer->fd);
        (void)close(writer->fd);
    }
    for (size_t i = 0; i < writer->column_count; i++) free((void *)writer->columns[i].label);
    free(writer->columns);
    free(writer);
}

static edge_history_result_t writer_append_cb(void *opaque, const struct tm *local_now,
                                              const edge_snapshot_cell_t *cells, size_t count,
                                              char *reason, size_t reason_size);
static edge_history_result_t writer_flush_cb(void *opaque, char *reason, size_t reason_size);
static void writer_close_cb(void *opaque);

edge_history_result_t snapshot_csv_writer_open(const char *local_root, const struct tm *local_now,
                                               const char *file_stem,
                                               const edge_snapshot_column_t *columns,
                                               size_t column_count,
                                               edge_snapshot_writer_port_t *out, char *reason,
                                               size_t reason_size) {
    if (reason && reason_size) reason[0] = '\0';
    if (!out) return fail(EDGE_HISTORY_INVALID, reason, reason_size, "output port is NULL");
    memset(out, 0, sizeof(*out));
    if (!local_root || local_root[0] == '\0' || !local_now || !file_stem || file_stem[0] == '\0' ||
        file_stem[0] == '.' || strchr(file_stem, '/') || strchr(file_stem, '\r') ||
        strchr(file_stem, '\n')) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot arguments are invalid");
    }
    if (column_count > 0 && !columns) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "columns are missing");
    }
    char date[SNAP_DATE_LEN + 1];
    if (snapshot_date_from_tm(local_now, date) != 0) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "local time is invalid");
    }
    char *header = NULL;
    size_t header_len = 0;
    edge_history_result_t rc =
        build_header(columns, column_count, &header, &header_len, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;

    struct stat root_st;
    if (stat(local_root, &root_st) != 0) {
        free(header);
        return fs_fail(reason, reason_size, "snapshot local root is unavailable");
    }
    if (!S_ISDIR(root_st.st_mode)) {
        free(header);
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot local root is not a dir");
    }
    char date_dir[SNAP_PATH_MAX];
    char csv_path[SNAP_PATH_MAX];
    char file_name[SNAP_PATH_MAX];
    int name_len = snprintf(file_name, sizeof(file_name), "%s.csv", file_stem);
    if (join_path2(date_dir, sizeof(date_dir), local_root, date) != 0 || name_len < 0 ||
        (size_t)name_len >= sizeof(file_name) ||
        join_path2(csv_path, sizeof(csv_path), date_dir, file_name) != 0) {
        free(header);
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot path is too long");
    }

    snapshot_csv_writer_t *writer = calloc(1, sizeof(*writer));
    if (!writer) {
        free(header);
        return fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
    }
    writer->fd = -1;
    if (column_count > 0) {
        writer->columns = calloc(column_count, sizeof(*writer->columns));
        if (!writer->columns) {
            free(header);
            writer_destroy(writer);
            return fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
        }
        for (size_t i = 0; i < column_count; i++) {
            writer->columns[i].kind = columns[i].kind;
            char *label = strdup(columns[i].label);
            if (!label) {
                free(header);
                writer_destroy(writer);
                return fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
            }
            writer->columns[i].label = label;
            writer->column_count = i + 1;
        }
    }
    memcpy(writer->date, date, sizeof(writer->date));

    if (mkdir(date_dir, 0755) != 0) {
        if (errno != EEXIST) {
            rc = fs_fail(reason, reason_size, "cannot create snapshot date directory");
            free(header);
            writer_destroy(writer);
            return rc;
        }
        struct stat dir_st;
        if (stat(date_dir, &dir_st) != 0 || !S_ISDIR(dir_st.st_mode)) {
            rc = fail(EDGE_HISTORY_IO, reason, reason_size, "snapshot date path is not a dir");
            free(header);
            writer_destroy(writer);
            return rc;
        }
    }
    char marker_path[SNAP_PATH_MAX];
    struct stat marker_st;
    if (join_path2(marker_path, sizeof(marker_path), date_dir, SNAP_MARKER_NAME) != 0) {
        free(header);
        writer_destroy(writer);
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot path is too long");
    }
    if (lstat(marker_path, &marker_st) == 0) {
        free(header);
        writer_destroy(writer);
        return fail(EDGE_HISTORY_CONFLICT, reason, reason_size,
                    "snapshot date directory is already archived");
    }
    if (errno != ENOENT) {
        rc = fs_fail(reason, reason_size, "cannot inspect snapshot date directory");
        free(header);
        writer_destroy(writer);
        return rc;
    }

    int fd = open(csv_path, O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC, (mode_t)0644);
    int created = 1;
    if (fd < 0) {
        if (errno != EEXIST) {
            rc = fs_fail(reason, reason_size, "cannot create snapshot file");
            free(header);
            writer_destroy(writer);
            return rc;
        }
        created = 0;
        fd = open(csv_path, O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            rc = fs_fail(reason, reason_size, "cannot open snapshot file");
            free(header);
            writer_destroy(writer);
            return rc;
        }
    }
    if (created) {
        if (write_all_fd(fd, header, header_len) != 0 || fsync(fd) != 0) {
            int saved = errno;
            (void)close(fd);
            (void)unlink(csv_path);
            errno = saved;
            rc = fs_fail(reason, reason_size, "cannot write snapshot header");
            free(header);
            writer_destroy(writer);
            return rc;
        }
    } else {
        rc = prepare_existing(fd, header, header_len, reason, reason_size);
        if (rc != EDGE_HISTORY_OK) {
            (void)close(fd);
            free(header);
            writer_destroy(writer);
            return rc;
        }
    }
    free(header);
    writer->fd = fd;
    out->ctx = writer;
    out->append = writer_append_cb;
    out->flush = writer_flush_cb;
    out->close = writer_close_cb;
    return EDGE_HISTORY_OK;
}

static edge_history_result_t writer_write(snapshot_csv_writer_t *writer, const char *data,
                                          size_t len, char *reason, size_t reason_size) {
    if (writer->fail_after > 0 &&
        writer->payload_written + (unsigned long long)len > writer->fail_after) {
        unsigned long long allowed = 0;
        if (writer->payload_written < writer->fail_after) {
            allowed = writer->fail_after - writer->payload_written;
        }
        if (allowed > (unsigned long long)len) allowed = (unsigned long long)len;
        if (allowed > 0 && write_all_fd(writer->fd, data, (size_t)allowed) != 0) {
            return fs_fail(reason, reason_size, "snapshot write failed");
        }
        writer->payload_written += allowed;
        return fail(EDGE_HISTORY_NO_SPACE, reason, reason_size, "snapshot write failed: disk full");
    }
    if (write_all_fd(writer->fd, data, len) != 0) {
        return fs_fail(reason, reason_size, "snapshot write failed");
    }
    writer->payload_written += (unsigned long long)len;
    return EDGE_HISTORY_OK;
}

static edge_history_result_t writer_append_cb(void *opaque, const struct tm *local_now,
                                              const edge_snapshot_cell_t *cells, size_t count,
                                              char *reason, size_t reason_size) {
    snapshot_csv_writer_t *writer = opaque;
    if (reason && reason_size) reason[0] = '\0';
    if (!writer || !local_now) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot writer is invalid");
    }
    if (count != writer->column_count) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot cell count mismatch");
    }
    if (count > 0 && !cells) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot cells are missing");
    }
    char date[SNAP_DATE_LEN + 1];
    if (snapshot_date_from_tm(local_now, date) != 0 || strcmp(date, writer->date) != 0) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size,
                    "snapshot row date differs from the open date");
    }
    for (size_t i = 0; i < count; i++) {
        if (cells[i].kind != writer->columns[i].kind) {
            return fail(EDGE_HISTORY_INVALID, reason, reason_size,
                        "snapshot cell kind differs from its column");
        }
    }
    off_t start = lseek(writer->fd, 0, SEEK_END);
    if (start < 0) return fs_fail(reason, reason_size, "cannot seek snapshot file");
    unsigned long long start_payload = writer->payload_written;
    char stamp[32];
    int written = snprintf(stamp, sizeof(stamp), "%04d-%02d-%02d %02d:%02d:%02d",
                           local_now->tm_year + 1900, local_now->tm_mon + 1, local_now->tm_mday,
                           local_now->tm_hour, local_now->tm_min, local_now->tm_sec);
    edge_history_result_t rc = EDGE_HISTORY_OK;
    if (written < 0 || (size_t)written >= sizeof(stamp)) {
        rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot time is invalid");
    } else {
        rc = writer_write(writer, stamp, (size_t)written, reason, reason_size);
    }
    for (size_t i = 0; rc == EDGE_HISTORY_OK && i < count; i++) {
        char value[SNAP_LINE_MAX];
        int len = 0;
        if (cells[i].kind == EDGE_SNAPSHOT_CELL_INT) {
            len = snprintf(value, sizeof(value), ",%lld", (long long)cells[i].integer);
        } else if (cells[i].kind == EDGE_SNAPSHOT_CELL_FLOAT) {
            len = snprintf(value, sizeof(value), ", %.2f", cells[i].real);
        } else {
            len = snprintf(value, sizeof(value), ", ");
        }
        if (len < 0 || (size_t)len >= sizeof(value)) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot cell value is too long");
        } else {
            rc = writer_write(writer, value, (size_t)len, reason, reason_size);
        }
    }
    if (rc == EDGE_HISTORY_OK) {
        rc = writer_write(writer, "\n", 1, reason, reason_size);
    }
    if (rc != EDGE_HISTORY_OK) {
        if (ftruncate(writer->fd, start) != 0 || lseek(writer->fd, start, SEEK_SET) < 0) {
            rc = fs_fail(reason, reason_size, "cannot roll back snapshot partial row");
        }
        writer->payload_written = start_payload;
    }
    return rc;
}

static edge_history_result_t writer_flush_cb(void *opaque, char *reason, size_t reason_size) {
    snapshot_csv_writer_t *writer = opaque;
    if (reason && reason_size) reason[0] = '\0';
    if (!writer || writer->fd < 0) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot writer is invalid");
    }
    if (fsync(writer->fd) != 0) return fs_fail(reason, reason_size, "snapshot flush failed");
    return EDGE_HISTORY_OK;
}

static void writer_close_cb(void *opaque) {
    writer_destroy(opaque);
}

void snapshot_csv_writer_close(edge_snapshot_writer_port_t *port) {
    if (!port) return;
    if (port->close) port->close(port->ctx);
    memset(port, 0, sizeof(*port));
}

void snapshot_csv_writer_set_fail_after(edge_snapshot_writer_port_t *port,
                                        unsigned long long bytes) {
    if (!port || !port->ctx) return;
    ((snapshot_csv_writer_t *)port->ctx)->fail_after = bytes;
}

/* ------------------------------------------------------------------------- */
/* Name lists                                                                */
/* ------------------------------------------------------------------------- */

typedef struct {
    char **names;
    size_t count;
    size_t capacity;
} name_list_t;

static void name_list_free(name_list_t *list) {
    for (size_t i = 0; i < list->count; i++) free(list->names[i]);
    free(list->names);
    memset(list, 0, sizeof(*list));
}

static int name_list_push(name_list_t *list, const char *name) {
    if (list->count == list->capacity) {
        size_t capacity = list->capacity ? list->capacity * 2 : 8;
        char **names = realloc(list->names, capacity * sizeof(*names));
        if (!names) return -1;
        list->names = names;
        list->capacity = capacity;
    }
    char *copy = strdup(name);
    if (!copy) return -1;
    list->names[list->count++] = copy;
    return 0;
}

static int name_list_contains(const name_list_t *list, const char *name) {
    for (size_t i = 0; i < list->count; i++) {
        if (strcmp(list->names[i], name) == 0) return 1;
    }
    return 0;
}

static int name_compare(const void *a, const void *b) {
    const char *const *left = a;
    const char *const *right = b;
    return strcmp(*left, *right);
}

static void name_list_sort(name_list_t *list) {
    if (list->count > 1) qsort(list->names, list->count, sizeof(*list->names), name_compare);
}

static int scan_dir(const char *path, name_list_t *out) {
    DIR *dir = opendir(path);
    if (!dir) return -1;
    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (name_list_push(out, entry->d_name) != 0) {
            closedir(dir);
            return -1;
        }
    }
    return closedir(dir) == 0 ? 0 : -1;
}

/* ------------------------------------------------------------------------- */
/* Completion marker                                                         */
/* ------------------------------------------------------------------------- */

enum {
    SNAP_MARKER_OK = 0,
    SNAP_MARKER_MISSING = 1,
    SNAP_MARKER_INVALID = 2,
    SNAP_MARKER_NOT_REGULAR = 3,
    SNAP_MARKER_IO = -1,
};

static int marker_name(const char *name) {
    return strcmp(name, SNAP_MARKER_NAME) == 0 || strcmp(name, SNAP_MARKER_TMP_NAME) == 0;
}

static edge_history_result_t write_marker(const char *dir, char *reason, size_t reason_size) {
    name_list_t entries = {0};
    name_list_t files = {0};
    edge_history_result_t rc = EDGE_HISTORY_OK;
    int fd;
    int failed = 0;
    int saved;
    if (scan_dir(dir, &entries) != 0) {
        rc = fs_fail(reason, reason_size, "cannot scan snapshot directory");
        goto out;
    }
    for (size_t i = 0; i < entries.count; i++) {
        const char *name = entries.names[i];
        if (marker_name(name)) continue;
        if (strchr(name, '\r') || strchr(name, '\n')) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size,
                      "snapshot file name contains CR or LF");
            goto out;
        }
        char path[SNAP_PATH_MAX];
        struct stat st;
        if (join_path2(path, sizeof(path), dir, name) != 0 || lstat(path, &st) != 0) {
            rc = fs_fail(reason, reason_size, "cannot stat snapshot file for marker");
            goto out;
        }
        if (!S_ISREG(st.st_mode)) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size,
                      "snapshot directory holds a non-regular entry");
            goto out;
        }
        if (name_list_push(&files, name) != 0) {
            rc = fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
            goto out;
        }
    }
    name_list_sort(&files);

    char tmp_path[SNAP_PATH_MAX];
    char final_path[SNAP_PATH_MAX];
    if (join_path2(tmp_path, sizeof(tmp_path), dir, SNAP_MARKER_TMP_NAME) != 0 ||
        join_path2(final_path, sizeof(final_path), dir, SNAP_MARKER_NAME) != 0) {
        rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot marker path is too long");
        goto out;
    }
    fd = open(tmp_path, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, (mode_t)0644);
    if (fd < 0) {
        rc = fs_fail(reason, reason_size, "cannot create snapshot marker");
        goto out;
    }
    if (write_all_fd(fd, SNAP_MARKER_HEADER, strlen(SNAP_MARKER_HEADER)) != 0) failed = 1;
    for (size_t i = 0; !failed && i < files.count; i++) {
        char path[SNAP_PATH_MAX];
        char sha[65];
        unsigned long long size = 0;
        if (join_path2(path, sizeof(path), dir, files.names[i]) != 0 ||
            edge_sha256_file(path, sha, &size) != 0) {
            failed = 1;
            break;
        }
        char line[SNAP_LINE_MAX];
        int len = snprintf(line, sizeof(line), "%s %llu %s\n", sha, size, files.names[i]);
        if (len < 0 || (size_t)len >= sizeof(line) || write_all_fd(fd, line, (size_t)len) != 0) {
            failed = 1;
            break;
        }
    }
    if (!failed && fsync(fd) != 0) failed = 1;
    saved = errno;
    if (close(fd) != 0 && !failed) {
        failed = 1;
        saved = errno;
    }
    errno = saved;
    if (failed) {
        (void)unlink(tmp_path);
        rc = fs_fail(reason, reason_size, "cannot write snapshot marker");
        goto out;
    }
    if (rename(tmp_path, final_path) != 0) {
        saved = errno;
        (void)unlink(tmp_path);
        errno = saved;
        rc = fs_fail(reason, reason_size, "cannot commit snapshot marker");
        goto out;
    }
    if (fsync_dir(dir) != 0) rc = fs_fail(reason, reason_size, "cannot sync snapshot directory");
out:
    name_list_free(&entries);
    name_list_free(&files);
    return rc;
}

static int marker_line_parse(char *line, char sha[65], unsigned long long *size,
                             const char **name) {
    size_t len = strlen(line);
    if (len == 0 || line[len - 1] != '\n') return -1;
    line[len - 1] = '\0';
    char *space = strchr(line, ' ');
    if (!space || (size_t)(space - line) != 64) return -1;
    if (strspn(line, "0123456789abcdef") != 64) return -1;
    memcpy(sha, line, 64);
    sha[64] = '\0';
    errno = 0;
    char *end = NULL;
    unsigned long long value = strtoull(space + 1, &end, 10);
    if (errno != 0 || end == space + 1 || *end != ' ') return -1;
    const char *file = end + 1;
    if (*file == '\0' || strchr(file, '/') || strchr(file, '\r') || marker_name(file)) return -1;
    *size = value;
    *name = file;
    return 0;
}

static int verify_marker(const char *dir, char *reason, size_t reason_size) {
    char path[SNAP_PATH_MAX];
    if (join_path2(path, sizeof(path), dir, SNAP_MARKER_NAME) != 0) return SNAP_MARKER_INVALID;
    /* lstat first: opening a FIFO marker with no writer would block forever. */
    struct stat marker_st;
    if (lstat(path, &marker_st) != 0) {
        if (errno == ENOENT) return SNAP_MARKER_MISSING;
        if (reason && reason_size) {
            snprintf(reason, reason_size, "cannot inspect snapshot marker: %s", strerror(errno));
        }
        return SNAP_MARKER_IO;
    }
    if (!S_ISREG(marker_st.st_mode)) return SNAP_MARKER_NOT_REGULAR;
    int fd = open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        if (errno == ENOENT) return SNAP_MARKER_MISSING;
        if (errno == ELOOP) return SNAP_MARKER_NOT_REGULAR;
        if (reason && reason_size) {
            snprintf(reason, reason_size, "cannot open snapshot marker: %s", strerror(errno));
        }
        return SNAP_MARKER_IO;
    }
    struct stat opened_st;
    if (fstat(fd, &opened_st) != 0) {
        int saved = errno;
        (void)close(fd);
        errno = saved;
        return SNAP_MARKER_IO;
    }
    if (!S_ISREG(opened_st.st_mode)) {
        (void)close(fd);
        return SNAP_MARKER_NOT_REGULAR;
    }
    FILE *file = fdopen(fd, "r");
    if (!file) {
        int saved = errno;
        (void)close(fd);
        errno = saved;
        return SNAP_MARKER_IO;
    }
    char line[SNAP_LINE_MAX];
    if (!fgets(line, sizeof(line), file) || strcmp(line, SNAP_MARKER_HEADER) != 0) {
        (void)fclose(file);
        return SNAP_MARKER_INVALID;
    }
    name_list_t listed = {0};
    int state = SNAP_MARKER_OK;
    while (state == SNAP_MARKER_OK && fgets(line, sizeof(line), file)) {
        char sha[65];
        unsigned long long size = 0;
        const char *name = NULL;
        if (marker_line_parse(line, sha, &size, &name) != 0 || name_list_contains(&listed, name)) {
            state = SNAP_MARKER_INVALID;
            break;
        }
        char file_path[SNAP_PATH_MAX];
        struct stat st;
        if (join_path2(file_path, sizeof(file_path), dir, name) != 0 ||
            lstat(file_path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
            (unsigned long long)st.st_size != size) {
            state = SNAP_MARKER_INVALID;
            break;
        }
        char actual[65];
        unsigned long long actual_size = 0;
        if (edge_sha256_file(file_path, actual, &actual_size) != 0) {
            state = SNAP_MARKER_INVALID;
            break;
        }
        if (actual_size != size || strcmp(actual, sha) != 0) {
            state = SNAP_MARKER_INVALID;
            break;
        }
        if (name_list_push(&listed, name) != 0) {
            state = SNAP_MARKER_IO;
            break;
        }
    }
    if (state == SNAP_MARKER_OK && ferror(file)) state = SNAP_MARKER_IO;
    if (fclose(file) != 0 && state == SNAP_MARKER_OK) state = SNAP_MARKER_IO;
    if (state != SNAP_MARKER_OK) {
        name_list_free(&listed);
        return state;
    }

    name_list_t entries = {0};
    if (scan_dir(dir, &entries) != 0) {
        name_list_free(&entries);
        name_list_free(&listed);
        return SNAP_MARKER_IO;
    }
    for (size_t i = 0; i < entries.count && state == SNAP_MARKER_OK; i++) {
        if (strcmp(entries.names[i], SNAP_MARKER_NAME) == 0) continue;
        if (!name_list_contains(&listed, entries.names[i])) state = SNAP_MARKER_INVALID;
    }
    name_list_free(&entries);
    name_list_free(&listed);
    return state;
}

/* ------------------------------------------------------------------------- */
/* Backup comparison, copy and move                                          */
/* ------------------------------------------------------------------------- */

static int files_match(const char *a, const char *b) {
    char sha_a[65];
    char sha_b[65];
    unsigned long long size_a = 0;
    unsigned long long size_b = 0;
    if (edge_sha256_file(a, sha_a, &size_a) != 0) return -1;
    if (edge_sha256_file(b, sha_b, &size_b) != 0) return -1;
    return size_a == size_b && strcmp(sha_a, sha_b) == 0;
}

static int backup_covers(const char *local_dir, const char *backup_dir, char *reason,
                         size_t reason_size) {
    name_list_t entries = {0};
    if (scan_dir(local_dir, &entries) != 0) {
        name_list_free(&entries);
        return -1;
    }
    int covered = 1;
    for (size_t i = 0; i < entries.count; i++) {
        const char *name = entries.names[i];
        if (marker_name(name)) continue;
        char local_path[SNAP_PATH_MAX];
        char backup_path[SNAP_PATH_MAX];
        struct stat local_st;
        struct stat backup_st;
        if (join_path2(local_path, sizeof(local_path), local_dir, name) != 0 ||
            join_path2(backup_path, sizeof(backup_path), backup_dir, name) != 0 ||
            lstat(local_path, &local_st) != 0 || !S_ISREG(local_st.st_mode) ||
            lstat(backup_path, &backup_st) != 0 || !S_ISREG(backup_st.st_mode)) {
            covered = 0;
            break;
        }
        int match = files_match(local_path, backup_path);
        if (match < 0) {
            covered = -1;
            break;
        }
        if (!match) {
            covered = 0;
            break;
        }
    }
    name_list_free(&entries);
    if (covered < 0 && reason && reason_size) {
        snprintf(reason, reason_size, "cannot hash snapshot backup: %s", strerror(errno));
    }
    return covered;
}

static edge_history_result_t copy_files(const char *src_dir, const char *dst_dir, char *reason,
                                        size_t reason_size) {
    name_list_t entries = {0};
    if (scan_dir(src_dir, &entries) != 0) {
        name_list_free(&entries);
        return fs_fail(reason, reason_size, "cannot scan snapshot source");
    }
    edge_history_result_t rc = EDGE_HISTORY_OK;
    for (size_t i = 0; i < entries.count; i++) {
        const char *name = entries.names[i];
        if (marker_name(name)) continue;
        char src_path[SNAP_PATH_MAX];
        char dst_path[SNAP_PATH_MAX];
        struct stat st;
        if (join_path2(src_path, sizeof(src_path), src_dir, name) != 0 ||
            join_path2(dst_path, sizeof(dst_path), dst_dir, name) != 0 ||
            lstat(src_path, &st) != 0) {
            rc = fs_fail(reason, reason_size, "cannot stat snapshot copy source");
            break;
        }
        if (!S_ISREG(st.st_mode)) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size,
                      "snapshot copy source is not a regular file");
            break;
        }
        int in = open(src_path, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
        if (in < 0) {
            rc = fs_fail(reason, reason_size, "cannot open snapshot copy source");
            break;
        }
        int out = open(dst_path, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, (mode_t)0644);
        if (out < 0) {
            int saved = errno;
            (void)close(in);
            errno = saved;
            rc = fs_fail(reason, reason_size, "cannot create snapshot copy");
            break;
        }
        char buffer[SNAP_COPY_BUF];
        int failed = 0;
        for (;;) {
            ssize_t got = read(in, buffer, sizeof(buffer));
            if (got < 0) {
                if (errno == EINTR) continue;
                failed = 1;
                break;
            }
            if (got == 0) break;
            if (write_all_fd(out, buffer, (size_t)got) != 0) {
                failed = 1;
                break;
            }
        }
        if (!failed && fsync(out) != 0) failed = 1;
        int saved = errno;
        if (close(out) != 0 && !failed) {
            failed = 1;
            saved = errno;
        }
        errno = saved;
        if (close(in) != 0 && !failed) {
            failed = 1;
            saved = errno;
            errno = saved;
        }
        if (failed) {
            rc = fs_fail(reason, reason_size, "cannot copy snapshot file");
            break;
        }
        int match = files_match(src_path, dst_path);
        if (match < 0) {
            rc = fs_fail(reason, reason_size, "cannot verify snapshot copy");
            break;
        }
        if (!match) {
            rc = fail(EDGE_HISTORY_IO, reason, reason_size,
                      "snapshot copy does not match its source");
            break;
        }
    }
    name_list_free(&entries);
    return rc;
}

static edge_history_result_t copy_move(const char *local_dir, const char *backup_root,
                                       const char *date, edge_snapshot_retention_result_t *result,
                                       char *reason, size_t reason_size) {
    char temp[SNAP_PATH_MAX];
    char backup_dir[SNAP_PATH_MAX];
    int len = snprintf(temp, sizeof(temp), "%s/%s%s-%ld", backup_root, SNAP_COPY_PREFIX, date,
                       (long)getpid());
    if (join_path2(backup_dir, sizeof(backup_dir), backup_root, date) != 0 || len < 0 ||
        (size_t)len >= sizeof(temp)) {
        result->failed_dirs++;
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot copy path is too long");
    }
    if (remove_tree(temp) != 0) {
        result->failed_dirs++;
        return fs_fail(reason, reason_size, "cannot clear stale snapshot copy");
    }
    if (mkdir(temp, 0755) != 0) {
        result->failed_dirs++;
        return fs_fail(reason, reason_size, "cannot create snapshot copy directory");
    }
    edge_history_result_t rc = copy_files(local_dir, temp, reason, reason_size);
    if (rc == EDGE_HISTORY_OK) rc = write_marker(temp, reason, reason_size);
    if (rc == EDGE_HISTORY_OK && fsync_dir(temp) != 0) {
        rc = fs_fail(reason, reason_size, "cannot sync snapshot copy directory");
    }
    if (rc == EDGE_HISTORY_OK && rename(temp, backup_dir) != 0) {
        rc = fs_fail(reason, reason_size, "cannot commit snapshot copy");
    }
    if (rc != EDGE_HISTORY_OK) {
        (void)remove_tree(temp);
        result->failed_dirs++;
        return rc;
    }
    if (fsync_dir(backup_root) != 0) {
        result->failed_dirs++;
        return fs_fail(reason, reason_size, "cannot sync snapshot backup root");
    }
    result->moved_dirs++;
    if (remove_tree(local_dir) != 0) {
        result->failed_dirs++;
        return fs_fail(reason, reason_size, "cannot remove snapshot local directory");
    }
    return EDGE_HISTORY_OK;
}

static edge_history_result_t move_one(const char *local_root, const char *backup_root,
                                      const char *date, edge_snapshot_retention_result_t *result,
                                      char *reason, size_t reason_size) {
    char local_dir[SNAP_PATH_MAX];
    char backup_dir[SNAP_PATH_MAX];
    char marker_tmp[SNAP_PATH_MAX];
    if (join_path2(local_dir, sizeof(local_dir), local_root, date) != 0 ||
        join_path2(backup_dir, sizeof(backup_dir), backup_root, date) != 0 ||
        join_path2(marker_tmp, sizeof(marker_tmp), local_dir, SNAP_MARKER_TMP_NAME) != 0) {
        result->failed_dirs++;
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot path is too long");
    }
    struct stat st;
    if (lstat(backup_dir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            result->conflicts++;
            return EDGE_HISTORY_OK;
        }
        int covered = backup_covers(local_dir, backup_dir, reason, reason_size);
        if (covered < 0) {
            result->failed_dirs++;
            return EDGE_HISTORY_IO;
        }
        if (!covered) {
            result->conflicts++;
            return EDGE_HISTORY_OK;
        }
        if (remove_tree(local_dir) != 0) {
            result->failed_dirs++;
            return fs_fail(reason, reason_size, "cannot remove duplicated snapshot directory");
        }
        result->deduped_local_dirs++;
        return EDGE_HISTORY_OK;
    }
    if (errno != ENOENT) {
        result->failed_dirs++;
        return fs_fail(reason, reason_size, "cannot inspect snapshot backup directory");
    }

    if (unlink(marker_tmp) != 0 && errno != ENOENT) {
        result->failed_dirs++;
        return fs_fail(reason, reason_size, "cannot remove stale snapshot marker");
    }
    int marker = verify_marker(local_dir, reason, reason_size);
    if (marker == SNAP_MARKER_IO) {
        result->failed_dirs++;
        return EDGE_HISTORY_IO;
    }
    if (marker == SNAP_MARKER_NOT_REGULAR) {
        result->conflicts++;
        return EDGE_HISTORY_OK;
    }
    if (marker == SNAP_MARKER_INVALID) {
        result->failed_dirs++;
        return fail(EDGE_HISTORY_CORRUPT, reason, reason_size,
                    "snapshot completion marker is invalid");
    }
    if (marker == SNAP_MARKER_MISSING) {
        edge_history_result_t rc = write_marker(local_dir, reason, reason_size);
        if (rc != EDGE_HISTORY_OK) {
            result->failed_dirs++;
            return rc;
        }
    }
    if (g_force_copy) return copy_move(local_dir, backup_root, date, result, reason, reason_size);
    if (rename(local_dir, backup_dir) == 0) {
        if (fsync_dir(backup_root) != 0) {
            result->failed_dirs++;
            return fs_fail(reason, reason_size, "cannot sync snapshot backup root");
        }
        result->moved_dirs++;
        return EDGE_HISTORY_OK;
    }
    if (errno == EXDEV) return copy_move(local_dir, backup_root, date, result, reason, reason_size);
    result->failed_dirs++;
    return fs_fail(reason, reason_size, "cannot move snapshot directory");
}

static edge_history_result_t cleanup_temp_dirs(const char *backup_root, char *reason,
                                               size_t reason_size) {
    name_list_t entries = {0};
    if (scan_dir(backup_root, &entries) != 0) {
        name_list_free(&entries);
        return fs_fail(reason, reason_size, "cannot scan snapshot backup root");
    }
    edge_history_result_t rc = EDGE_HISTORY_OK;
    for (size_t i = 0; i < entries.count; i++) {
        const char *name = entries.names[i];
        if (strncmp(name, SNAP_DELETING_PREFIX, strlen(SNAP_DELETING_PREFIX)) != 0 &&
            strncmp(name, SNAP_COPY_PREFIX, strlen(SNAP_COPY_PREFIX)) != 0) {
            continue;
        }
        char path[SNAP_PATH_MAX];
        struct stat st;
        if (join_path2(path, sizeof(path), backup_root, name) != 0) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot path is too long");
            break;
        }
        if (lstat(path, &st) != 0) {
            if (errno == ENOENT) continue;
            rc = fs_fail(reason, reason_size, "cannot inspect snapshot temporary directory");
            break;
        }
        if (!S_ISDIR(st.st_mode)) continue;
        if (remove_tree(path) != 0) {
            rc = fs_fail(reason, reason_size, "cannot remove snapshot temporary directory");
            break;
        }
    }
    name_list_free(&entries);
    return rc;
}

/* ------------------------------------------------------------------------- */
/* Backup quota                                                              */
/* ------------------------------------------------------------------------- */

typedef struct {
    const char *backup_root;
    const char *today;
    uint32_t threshold;
    uint32_t max_delete;
    uint32_t deleted;
    int counted;
    edge_snapshot_retention_result_t *result;
} snapshot_quota_t;

static edge_history_result_t usage_state(const char *backup_root, uint32_t threshold,
                                         uint32_t *percent, int *over, char *reason,
                                         size_t reason_size) {
    if (g_usage_percent != 0) {
        *percent = g_usage_percent;
        *over = g_usage_percent > threshold;
        return EDGE_HISTORY_OK;
    }
    struct statvfs info;
    if (statvfs(backup_root, &info) != 0) {
        return fs_fail(reason, reason_size, "cannot read snapshot backup filesystem");
    }
    if (info.f_blocks == 0) {
        return fail(EDGE_HISTORY_IO, reason, reason_size,
                    "snapshot backup filesystem reports zero blocks");
    }
    uint64_t blocks = (uint64_t)info.f_blocks;
    uint64_t available = (uint64_t)info.f_bavail;
    uint64_t used = blocks > available ? blocks - available : 0;
    *percent = (uint32_t)(used * 100u / blocks);
    *over = used * 100u > (uint64_t)threshold * blocks;
    return EDGE_HISTORY_OK;
}

static edge_history_result_t quota_pass(snapshot_quota_t *quota, char *reason, size_t reason_size) {
    if (quota->threshold == 0) return EDGE_HISTORY_OK;
    uint32_t percent = 0;
    int over = 0;
    int report;
    edge_history_result_t rc =
        usage_state(quota->backup_root, quota->threshold, &percent, &over, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;
    quota->result->usage_known = 1;
    quota->result->usage_percent_after = percent;
    quota->result->still_over_threshold = over ? 1 : 0;
    if (!over || quota->deleted >= quota->max_delete) return EDGE_HISTORY_OK;

    name_list_t entries = {0};
    name_list_t dates = {0};
    if (scan_dir(quota->backup_root, &entries) != 0) {
        rc = fs_fail(reason, reason_size, "cannot scan snapshot backup root");
        goto out;
    }
    report = !quota->counted;
    for (size_t i = 0; i < entries.count; i++) {
        const char *name = entries.names[i];
        char path[SNAP_PATH_MAX];
        struct stat st;
        if (join_path2(path, sizeof(path), quota->backup_root, name) != 0) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot path is too long");
            goto out;
        }
        if (lstat(path, &st) != 0) {
            if (errno == ENOENT) continue;
            rc = fs_fail(reason, reason_size, "cannot inspect snapshot backup entry");
            goto out;
        }
        if (!S_ISDIR(st.st_mode) || parse_date(name) != 0 || strcmp(name, quota->today) >= 0) {
            if (report) quota->result->skipped_unknown++;
            continue;
        }
        if (name_list_push(&dates, name) != 0) {
            rc = fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
            goto out;
        }
    }
    quota->counted = 1;
    name_list_sort(&dates);
    for (size_t i = 0; i < dates.count && quota->deleted < quota->max_delete; i++) {
        char dir[SNAP_PATH_MAX];
        if (join_path2(dir, sizeof(dir), quota->backup_root, dates.names[i]) != 0) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot path is too long");
            break;
        }
        int marker = verify_marker(dir, reason, reason_size);
        if (marker == SNAP_MARKER_MISSING) {
            if (report) quota->result->skipped_unknown++;
            continue;
        }
        if (marker == SNAP_MARKER_INVALID || marker == SNAP_MARKER_NOT_REGULAR) {
            if (report) quota->result->conflicts++;
            continue;
        }
        if (marker == SNAP_MARKER_IO) {
            rc = EDGE_HISTORY_IO;
            break;
        }
        char temp[SNAP_PATH_MAX];
        int len = snprintf(temp, sizeof(temp), "%s/%s%s-%ld", quota->backup_root,
                           SNAP_DELETING_PREFIX, dates.names[i], (long)getpid());
        if (len < 0 || (size_t)len >= sizeof(temp)) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot path is too long");
            break;
        }
        if (rename(dir, temp) != 0) {
            if (errno == ENOENT) continue;
            rc = fs_fail(reason, reason_size, "cannot rename snapshot backup directory");
            break;
        }
        if (remove_tree(temp) != 0) {
            rc = fs_fail(reason, reason_size, "cannot delete snapshot backup directory");
            break;
        }
        quota->deleted++;
        quota->result->deleted_backup_dirs++;
        rc =
            usage_state(quota->backup_root, quota->threshold, &percent, &over, reason, reason_size);
        if (rc != EDGE_HISTORY_OK) break;
        quota->result->usage_percent_after = percent;
        quota->result->still_over_threshold = over ? 1 : 0;
        if (!over) break;
    }
out:
    name_list_free(&entries);
    name_list_free(&dates);
    return rc;
}

/* ------------------------------------------------------------------------- */
/* Maintenance entry point                                                   */
/* ------------------------------------------------------------------------- */

static edge_history_result_t move_expired(const edge_snapshot_retention_policy_t *policy,
                                          const char *today,
                                          edge_snapshot_retention_result_t *result, char *reason,
                                          size_t reason_size) {
    name_list_t entries = {0};
    name_list_t past = {0};
    edge_history_result_t rc = EDGE_HISTORY_OK;
    size_t keep;
    size_t limit;
    if (scan_dir(policy->local_root, &entries) != 0) {
        name_list_free(&entries);
        return fs_fail(reason, reason_size, "cannot scan snapshot local root");
    }
    for (size_t i = 0; i < entries.count; i++) {
        const char *name = entries.names[i];
        char path[SNAP_PATH_MAX];
        struct stat st;
        if (join_path2(path, sizeof(path), policy->local_root, name) != 0) {
            rc = fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot path is too long");
            goto out;
        }
        if (lstat(path, &st) != 0) {
            if (errno == ENOENT) continue;
            rc = fs_fail(reason, reason_size, "cannot inspect snapshot local entry");
            goto out;
        }
        if (S_ISDIR(st.st_mode) && parse_date(name) == 0) {
            if (strcmp(name, today) < 0) {
                if (name_list_push(&past, name) != 0) {
                    rc = fail(EDGE_HISTORY_IO, reason, reason_size, "out of memory");
                    goto out;
                }
            } else if (strcmp(name, today) > 0) {
                result->skipped_unknown++;
            }
        } else {
            result->skipped_unknown++;
        }
    }
    name_list_sort(&past);
    keep = (size_t)policy->keep_local_days;
    limit = past.count > keep ? past.count - keep : 0;
    for (size_t i = 0; i < limit; i++) {
        rc = move_one(policy->local_root, policy->backup_root, past.names[i], result, reason,
                      reason_size);
        if (rc != EDGE_HISTORY_OK) break;
    }
out:
    name_list_free(&entries);
    name_list_free(&past);
    return rc;
}

edge_history_result_t edge_snapshot_maintain(const struct tm *local_now,
                                             const edge_snapshot_retention_policy_t *policy,
                                             edge_snapshot_retention_result_t *result, char *reason,
                                             size_t reason_size) {
    if (reason && reason_size) reason[0] = '\0';
    if (!local_now || !policy || !result || !policy->local_root || policy->local_root[0] == '\0') {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot policy is invalid");
    }
    memset(result, 0, sizeof(*result));
    char today[SNAP_DATE_LEN + 1];
    if (snapshot_date_from_tm(local_now, today) != 0) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "local time is invalid");
    }
    struct stat local_st;
    if (stat(policy->local_root, &local_st) != 0) {
        return fs_fail(reason, reason_size, "snapshot local root is unavailable");
    }
    if (!S_ISDIR(local_st.st_mode)) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size, "snapshot local root is not a dir");
    }
    if (!policy->backup_root || policy->backup_root[0] == '\0') {
        result->backup_unavailable = 1;
        return EDGE_HISTORY_OK;
    }
    struct stat backup_st;
    if (stat(policy->backup_root, &backup_st) != 0) {
        result->backup_unavailable = 1;
        return fs_fail(reason, reason_size, "snapshot backup root is unavailable");
    }
    if (!S_ISDIR(backup_st.st_mode)) {
        result->backup_unavailable = 1;
        return fail(EDGE_HISTORY_IO, reason, reason_size, "snapshot backup root is not a dir");
    }
    if (local_st.st_dev == backup_st.st_dev && local_st.st_ino == backup_st.st_ino) {
        return fail(EDGE_HISTORY_INVALID, reason, reason_size,
                    "snapshot local and backup roots are the same directory");
    }

    edge_history_result_t rc = cleanup_temp_dirs(policy->backup_root, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;

    snapshot_quota_t quota;
    memset(&quota, 0, sizeof(quota));
    quota.backup_root = policy->backup_root;
    quota.today = today;
    quota.threshold = policy->usage_threshold_percent;
    quota.max_delete = policy->max_delete_dirs ? policy->max_delete_dirs : SNAP_DEFAULT_MAX_DELETE;
    quota.result = result;

    rc = quota_pass(&quota, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;
    rc = move_expired(policy, today, result, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;
    rc = quota_pass(&quota, reason, reason_size);
    if (rc != EDGE_HISTORY_OK) return rc;
    if (result->conflicts > 0) {
        return fail(EDGE_HISTORY_CONFLICT, reason, reason_size,
                    "snapshot backup verification failed");
    }
    if (result->failed_dirs > 0) {
        return fail(EDGE_HISTORY_IO, reason, reason_size, "snapshot directory operation failed");
    }
    return EDGE_HISTORY_OK;
}
