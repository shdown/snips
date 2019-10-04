#ifndef chat_enqueue_objs_h_
#define chat_enqueue_objs_h_

#include "common/span.h"
#include "../proto/proto.h"
#include <arpa/inet.h>
#include <sys/time.h>

LS_INHEADER void enqueue_int(ProtoEnqueue *e, uint32_t value) {
    uint32_t tmp = htonl(value);
    proto_enqueue_str(e, (ConstSpan) {(const char *) &tmp, 4});
}

LS_INHEADER void enqueue_timeval(ProtoEnqueue *e, struct timeval tv) {
    char buf[8];
    char *ptr = buf;
    *(uint32_t *) ptr       = htonl(tv.tv_sec);
    *(uint32_t *) (ptr + 4) = htonl(tv.tv_usec);
    proto_enqueue_str(e, (ConstSpan) {buf, 8});
}

#endif
