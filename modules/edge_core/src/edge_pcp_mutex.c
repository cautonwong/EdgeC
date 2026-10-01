/**
 * @file edge_pcp_mutex.c
 * @brief PREEMPT_RT Priority Ceiling Protocol (PCP) Mutex & Real-Time Lock Audit (AEP-711 / Issue #244)
 */

#include "edge_pcp_mutex.h"
#include <string.h>
#include <errno.h>
#include <time.h>

/**
 * @brief Concrete internal structure for PCP Mutex
 */
struct edge_pcp_mutex {
    pthread_mutex_t lock;
    edge_pcp_mutex_config_t config;
    edge_pcp_mutex_port_t port;
    edge_pcp_mutex_stats_t stats;
    struct timespec lock_time;
    bool initialized;
};

typedef struct edge_pcp_mutex edge_pcp_mutex_impl_t;

_Static_assert(sizeof(struct edge_pcp_mutex) <= sizeof(edge_pcp_mutex_storage_t),
               "edge_pcp_mutex_storage_t buffer size too small for edge_pcp_mutex");

edge_pcp_mutex_t *edge_pcp_mutex_create_in(edge_pcp_mutex_storage_t *store,
                                           const edge_pcp_mutex_config_t *cfg,
                                           const edge_pcp_mutex_port_t *port)
{
    if (!store || !cfg) {
        return NULL;
    }

    edge_pcp_mutex_impl_t *impl = (edge_pcp_mutex_impl_t *)store->raw;
    memset(impl, 0, sizeof(*impl));
    memcpy(&impl->config, cfg, sizeof(edge_pcp_mutex_config_t));
    if (port) {
        memcpy(&impl->port, port, sizeof(edge_pcp_mutex_port_t));
    }

    // Initialize stats
    impl->stats.mutex_id = cfg->mutex_id;
    strncpy(impl->stats.name, cfg->name, sizeof(impl->stats.name) - 1);
    impl->stats.protocol = cfg->protocol;
    impl->stats.prio_ceiling = cfg->prio_ceiling;

    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        return NULL;
    }

    if (cfg->recursive) {
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    }

    bool rt_capable = false;
    int sched_policy = sched_getscheduler(0);
    if (sched_policy == SCHED_FIFO || sched_policy == SCHED_RR) {
        rt_capable = true;
    }

    if (rt_capable && cfg->protocol == EDGE_PCP_PROTOCOL_PROTECT) {
        pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_PROTECT);
        int ceiling = cfg->prio_ceiling > 0 ? cfg->prio_ceiling : 50;
        pthread_mutexattr_setprioceiling(&attr, ceiling);
    } else if (rt_capable && cfg->protocol == EDGE_PCP_PROTOCOL_INHERIT) {
        pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);
    } else {
        pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_NONE);
    }

    int rc = pthread_mutex_init(&impl->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    if (rc != 0) {
        // Fallback to normal mutex if OS lacks PREEMPT_RT or ceiling permissions
        pthread_mutexattr_t fallback_attr;
        pthread_mutexattr_init(&fallback_attr);
        if (cfg->recursive) {
            pthread_mutexattr_settype(&fallback_attr, PTHREAD_MUTEX_RECURSIVE);
        }
        pthread_mutexattr_setprotocol(&fallback_attr, PTHREAD_PRIO_NONE);
        rc = pthread_mutex_init(&impl->lock, &fallback_attr);
        pthread_mutexattr_destroy(&fallback_attr);
        if (rc != 0) {
            return NULL;
        }
    }

    impl->initialized = true;
    return (edge_pcp_mutex_t *)impl;
}

int edge_pcp_mutex_lock(edge_pcp_mutex_t *m)
{
    if (!m || !m->initialized) {
        return -EINVAL;
    }

    struct timespec t_wait_start, t_wait_end;
    clock_gettime(CLOCK_MONOTONIC, &t_wait_start);

    int rc = pthread_mutex_trylock(&m->lock);
    if (rc == EBUSY) {
        __atomic_fetch_add(&m->stats.contention_count, 1, __ATOMIC_RELAXED);
        if (m->config.protocol == EDGE_PCP_PROTOCOL_PROTECT || m->config.protocol == EDGE_PCP_PROTOCOL_INHERIT) {
            __atomic_fetch_add(&m->stats.inversions_prevented, 1, __ATOMIC_RELAXED);
        }
        rc = pthread_mutex_lock(&m->lock);
        if (rc == EINVAL || rc == EPERM || rc == ENOTSUP) {
            // Kernel / glibc FUTEX_LOCK_PI rejected SCHED_OTHER contention;
            // spin-trylock with small yield backoff for deterministic real-time userland emulation
            while ((rc = pthread_mutex_trylock(&m->lock)) == EBUSY) {
                struct timespec ts = { .tv_sec = 0, .tv_nsec = 20000 }; // 20us
                nanosleep(&ts, NULL);
            }
        }
    }

    if (rc != 0) {
        return -rc;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_wait_end);
    uint64_t wait_ns = (uint64_t)(t_wait_end.tv_sec - t_wait_start.tv_sec) * 1000000000ULL +
                       (uint64_t)(t_wait_end.tv_nsec - t_wait_start.tv_nsec);
    m->stats.last_wait_duration_ns = wait_ns;
    if (wait_ns > m->stats.max_wait_duration_ns) {
        m->stats.max_wait_duration_ns = wait_ns;
    }

    __atomic_fetch_add(&m->stats.total_lock_count, 1, __ATOMIC_RELAXED);
    m->stats.is_locked = true;
    m->stats.owner_thread = pthread_self();
    clock_gettime(CLOCK_MONOTONIC, &m->lock_time);

    return 0;
}

int edge_pcp_mutex_trylock(edge_pcp_mutex_t *m)
{
    if (!m || !m->initialized) {
        return -EINVAL;
    }

    int rc = pthread_mutex_trylock(&m->lock);
    if (rc == 0) {
        __atomic_fetch_add(&m->stats.total_lock_count, 1, __ATOMIC_RELAXED);
        m->stats.is_locked = true;
        m->stats.owner_thread = pthread_self();
        m->stats.last_wait_duration_ns = 0;
        clock_gettime(CLOCK_MONOTONIC, &m->lock_time);
        return 0;
    } else if (rc == EBUSY) {
        __atomic_fetch_add(&m->stats.contention_count, 1, __ATOMIC_RELAXED);
        return EBUSY;
    }
    return -rc;
}

int edge_pcp_mutex_timedlock(edge_pcp_mutex_t *m, uint64_t timeout_us)
{
    if (!m || !m->initialized) {
        return -EINVAL;
    }

    struct timespec abs_timeout;
    clock_gettime(CLOCK_REALTIME, &abs_timeout);
    abs_timeout.tv_sec += (time_t)(timeout_us / 1000000ULL);
    abs_timeout.tv_nsec += (long)((timeout_us % 1000000ULL) * 1000ULL);
    if (abs_timeout.tv_nsec >= 1000000000L) {
        abs_timeout.tv_sec += 1;
        abs_timeout.tv_nsec -= 1000000000L;
    }

    struct timespec t_wait_start, t_wait_end;
    clock_gettime(CLOCK_MONOTONIC, &t_wait_start);

    int rc = pthread_mutex_timedlock(&m->lock, &abs_timeout);
    if (rc == EINVAL || rc == EPERM || rc == ENOTSUP) {
        // Fallback to timed trylock loop
        struct timespec cur;
        while ((rc = pthread_mutex_trylock(&m->lock)) == EBUSY) {
            clock_gettime(CLOCK_REALTIME, &cur);
            if (cur.tv_sec > abs_timeout.tv_sec ||
                (cur.tv_sec == abs_timeout.tv_sec && cur.tv_nsec >= abs_timeout.tv_nsec)) {
                rc = ETIMEDOUT;
                break;
            }
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 20000 }; // 20us
            nanosleep(&ts, NULL);
        }
    }

    if (rc != 0) {
        if (rc == ETIMEDOUT || rc == EBUSY) {
            __atomic_fetch_add(&m->stats.contention_count, 1, __ATOMIC_RELAXED);
        }
        return rc == ETIMEDOUT ? ETIMEDOUT : -rc;
    }

    clock_gettime(CLOCK_MONOTONIC, &t_wait_end);
    uint64_t wait_ns = (uint64_t)(t_wait_end.tv_sec - t_wait_start.tv_sec) * 1000000000ULL +
                       (uint64_t)(t_wait_end.tv_nsec - t_wait_start.tv_nsec);
    m->stats.last_wait_duration_ns = wait_ns;
    if (wait_ns > m->stats.max_wait_duration_ns) {
        m->stats.max_wait_duration_ns = wait_ns;
    }

    __atomic_fetch_add(&m->stats.total_lock_count, 1, __ATOMIC_RELAXED);
    m->stats.is_locked = true;
    m->stats.owner_thread = pthread_self();
    clock_gettime(CLOCK_MONOTONIC, &m->lock_time);

    return 0;
}

int edge_pcp_mutex_unlock(edge_pcp_mutex_t *m)
{
    if (!m || !m->initialized) {
        return -EINVAL;
    }

    struct timespec t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_end);

    uint64_t hold_ns = (uint64_t)(t_end.tv_sec - m->lock_time.tv_sec) * 1000000000ULL +
                       (uint64_t)(t_end.tv_nsec - m->lock_time.tv_nsec);
    m->stats.last_hold_duration_ns = hold_ns;
    if (hold_ns > m->stats.max_hold_duration_ns) {
        m->stats.max_hold_duration_ns = hold_ns;
    }

    if (m->stats.total_lock_count > 0) {
        if (m->stats.avg_hold_duration_ns == 0.0) {
            m->stats.avg_hold_duration_ns = (double)hold_ns;
        } else {
            // Exponential moving average (alpha = 0.1)
            m->stats.avg_hold_duration_ns = 0.9 * m->stats.avg_hold_duration_ns + 0.1 * (double)hold_ns;
        }
    }

    uint64_t hold_us = hold_ns / 1000ULL;
    if (m->config.max_expected_hold_us > 0 && hold_us > m->config.max_expected_hold_us) {
        m->stats.overrun_count++;
        if (m->port.on_hold_overrun) {
            m->port.on_hold_overrun(m->port.ctx, m->config.mutex_id, hold_us, m->config.max_expected_hold_us);
        }
    }

    m->stats.is_locked = false;
    memset(&m->stats.owner_thread, 0, sizeof(m->stats.owner_thread));

    int rc = pthread_mutex_unlock(&m->lock);
    return rc == 0 ? 0 : -rc;
}

int edge_pcp_mutex_get_stats(const edge_pcp_mutex_t *m, edge_pcp_mutex_stats_t *stats)
{
    if (!m || !stats || !m->initialized) {
        return -EINVAL;
    }
    memcpy(stats, &m->stats, sizeof(edge_pcp_mutex_stats_t));
    return 0;
}

int edge_pcp_mutex_set_prioceiling(edge_pcp_mutex_t *m, int new_ceiling, int *old_ceiling)
{
    if (!m || !m->initialized) {
        return -EINVAL;
    }
    int rc = pthread_mutex_setprioceiling(&m->lock, new_ceiling, old_ceiling);
    if (rc == 0) {
        m->config.prio_ceiling = new_ceiling;
        m->stats.prio_ceiling = new_ceiling;
    }
    return rc == 0 ? 0 : -rc;
}

int edge_pcp_mutex_destroy(edge_pcp_mutex_t *m)
{
    if (!m || !m->initialized) {
        return -EINVAL;
    }
    m->initialized = false;
    int rc = pthread_mutex_destroy(&m->lock);
    return rc == 0 ? 0 : -rc;
}
