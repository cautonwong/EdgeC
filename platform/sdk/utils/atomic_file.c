#include "atomic_file.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

int atomic_file_save(const char *filename, const char *content) {
    if (!filename || !content) {
        return -1;
    }

    char tmp_name[256];
    int ret = snprintf(tmp_name, sizeof(tmp_name), "%s.tmp", filename);
    if (ret < 0 || ret >= sizeof(tmp_name)) {
        // Failed to generate temporary filename
        return -1;
    }

    // 1. Create and open the temporary file
    FILE *fp = fopen(tmp_name, "w");
    if (!fp) {
        perror("atomic_file_save: fopen failed");
        return -1;
    }

    // 2. Write the content
    if (fputs(content, fp) == EOF) {
        perror("atomic_file_save: fputs failed");
        fclose(fp);
        unlink(tmp_name); // Clean up the temporary file
        return -1;
    }

    // 3. Force flush to disk to guarantee persistence
    // fflush() only flushes to the OS kernel buffer.
    if (fflush(fp) != 0) {
        perror("atomic_file_save: fflush failed");
        fclose(fp);
        unlink(tmp_name);
        return -1;
    }
    // fsync() ensures the kernel buffer is written to the physical disk.
    int fd = fileno(fp);
    if (fsync(fd) != 0) {
        perror("atomic_file_save: fsync failed");
        fclose(fp);
        unlink(tmp_name);
        return -1;
    }

    // Close the temporary file
    if (fclose(fp) != 0) {
        perror("atomic_file_save: fclose failed");
        unlink(tmp_name);
        return -1;
    }

    // 4. Atomically rename the temporary file to the final filename.
    // This is the atomic step. It either succeeds or fails, leaving the
    // original file untouched.
    if (rename(tmp_name, filename) != 0) {
        perror("atomic_file_save: rename failed");
        unlink(tmp_name);
        return -1;
    }

    return 0; // Success
}
