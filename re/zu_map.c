#define _GNU_SOURCE
#include "zu_map.h"
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "libls/alloc_utils.h"
#include "libls/panic.h"

typedef struct {
    size_t key;
    size_t value;
} Entry;

struct Zu_Map {
    size_t ndata;
    Entry data[];
};

Zu_Map *
zu_map_new(void)
{
    const size_t ndata = 2000003; // is a prime
    Zu_Map *m = ls_xcalloc(sizeof(Zu_Map) + ndata * sizeof(size_t), 1);
    m->ndata = ndata;
    return m;
}

size_t
zu_map_get(Zu_Map *m, size_t key)
{
    const size_t n = m->ndata;
    const size_t base = key % n;

    for (size_t offset = 0; offset != n; ++offset) {
        Entry e = m->data[(base + offset) % n];
        if (!e.value) {
            break;
        }
        if (e.key == key) {
            return e.value - 1;
        }
    }

    return -1;
}

void
zu_map_set(Zu_Map *m, size_t key, size_t value)
{
    const size_t n = m->ndata;
    const size_t base = key % n;

    for (size_t offset = 0; offset != n; ++offset) {
        Entry *e = &m->data[(base + offset) % n];
        if (!e->value) {
            *e = (Entry) {.key = key, .value = value + 1};
            return;
        }
    }

    LS_PANIC("no space left in zu_map");
}

size_t
zu_map_get_or_set(Zu_Map *m, size_t key, size_t value)
{
    const size_t n = m->ndata;
    const size_t base = key % n;

    for (size_t offset = 0; offset != n; ++offset) {
        Entry *e = &m->data[(base + offset) % n];
        if (!e->value) {
            *e = (Entry) {.key = key, .value = value + 1};
            return value;
        }
        if (e->key == key) {
            return e->value - 1;
        }
    }

    LS_PANIC("no space left in zu_map");
}

void
zu_map_free(Zu_Map *m)
{
    free(m);
}
