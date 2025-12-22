#include "shm_ring.h"

#include <errno.h>
#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// Helper to round up to the next power of two
static size_t roundup_pow_of_two(size_t n) {
  if (n == 0) return 2;  // Minimum size of 2
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  if (sizeof(size_t) == 8) {  // 64-bit
    n |= n >> 32;
  }
  return n + 1;
}

shm_ring_t* shm_ring_init(const char* name, size_t count, size_t item_size,
                          bool is_producer) {
  int fd;
  size_t real_count = roundup_pow_of_two(count);
  size_t total_size = sizeof(shm_ring_t) + (real_count * item_size);

  if (is_producer) {
    shm_unlink(name);
    fd = shm_open(name, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
      perror("[shm_ring] shm_open failed");
      return NULL;
    }
    if (ftruncate(fd, total_size) < 0) {
      perror("[shm_ring] ftruncate failed");
      close(fd);
      shm_unlink(name);
      return NULL;
    }
  } else {
    int retries = 5;
    while ((fd = shm_open(name, O_RDWR, 0666)) < 0 && retries-- > 0) {
      usleep(100000);  // Wait 100ms for producer to create it
    }
    if (fd < 0) {
      perror("[shm_ring] shm_open for consumer failed");
      return NULL;
    }
  }

  void* ptr = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  close(fd);

  if (ptr == MAP_FAILED) {
    perror("[shm_ring] mmap failed");
    if (is_producer) shm_unlink(name);
    return NULL;
  }

  shm_ring_t* ring = (shm_ring_t*)ptr;

  if (is_producer) {
    atomic_init(&ring->head, 0);
    atomic_init(&ring->tail, 0);
    ring->size = real_count;
    ring->mask = real_count - 1;
    ring->item_size = item_size;
  } else {
    // Consumer verifies the configuration
    if (ring->size != real_count || ring->item_size != item_size) {
      fprintf(
          stderr,
          "[shm_ring] Configuration mismatch between producer and consumer.\n");
      munmap(ptr, total_size);
      return NULL;
    }
  }

  return ring;
}

void shm_ring_free(shm_ring_t* ring, const char* name, bool is_producer) {
  if (!ring) return;
  size_t total_size = sizeof(shm_ring_t) + (ring->size * ring->item_size);
  munmap(ring, total_size);

  if (is_producer && name) {
    shm_unlink(name);
  }
}

bool shm_ring_push(shm_ring_t* ring, const void* data) {
  const size_t head = atomic_load_explicit(&ring->head, memory_order_relaxed);
  const size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);

  if ((head - tail) >= ring->size) {
    return false;  // Buffer is full
  }

  const size_t offset = (head & ring->mask) * ring->item_size;
  memcpy(ring->data + offset, data, ring->item_size);

  atomic_store_explicit(&ring->head, head + 1, memory_order_release);
  return true;
}

bool shm_ring_pop(shm_ring_t* ring, void* data) {
  const size_t tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
  const size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);

  if (tail >= head) {
    return false;  // Buffer is empty
  }

  const size_t offset = (tail & ring->mask) * ring->item_size;
  memcpy(data, ring->data + offset, ring->item_size);

  atomic_store_explicit(&ring->tail, tail + 1, memory_order_release);
  return true;
}

size_t shm_ring_count(shm_ring_t* ring) {
  const size_t head = atomic_load_explicit(&ring->head, memory_order_acquire);
  const size_t tail = atomic_load_explicit(&ring->tail, memory_order_acquire);
  return head - tail;
}
