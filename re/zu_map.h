#ifndef zu_map_h_
#define zu_map_h_

#include <stdlib.h>

typedef struct Zu_Map Zu_Map;

Zu_Map *
zu_map_new(void);

size_t
zu_map_get(Zu_Map *m, size_t key);

void
zu_map_set(Zu_Map *m, size_t key, size_t value);

size_t
zu_map_get_or_set(Zu_Map *m, size_t key, size_t value);

void
zu_map_free(Zu_Map *m);

#endif
