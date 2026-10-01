/**
 * @file edge_cpu_affinity.c
 * @brief CPU Core Affinity Isolation, IRQ Tuning, and Memory Locking (AEP-716 / Issue #247)
 */

#define _GNU_SOURCE
#include "edge_cpu_affinity.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#define EDGE_PAGE_SIZE 4096

struct edge_cpu_affinity {
    edge_cpu_affinity_config_t config;
    edge_cpu_affinity_port_t port;
    edge_cpu_topology_t topology;
    edge_pinned_thread_info_t pinned_threads[EDGE_MAX_PINNED_THREADS];
    uint32_t pinned_thread_count;
    uint32_t irqs_steered_count;
    bool memory_locked;
    size_t prefaulted_bytes;
    pthread_mutex_t lock;
    bool initialized;
};

_Static_assert(sizeof(struct edge_cpu_affinity) <= sizeof(edge_cpu_affinity_storage_t),
               "edge_cpu_affinity_storage_t size is insufficient for struct edge_cpu_affinity");

static void detect_cpu_topology(edge_cpu_topology_t *topo) {
    if (!topo) return;
    memset(topo, 0x00, sizeof(*topo));

    long nproc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nproc <= 0) {
        nproc = 1;
    }
    topo->total_online_cores = (uint32_t)nproc;

    uint64_t mask = 0;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if (sched_getaffinity(0, sizeof(cpuset), &cpuset) == 0) {
        for (long i = 0; i < nproc && i < 64; i++) {
            if (CPU_ISSET(i, &cpuset)) {
                mask |= (1ULL << (uint64_t)i);
            }
        }
    } else {
        for (long i = 0; i < nproc && i < 64; i++) {
            mask |= (1ULL << (uint64_t)i);
        }
    }
    topo->available_core_mask = mask;
    topo->numa_nodes_count = 1;

    // Detect isolated cores from /sys/devices/system/cpu/isolated if accessible
    FILE *fp = fopen("/sys/devices/system/cpu/isolated", "r");
    if (fp) {
        char buf[64] = {0};
        if (fgets(buf, sizeof(buf), fp)) {
            // e.g. "2,3" or "2-3"
            uint32_t start = 0, end = 0;
            if (sscanf(buf, "%u-%u", &start, &end) == 2) {
                for (uint32_t c = start; c <= end && c < 64; c++) {
                    topo->isolated_core_mask |= (1ULL << (uint64_t)c);
                }
            } else if (sscanf(buf, "%u", &start) == 1) {
                topo->isolated_core_mask |= (1ULL << (uint64_t)start);
            }
        }
        fclose(fp);
    }
}

static void prefault_stack_pages(size_t size) {
    if (size == 0) return;
    // Pre-fault stack space by touching each page
    size_t num_pages = (size + EDGE_PAGE_SIZE - 1) / EDGE_PAGE_SIZE;
    if (num_pages > 256) num_pages = 256; // Limit to 1MB stack pre-fault

    volatile uint8_t *stack_ptr = (volatile uint8_t *)alloca(num_pages * EDGE_PAGE_SIZE);
    for (size_t i = 0; i < num_pages; i++) {
        stack_ptr[i * EDGE_PAGE_SIZE] = 0xA5;
    }
}

edge_cpu_affinity_t *edge_cpu_affinity_create_in(edge_cpu_affinity_storage_t *storage,
                                                 const edge_cpu_affinity_config_t *cfg,
                                                 const edge_cpu_affinity_port_t *port) {
    if (!storage || !cfg) {
        return NULL;
    }

    struct edge_cpu_affinity *aff = (struct edge_cpu_affinity *)storage->raw;
    memset(aff, 0x00, sizeof(*aff));
    aff->config = *cfg;
    if (port) {
        aff->port = *port;
    }

    pthread_mutex_init(&aff->lock, NULL);
    detect_cpu_topology(&aff->topology);

    // Apply memory locking if requested
    if (cfg->enable_mlockall) {
        edge_cpu_affinity_lock_memory(aff, cfg->pre_fault_stack_size);
    }

    // Apply IRQ steering if requested and housekeeping mask is provided
    if (cfg->auto_isolate_irqs && cfg->housekeeping_core_mask != 0) {
        uint32_t count = 0;
        edge_cpu_affinity_steer_irqs(aff, cfg->housekeeping_core_mask, &count);
    }

    aff->initialized = true;
    return aff;
}

int edge_cpu_affinity_destroy(edge_cpu_affinity_t *aff) {
    if (!aff || !aff->initialized) {
        return -EINVAL;
    }

    pthread_mutex_lock(&aff->lock);
    if (aff->memory_locked) {
        munlockall();
        aff->memory_locked = false;
    }
    aff->initialized = false;
    pthread_mutex_unlock(&aff->lock);
    pthread_mutex_destroy(&aff->lock);

    return 0;
}

int edge_cpu_affinity_get_topology(const edge_cpu_affinity_t *aff, edge_cpu_topology_t *topo) {
    if (!aff || !topo || !aff->initialized) {
        return -EINVAL;
    }
    *topo = aff->topology;
    return 0;
}

int edge_cpu_affinity_pin_current_thread(edge_cpu_affinity_t *aff, uint32_t core_id, const char *task_name) {
    return edge_cpu_affinity_pin_thread(aff, pthread_self(), core_id, task_name);
}

int edge_cpu_affinity_pin_thread(edge_cpu_affinity_t *aff, pthread_t thread, uint32_t core_id, const char *task_name) {
    if (!aff || !aff->initialized) {
        return -EINVAL;
    }
    if (core_id >= aff->topology.total_online_cores && aff->topology.total_online_cores > 0) {
        return -ERANGE;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        return -rc;
    }

    pthread_mutex_lock(&aff->lock);

    pid_t tid = (pid_t)syscall(SYS_gettid);
    // Record pinned thread
    bool updated = false;
    for (uint32_t i = 0; i < aff->pinned_thread_count; i++) {
        if (pthread_equal(aff->pinned_threads[i].thread_handle, thread)) {
            aff->pinned_threads[i].assigned_core_id = core_id;
            aff->pinned_threads[i].tid = tid;
            if (task_name) {
                strncpy(aff->pinned_threads[i].task_name, task_name, sizeof(aff->pinned_threads[i].task_name) - 1);
            }
            updated = true;
            break;
        }
    }

    if (!updated && aff->pinned_thread_count < EDGE_MAX_PINNED_THREADS) {
        edge_pinned_thread_info_t *info = &aff->pinned_threads[aff->pinned_thread_count++];
        info->thread_handle = thread;
        info->tid = tid;
        info->assigned_core_id = core_id;
        info->is_active = true;
        if (task_name) {
            strncpy(info->task_name, task_name, sizeof(info->task_name) - 1);
        }
    }

    pthread_mutex_unlock(&aff->lock);

    if (aff->port.on_core_pinned) {
        aff->port.on_core_pinned(aff->port.ctx, tid, core_id, task_name ? task_name : "unknown");
    }

    return 0;
}

int edge_cpu_affinity_set_mask_for_thread(edge_cpu_affinity_t *aff, pthread_t thread, uint64_t core_mask) {
    if (!aff || !aff->initialized || core_mask == 0) {
        return -EINVAL;
    }

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    for (uint32_t i = 0; i < 64; i++) {
        if (core_mask & (1ULL << (uint64_t)i)) {
            CPU_SET(i, &cpuset);
        }
    }

    int rc = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        return -rc;
    }

    return 0;
}

int edge_cpu_affinity_lock_memory(edge_cpu_affinity_t *aff, size_t prefault_bytes) {
    if (!aff) {
        return -EINVAL;
    }

    int rc = mlockall(MCL_CURRENT | MCL_FUTURE);
    int err = (rc != 0) ? errno : 0;

    pthread_mutex_lock(&aff->lock);
    if (rc == 0) {
        aff->memory_locked = true;
        if (prefault_bytes > 0) {
            prefault_stack_pages(prefault_bytes);
            aff->prefaulted_bytes = prefault_bytes;
        }
    } else {
        aff->memory_locked = false;
    }
    pthread_mutex_unlock(&aff->lock);

    if (aff->port.on_memlock_applied) {
        aff->port.on_memlock_applied(aff->port.ctx, (rc == 0), prefault_bytes, err);
    }

    return (rc == 0) ? 0 : -err;
}

int edge_cpu_affinity_steer_irqs(edge_cpu_affinity_t *aff, uint64_t target_mask, uint32_t *migrated_count) {
    if (!aff || target_mask == 0) {
        return -EINVAL;
    }

    uint32_t count = 0;
    DIR *dir = opendir("/proc/irq");
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
                uint32_t irq = (uint32_t)atoi(entry->d_name);
                char path[128];
                snprintf(path, sizeof(path), "/proc/irq/%u/smp_affinity", irq);

                // Attempt to write target affinity mask if writeable
                int fd = open(path, O_WRONLY);
                if (fd >= 0) {
                    char mask_str[32];
                    int len = snprintf(mask_str, sizeof(mask_str), "%llx\n", (unsigned long long)target_mask);
                    if (write(fd, mask_str, (size_t)len) > 0) {
                        count++;
                        if (aff->port.on_irq_migrated) {
                            aff->port.on_irq_migrated(aff->port.ctx, irq, 0, target_mask);
                        }
                    }
                    close(fd);
                }
            }
        }
        closedir(dir);
    }

    pthread_mutex_lock(&aff->lock);
    aff->irqs_steered_count += count;
    pthread_mutex_unlock(&aff->lock);

    if (migrated_count) {
        *migrated_count = count;
    }
    return 0;
}

int edge_cpu_affinity_get_stats(const edge_cpu_affinity_t *aff, edge_cpu_affinity_stats_t *stats) {
    if (!aff || !stats || !aff->initialized) {
        return -EINVAL;
    }

    memset(stats, 0x00, sizeof(*stats));
    stats->rt_core_mask = aff->config.rt_core_mask;
    stats->housekeeping_core_mask = aff->config.housekeeping_core_mask;
    stats->pinned_thread_count = aff->pinned_thread_count;
    stats->irqs_steered_count = aff->irqs_steered_count;
    stats->memory_locked = aff->memory_locked;
    stats->prefaulted_bytes = aff->prefaulted_bytes;

    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        stats->minor_page_faults = (uint64_t)usage.ru_minflt;
        stats->major_page_faults = (uint64_t)usage.ru_majflt;
    }

    return 0;
}
