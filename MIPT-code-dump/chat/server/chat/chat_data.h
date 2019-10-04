#ifndef chat_chat_data_h_
#define chat_chat_data_h_

#include <sys/types.h>
#include "libls/compdep.h"

typedef struct {
    ssize_t user_id;
} ChatData;

LS_INHEADER ChatData chat_data_new(void) {
    return (ChatData) { .user_id = -1, };
}

LS_INHEADER void chat_data_destroy(ChatData *d) {
    (void) d;
}

#endif
