#ifndef __KERNEL_H__
#define __KERNEL_H__

#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys_clock.h>

struct k_thread {
    void *dummy;
};
struct k_mutex {
    void *dummy;
};

typedef struct k_thread *k_tid_t;

typedef char *k_thread_stack_t;

typedef void (*k_thread_entry_t)(void *p1, void *p2, void *p3);

#define K_USER 4

#define K_NO_WAIT ((k_timeout_t) { .ticks = (0) })
#define K_FOREVER ((k_timeout_t) { .ticks = (-1) })

#define K_MSEC(ms) ((k_timeout_t) { .ticks = (ms) })

#define K_THREAD_STACK_DEFINE(sym, size) k_thread_stack_t *sym;

k_tid_t k_thread_create(struct k_thread * new_thread,
                        k_thread_stack_t *stack,
                        size_t            stack_size,
                        k_thread_entry_t  entry,
                        void *            p1,
                        void *            p2,
                        void *            p3,
                        int               prio,
                        uint32_t          options,
                        k_timeout_t       delay);
int     k_thread_name_set(k_tid_t thread, const char *str);
void    k_thread_abort(k_tid_t thread);

int k_mutex_init(struct k_mutex *mutex);
int k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout);
int k_mutex_unlock(struct k_mutex *mutex);

int64_t k_uptime_get(void);
int32_t k_msleep(int32_t ms);

#endif /* __KERNEL_H__ */
