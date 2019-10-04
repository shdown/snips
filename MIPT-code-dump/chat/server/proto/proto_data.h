#ifndef proto_proto_data_h_
#define proto_proto_data_h_

#include "../chat/chat_data.h"
#include "libls/string_.h"
#include "libls/vector.h"
#include "libls/compdep.h"

typedef struct {
    // read buffer
    LSString rb;

    // number of bytes to read before doing anything else
    size_t rb_toread;

    // if the message is too long, number of bytes to skip; otherwise zero.
    size_t toskip;

    // chat-layer data
    ChatData chat_data;
} ProtoData;

LS_INHEADER ProtoData proto_data_new(void) {
    return (ProtoData) {
        .rb = LS_VECTOR_NEW(),
        .rb_toread = 5, // the 5-byte header: 1-byte type and 4-byte length of the rest
        .toskip = 0,
        .chat_data = chat_data_new(),
    };
}

LS_INHEADER void proto_data_destroy(ProtoData *d) {
    LS_VECTOR_FREE(d->rb);
    chat_data_destroy(&d->chat_data);
}

#endif
