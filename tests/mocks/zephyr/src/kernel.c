#include <zephyr/kernel.h>

k_tid_t
k_thread_create(struct k_thread * new_thread,
                k_thread_stack_t *stack,
                size_t            stack_size,
                k_thread_entry_t  entry,
                void *            p1,
                void *            p2,
                void *            p3,
                int               prio,
                uint32_t          options,
                k_timeout_t       delay) {
    return NULL;
}

int
k_thread_name_set(k_tid_t thread, const char *str) {
    return 0;
}

void
k_thread_abort(k_tid_t thread) {
}

int
k_mutex_init(struct k_mutex *mutex) {
    return 0;
}

int
k_mutex_lock(struct k_mutex *mutex, k_timeout_t timeout) {
    return 0;
}

int
k_mutex_unlock(struct k_mutex *mutex) {
    return 0;
}

int64_t
k_uptime_get(void) {
    return 0;
}

int32_t
k_msleep(int32_t ms) {
    return 0;
}
