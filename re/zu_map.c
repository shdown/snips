#define _GNU_SOURCE
#include "zu_map.h"
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

void
zu_map_init(Zu_Map *m)
{
    m->length = 1ULL << 40;
    while ((m->ptr = mmap(NULL, m->length, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0))
            == MAP_FAILED)
    {
        if (errno != EINVAL && errno != ENOMEM) {
            perror("mmap");
            abort();
        }
        m->length /= 2;
    }
    fprintf(stderr, "I: allocated Zu_Map of length %zu.\n", m->length);
}

size_t
zu_map_get(Zu_Map *m, size_t key)
{
    return m->ptr[key] - 1;
}

void
zu_map_set(Zu_Map *m, size_t key, size_t value)
{
    m->ptr[key] = value + 1;
}

size_t
zu_map_get_or_set(Zu_Map *m, size_t key, size_t value)
{
    const size_t old_value = m->ptr[key];
    if (!old_value) {
        m->ptr[key] = value + 1;
        return value;
    }
    return old_value - 1;
}

void
zu_map_free(Zu_Map *m)
{
    munmap(m->ptr, m->length);
}
