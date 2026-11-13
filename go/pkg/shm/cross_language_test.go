package shm

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"testing"
)

const cHelperSource = `
#define _GNU_SOURCE
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    uint32_t magic;
    uint16_t abi;
    uint16_t header_size;
    uint32_t capacity;
    uint32_t record_size;
    uint64_t sequence;
    uint64_t data_offset;
} shm_header_t;

int main(int argc, char **argv) {
    if (argc != 4) return 2;
    int writing = strcmp(argv[1], "write") == 0;
    int iterations = atoi(argv[3]);
    char path[256];
    snprintf(path, sizeof(path), "/dev/shm/edgeos_shadow_%s", argv[2]);

    int fd = open(path, O_RDWR);
    if (fd < 0) return 3;
    struct stat st;
    if (fstat(fd, &st) < 0) return 3;
    void *mapping = mmap(NULL, (size_t)st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) return 3;
    shm_header_t *h = (shm_header_t *)mapping;
    if (h->magic != 0x44484231u || h->abi != 1 || h->data_offset != 32) return 4;
    uint8_t *base = (uint8_t *)mapping + h->data_offset;
    double *value = (double *)(base + 32);

    for (int i = 0; i < iterations; i++) {
        if (writing) {
            flock(fd, LOCK_EX);
            __atomic_fetch_add(&h->sequence, 1, __ATOMIC_ACQ_REL);
            *value = (i & 1) ? 2.0 : 1.0;
            __atomic_fetch_add(&h->sequence, 1, __ATOMIC_RELEASE);
            flock(fd, LOCK_UN);
        } else {
            int consistent = 0;
            for (int retry = 0; retry < 8; retry++) {
                flock(fd, LOCK_SH);
                uint64_t before = __atomic_load_n(&h->sequence, __ATOMIC_ACQUIRE);
                if (before & 1) { flock(fd, LOCK_UN); continue; }
                double v = *value;
                uint64_t after = __atomic_load_n(&h->sequence, __ATOMIC_ACQUIRE);
                flock(fd, LOCK_UN);
                if (before == after) {
                    if (v != 1.0 && v != 2.0) return 5;
                    consistent = 1;
                    break;
                }
            }
            if (!consistent) return 6;
        }
    }
    munmap(mapping, (size_t)st.st_size);
    close(fd);
    return 0;
}
`

func buildCrossLanguageHelper(t *testing.T) string {
	t.Helper()
	if _, err := exec.LookPath("cc"); err != nil {
		t.Skip("cc not available")
	}
	dir := t.TempDir()
	src := filepath.Join(dir, "shm_peer.c")
	bin := filepath.Join(dir, "shm_peer")
	if err := os.WriteFile(src, []byte(cHelperSource), 0o600); err != nil {
		t.Fatal(err)
	}
	cmd := exec.Command("cc", "-O2", "-std=gnu11", "-pthread", "-o", bin, src)
	if out, err := cmd.CombinedOutput(); err != nil {
		t.Fatalf("build helper: %v\n%s", err, out)
	}
	return bin
}

func TestCrossLanguageConcurrentSeqlock(t *testing.T) {
	helper := buildCrossLanguageHelper(t)
	name := fmt.Sprintf("go_xlang_%d", os.Getpid())
	matrix, err := CreateOrOpenShadowMatrix(name, 4)
	if err != nil {
		t.Fatal(err)
	}
	defer func() {
		matrix.Close()
		os.Remove("/dev/shm/edgeos_shadow_" + name)
	}()
	if err := matrix.WriteTag(0, 1.0, 0); err != nil {
		t.Fatal(err)
	}

	writer := exec.Command(helper, "write", name, "200000")
	if err := writer.Start(); err != nil {
		t.Fatal(err)
	}
	for i := 0; i < 200000; i++ {
		msg, err := matrix.ReadTag(0)
		if err != nil {
			t.Fatalf("read while C writes: %v", err)
		}
		v, _ := msg.Scalar()
		if v != 1.0 && v != 2.0 {
			t.Fatalf("torn read from C writer: %v", v)
		}
	}
	if err := writer.Wait(); err != nil {
		t.Fatalf("C writer: %v", err)
	}

	reader := exec.Command(helper, "read", name, "200000")
	if err := reader.Start(); err != nil {
		t.Fatal(err)
	}
	for i := 0; i < 200000; i++ {
		v := 1.0
		if i&1 != 0 {
			v = 2.0
		}
		if err := matrix.WriteTag(0, v, uint64(i)); err != nil {
			t.Fatalf("write while C reads: %v", err)
		}
	}
	if err := reader.Wait(); err != nil {
		t.Fatalf("C reader: %v", err)
	}
}
