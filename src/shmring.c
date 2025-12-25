#include "shmring.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>

// Ensure the header struct is exactly one cache line
_Static_assert(sizeof(ShmRingHeader) == CACHELINE_SIZE, "ShmRingHeader must be one cache line size");

int shmring_create(shm_ring_t *ring, const char *name, uint32_t slots)
{
    if (!ring || !name || slots == 0 || slots > SHMRING_MAX_SLOTS) {
        return -1;
    }

    size_t size = sizeof(ShmRingHeader) + slots * sizeof(PipeMsg);

    int fd = shm_open(name, O_CREAT | O_RDWR | O_EXCL, 0666);
    if (fd < 0) {
        // Try opening to see if it exists, maybe unlink?
        // For now, standard error behavior.
        // perror("shm_open create"); 
        return -1;
    }

    if (ftruncate(fd, size) != 0) {
        close(fd);
        shm_unlink(name);
        return -1;
    }

    void *mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mem == MAP_FAILED) {
        shm_unlink(name);
        return -1;
    }

    ring->hdr = (ShmRingHeader *)mem;
    ring->slots = (PipeMsg *)((uint8_t *)mem + sizeof(ShmRingHeader));
    ring->map_size = size;
    strncpy(ring->name, name, sizeof(ring->name) - 1);
    ring->name[sizeof(ring->name) - 1] = '\0';

    ring->hdr->magic = SHMRING_MAGIC;
    ring->hdr->slot_count = slots;
    atomic_init(&ring->hdr->seq, 0);
    atomic_init(&ring->hdr->write_idx, 0);
    atomic_init(&ring->hdr->total_writes, 0);

    return 0;
}

int shmring_open(shm_ring_t *ring, const char *name)
{
    if (!ring || !name) {
        return -1;
    }

    int fd = shm_open(name, O_RDWR, 0666);
    if (fd < 0) {
        return -1;
    }

    void *header_mem = mmap(NULL, sizeof(ShmRingHeader), PROT_READ, MAP_SHARED, fd, 0);
    if (header_mem == MAP_FAILED) {
        close(fd);
        return -1;
    }
    
    ShmRingHeader *hdr = (ShmRingHeader *)header_mem;
    if (hdr->magic != SHMRING_MAGIC) {
        munmap(header_mem, sizeof(ShmRingHeader));
        close(fd);
        return -1;
    }
    uint32_t slots = hdr->slot_count;
    munmap(header_mem, sizeof(ShmRingHeader));

    size_t actual_size = sizeof(ShmRingHeader) + slots * sizeof(PipeMsg);

    void *mem = mmap(NULL, actual_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mem == MAP_FAILED) {
        return -1;
    }

    ring->hdr = (ShmRingHeader *)mem;
    ring->slots = (PipeMsg *)((uint8_t *)mem + sizeof(ShmRingHeader));
    ring->map_size = actual_size;
    strncpy(ring->name, name, sizeof(ring->name) - 1);
    ring->name[sizeof(ring->name) - 1] = '\0';
    
    return 0;
}

void shmring_close(shm_ring_t *ring)
{
    if (ring && ring->hdr) {
        munmap(ring->hdr, ring->map_size);
        ring->hdr = NULL;
        ring->slots = NULL;
    }
}

int shmring_destroy(shm_ring_t *ring)
{
    if (!ring || ring->name[0] == '\0') {
        return -1;
    }
    shmring_close(ring);
    return shm_unlink(ring->name);
}

void shm_ring_write(shm_ring_t *ring, const PipeMsg *msg)
{
    atomic_fetch_add_explicit(&ring->hdr->seq, 1, memory_order_release);

    uint32_t idx = atomic_load_explicit(&ring->hdr->write_idx, memory_order_relaxed);
    
    ring->slots[idx] = *msg;

    atomic_store_explicit(&ring->hdr->write_idx, (idx + 1) % ring->hdr->slot_count, memory_order_relaxed);
    
    // Increment total_writes strictly after data write
    atomic_fetch_add_explicit(&ring->hdr->total_writes, 1, memory_order_relaxed);

    atomic_fetch_add_explicit(&ring->hdr->seq, 1, memory_order_release);
}

int shm_ring_read(shm_ring_t *ring, uint64_t *last_seq, PipeMsg *out)
{
    uint64_t total = atomic_load_explicit(&ring->hdr->total_writes, memory_order_acquire);
    
    if (*last_seq >= total) {
        return -1; // Up to date
    }

    // Determine which message to read.
    // Since this is a ring, we might have been overwritten if we are too slow.
    uint64_t target_seq = *last_seq;
    uint32_t slots = ring->hdr->slot_count;
    
    if (total - target_seq > slots) {
        // We are too slow, jump to the oldest available data
        target_seq = total - slots;
        // Optionally notify user of dropped frames?
    }

    // Calculate index for target_seq
    // Since we start at 0, and total_writes increments after write, 
    // the msg at 'total_writes=1' was written at index 0.
    // So if total_writes is T, valid indices are (T-slots) to (T-1).
    // The slot index for message #N (0-based) is N % slots.
    uint32_t idx = target_seq % slots;

    // Use seqlock to read safely
    unsigned int seq1, seq2;
    do {
        seq1 = atomic_load_explicit(&ring->hdr->seq, memory_order_acquire);
        if (seq1 & 1) continue; // Writer active

        *out = ring->slots[idx];

        seq2 = atomic_load_explicit(&ring->hdr->seq, memory_order_acquire);
    } while (seq1 != seq2);

    // Update the caller's sequence
    *last_seq = target_seq + 1;
    return 0;
}