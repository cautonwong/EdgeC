/**
 * @file main.c
 * @brief edgeos-backup: create, verify and restore platform backup bundles.
 */

#include <stdio.h>
#include <string.h>

#include "edgeos/edge_backup.h"

static int usage(void) {
    fprintf(stderr,
            "usage:\n"
            "  edgeos-backup create --db PATH --out FILE\n"
            "  edgeos-backup verify --in FILE\n"
            "  edgeos-backup restore --in FILE --db PATH\n");
    return 2;
}

static const char *value_of(int argc, char **argv, const char *name) {
    for (int i = 2; i + 1 < argc; i++) {
        if (strcmp(argv[i], name) == 0) return argv[i + 1];
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 2) return usage();
    char reason[EDGE_BACKUP_REASON_SIZE] = "";

    if (strcmp(argv[1], "create") == 0) {
        const char *db = value_of(argc, argv, "--db");
        const char *out = value_of(argc, argv, "--out");
        if (!db || !out) return usage();
        if (edge_backup_create(db, out, reason, sizeof(reason)) != 0) {
            fprintf(stderr, "edgeos-backup: %s\n", reason);
            return 1;
        }
        printf("wrote %s\n", out);
        return 0;
    }
    if (strcmp(argv[1], "verify") == 0) {
        const char *in = value_of(argc, argv, "--in");
        if (!in) return usage();
        if (edge_backup_verify(in, reason, sizeof(reason)) != 0) {
            fprintf(stderr, "edgeos-backup: %s\n", reason);
            return 1;
        }
        printf("ok %s\n", in);
        return 0;
    }
    if (strcmp(argv[1], "restore") == 0) {
        const char *in = value_of(argc, argv, "--in");
        const char *db = value_of(argc, argv, "--db");
        if (!in || !db) return usage();
        if (edge_backup_restore(in, db, reason, sizeof(reason)) != 0) {
            fprintf(stderr, "edgeos-backup: %s\n", reason);
            return 1;
        }
        printf("restored %s\n", db);
        return 0;
    }
    return usage();
}
