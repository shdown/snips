#ifndef zu_map_h_
#define zu_map_h_

#include <stdlib.h>

typedef struct {
    size_t *ptr;
    size_t length;
} Zu_Map;

void
zu_map_init(Zu_Map *m);

size_t
zu_map_get(Zu_Map *m, size_t key);

void
zu_map_set(Zu_Map *m, size_t key, size_t value);

size_t
zu_map_get_or_set(Zu_Map *m, size_t key, size_t value);

void
zu_map_free(Zu_Map *m);

#endif
