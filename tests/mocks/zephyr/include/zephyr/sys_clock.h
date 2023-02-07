#ifndef __SYS_CLOCK_H__
#define __SYS_CLOCK_H__

typedef uint32_t k_ticks_t;

typedef struct {
    k_ticks_t ticks;
} k_timeout_t;

#endif /* __SYS_CLOCK_H__ */
