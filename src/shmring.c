#include "shmring.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>

// Ensure the header struct is exactly one cache line
_Static_assert(sizeof(ShmRingHeader) == CACHELINE_SIZE, "ShmRingHeader must be one cache line size");

int shmring_create(ShmRing *ring, const char *name, uint32_t slots)
{
    if (!ring || !name || slots == 0 || slots > SHMRING_MAX_SLOTS) {
        return -1;
    }

    size_t size = sizeof(ShmRingHeader) + slots * sizeof(PipeMsg);

    // O_EXCL ensures that we are the exclusive creator of the object.
    int fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, 0666);
    if (fd < 0) {
        perror("shm_open");
        return -1;
    }

    if (ftruncate(fd, size) != 0) {
        perror("ftruncate");
        close(fd);
        shm_unlink(name);
        return -1;
    }

    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd); // fd can be closed after mmap without affecting the mapping
    if (mem == MAP_FAILED) {
        perror("mmap");
        shm_unlink(name);
        return -1;
    }

    // Initialize ring structure
    ring->hdr = (ShmRingHeader *)mem;
    ring->slots = (PipeMsg *)((uint8_t *)mem + sizeof(ShmRingHeader));
    ring->map_size = size;
    strncpy(ring->name, name, sizeof(ring->name) - 1);
    ring->name[sizeof(ring->name) - 1] = '\0';

    // Initialize header fields
    ring->hdr->magic = SHMRING_MAGIC;
    ring->hdr->slot_count = slots;
    atomic_init(&ring->hdr->seq, 0);
    atomic_init(&ring->hdr->write_idx, 0);

    return 0;
}

int shmring_open(ShmRing *ring, const char *name)
{
    if (!ring || !name) {
        return -1;
    }

    int fd = shm_open(name, O_RDWR, 0666);
    if (fd < 0) {
        perror("shm_open");
        return -1;
    }

    // First, map only the header to read the actual size
    void *header_mem = mmap(NULL, sizeof(ShmRingHeader), PROT_READ, MAP_SHARED, fd, 0);
    if (header_mem == MAP_FAILED) {
        perror("mmap header");
        close(fd);
        return -1;
    }
    
    ShmRingHeader *hdr = (ShmRingHeader *)header_mem;
    if (hdr->magic != SHMRING_MAGIC) {
        fprintf(stderr, "Invalid shmring magic number.\n");
        munmap(header_mem, sizeof(ShmRingHeader));
        close(fd);
        return -1;
    }
    uint32_t slots = hdr->slot_count;
    munmap(header_mem, sizeof(ShmRingHeader));

    size_t actual_size = sizeof(ShmRingHeader) + slots * sizeof(PipeMsg);

    // Now map the full region
    void *mem = mmap(NULL, actual_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mem == MAP_FAILED) {
        perror("mmap full");
        return -1;
    }

    ring->hdr = (ShmRingHeader *)mem;
    ring->slots = (PipeMsg *)((uint8_t *)mem + sizeof(ShmRingHeader));
    ring->map_size = actual_size;
    strncpy(ring->name, name, sizeof(ring->name) - 1);
    ring->name[sizeof(ring->name) - 1] = '\0';
    
    return 0;
}

void shmring_close(ShmRing *ring)
{
    if (ring && ring->hdr) {
        munmap(ring->hdr, ring->map_size);
        ring->hdr = NULL;
        ring->slots = NULL;
    }
}

int shmring_destroy(ShmRing *ring)
{
    if (!ring || ring->name[0] == '\0') {
        return -1;
    }
    shmring_close(ring);
    return shm_unlink(ring->name);
}


void shmring_write(ShmRing *ring, const PipeMsg *msg)
{
    // Increment sequence to start write (odd number)
    // This signals to readers that a write is in progress.
    atomic_fetch_add_explicit(&ring->hdr->seq, 1, memory_order_release);

    // Get the next slot index
    uint32_t idx = atomic_load_explicit(&ring->hdr->write_idx, memory_order_relaxed);
    
    // Write the data to the slot
    ring->slots[idx] = *msg;

    // Move write index to the next slot for the *next* write
    atomic_store_explicit(&ring->hdr->write_idx, (idx + 1) % ring->hdr->slot_count, memory_order_relaxed);

    // Increment sequence again to finish write (even number)
    // This signals to readers that the write is complete.
    atomic_fetch_add_explicit(&ring->hdr->seq, 1, memory_order_release);
}

bool shmring_read_latest(ShmRing *ring, PipeMsg *out)
{
    unsigned int seq1, seq2;
    uint32_t idx;

    do {
        seq1 = atomic_load_explicit(&ring->hdr->seq, memory_order_acquire);

        // If seq is odd, a write is in progress, so retry.
        if (seq1 & 1) {
            continue;
        }

        // Get the last written index
        idx = atomic_load_explicit(&ring->hdr->write_idx, memory_order_relaxed);
        
        // If nothing has been written yet, ring is empty
        if (seq1 == 0 && idx == 0) {
            return false;
        }

        // The latest message is at the previous index
        uint32_t read_idx = (idx == 0) ? (ring->hdr->slot_count - 1) : (idx - 1);

        // Read the data from the slot
        *out = ring->slots[read_idx];

        // Check the sequence number again
        seq2 = atomic_load_explicit(&ring->hdr->seq, memory_order_acquire);

    } while (seq1 != seq2); // If they differ, a write happened during our read, so retry.

    return true;
}
