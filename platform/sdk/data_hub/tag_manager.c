#include "tag_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Simple Hash Map Implementation ---

typedef struct HashNode {
  char *key;
  uint32_t value;
  struct HashNode *next;
} HashNode;

typedef struct {
  HashNode **buckets;
  size_t capacity;
  size_t count;
  uint32_t next_id;
  pthread_rwlock_t lock;
} HashMap;

// --- String to ID mapping ---
static HashMap g_name_to_id_map;
// --- ID to String mapping (simple array lookup) ---
static char **g_id_to_name_map = NULL;
static size_t g_id_map_capacity = 0;

// FNV-1a hash function
static uint32_t fnv1a_hash(const char *str) {
  uint32_t hash = 0x811c9dc5;
  while (*str) {
    hash ^= (uint8_t)*str++;
    hash *= 0x01000193;
  }
  return hash;
}

static int hash_map_init(HashMap *map, size_t capacity) {
  map->capacity = capacity;
  map->count = 0;
  map->next_id = 1;  // ID 0 is reserved for invalid/failure
  map->buckets = calloc(capacity, sizeof(HashNode *));
  if (!map->buckets) return -1;
  pthread_rwlock_init(&map->lock, NULL);
  return 0;
}

static void hash_map_cleanup(HashMap *map) {
  for (size_t i = 0; i < map->capacity; i++) {
    HashNode *node = map->buckets[i];
    while (node) {
      HashNode *temp = node;
      node = node->next;
      free(temp->key);
      free(temp);
    }
  }
  free(map->buckets);
  pthread_rwlock_destroy(&map->lock);
}

// --- Tag Manager Public API ---

int tag_manager_init(size_t initial_capacity) {
  g_id_map_capacity = initial_capacity > 0 ? initial_capacity : 1024;
  g_id_to_name_map = calloc(g_id_map_capacity, sizeof(char *));
  if (!g_id_to_name_map) return -1;
  return hash_map_init(&g_name_to_id_map, g_id_map_capacity);
}

void tag_manager_cleanup(void) {
  hash_map_cleanup(&g_name_to_id_map);
  // Keys are freed in hash_map_cleanup, so we just free the array itself
  free(g_id_to_name_map);
}

uint32_t tag_manager_get_id(const char *tag_name) {
  if (!tag_name) return 0;

  uint32_t hash = fnv1a_hash(tag_name);
  size_t index = hash % g_name_to_id_map.capacity;

  // First, try a read lock for performance
  pthread_rwlock_rdlock(&g_name_to_id_map.lock);
  HashNode *node = g_name_to_id_map.buckets[index];
  while (node) {
    if (strcmp(node->key, tag_name) == 0) {
      uint32_t id = node->value;
      pthread_rwlock_unlock(&g_name_to_id_map.lock);
      return id;
    }
    node = node->next;
  }
  pthread_rwlock_unlock(&g_name_to_id_map.lock);

  // Not found, need a write lock to insert
  pthread_rwlock_wrlock(&g_name_to_id_map.lock);

  // Double-check, another thread might have added it while we were waiting for
  // the lock
  node = g_name_to_id_map.buckets[index];
  while (node) {
    if (strcmp(node->key, tag_name) == 0) {
      uint32_t id = node->value;
      pthread_rwlock_unlock(&g_name_to_id_map.lock);
      return id;
    }
    node = node->next;
  }

  // Still not found, create it
  HashNode *new_node = malloc(sizeof(HashNode));
  if (!new_node) {
    pthread_rwlock_unlock(&g_name_to_id_map.lock);
    return 0;  // Failure
  }
  new_node->key = strdup(tag_name);
  new_node->value = g_name_to_id_map.next_id++;

  // Insert into hash map
  new_node->next = g_name_to_id_map.buckets[index];
  g_name_to_id_map.buckets[index] = new_node;
  g_name_to_id_map.count++;

  // Insert into reverse map (expand if necessary)
  if (new_node->value >= g_id_map_capacity) {
    size_t new_cap = g_id_map_capacity * 2;
    char **new_map = realloc(g_id_to_name_map, new_cap * sizeof(char *));
    if (!new_map) {  // realloc failed, major issue but try to continue
      // We can't store the reverse mapping, but the forward one works
    } else {
      g_id_to_name_map = new_map;
      g_id_map_capacity = new_cap;
    }
  }
  if (new_node->value < g_id_map_capacity) {
    g_id_to_name_map[new_node->value] = new_node->key;
  }

  uint32_t new_id = new_node->value;
  pthread_rwlock_unlock(&g_name_to_id_map.lock);

  return new_id;
}

const char *tag_manager_get_name(uint32_t tag_id) {
  // Read lock is sufficient here
  pthread_rwlock_rdlock(&g_name_to_id_map.lock);
  if (tag_id > 0 && tag_id < g_name_to_id_map.next_id) {
    const char *name = g_id_to_name_map[tag_id];
    pthread_rwlock_unlock(&g_name_to_id_map.lock);
    return name;
  }
  pthread_rwlock_unlock(&g_name_to_id_map.lock);
  return NULL;
}
