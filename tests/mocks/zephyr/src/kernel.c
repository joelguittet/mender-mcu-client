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

int
k_sem_init(struct k_sem *sem, unsigned int initial_count, unsigned int limit) {
    return 0;
}

int
k_sem_take(struct k_sem *sem, k_timeout_t timeout) {
    return 0;
}

void
k_sem_give(struct k_sem *sem) {
}

void
k_timer_init(struct k_timer *timer, k_timer_expiry_t expiry_fn, k_timer_stop_t stop_fn) {
}

void
k_timer_user_data_set(struct k_timer *timer, void *user_data) {
}

void *
k_timer_user_data_get(const struct k_timer *timer) {
    return NULL;
}

void
k_timer_start(struct k_timer *timer, k_timeout_t duration, k_timeout_t period) {
}

void
k_timer_stop(struct k_timer *timer) {
}

void
k_work_init(struct k_work *work, k_work_handler_t handler) {
}

int
k_work_submit_to_queue(struct k_work_q *queue, struct k_work *work) {
    return 0;
}

void
k_work_queue_init(struct k_work_q *queue) {
}

void
k_work_queue_start(struct k_work_q *queue, k_thread_stack_t *stack, size_t stack_size, int prio, const struct k_work_queue_config *cfg) {
}

int64_t
k_uptime_get(void) {
    return 0;
}

int32_t
k_msleep(int32_t ms) {
    return 0;
}
