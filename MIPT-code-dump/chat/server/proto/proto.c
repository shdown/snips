#include "proto.h"
#include "proto_data.h"
#include "../chat/chat.h"
#include "common/message.h"
#include "common/message_parse.h"

// We reject any messages with the size of rest exceedig this value.
enum { MAX_REST = 128 * 1024 };

void proto_init(void) {
}

Span proto_get_read_buf(size_t index) {
    ProtoData *d = net_conn_data(index);

    if (d->toskip) {
        LS_VECTOR_ENSURE(d->rb, MAX_REST);
        return (Span) {d->rb.data, d->rb.capacity};
    } else {
        LS_VECTOR_ENSURE(d->rb, d->rb_toread);
        return (Span) {d->rb.data + d->rb.size, d->rb_toread - d->rb.size};
    }
}

static Message msg = MESSAGE_NEW();

static void forward_msg(size_t index) {
    ProtoData *d = net_conn_data(index);

    char *data = d->rb.data;
    size_t ndata = d->rb.size;

    if (message_parse(&msg, data, data + 5, ndata - 5)) {
        chat_process_msg(index, msg);
    } else {
        chat_invalid_msg(index);
    }
}

void proto_process_chunk(size_t index, size_t nread) {
    ProtoData *d = net_conn_data(index);

    if (d->toskip) {
        d->toskip -= nread;
        if (!d->toskip) {
            chat_invalid_msg(index);
            d->rb_toread = 5;
            d->rb.size = 0;
        }
        return;
    }

    d->rb.size += nread;
    if (d->rb.size == d->rb_toread) {
        bool done = false;
        if (d->rb_toread == 5) {
            uint32_t rest = ntohl(* (uint32_t *) (d->rb.data + 1));
            if (rest) {
                if (rest > MAX_REST) {
                    d->toskip = rest;
                }
                d->rb_toread += rest;
            } else {
                done = true;
            }
        } else {
            done = true;
        }
        if (done) {
            forward_msg(index);
            d->rb_toread = 5;
            d->rb.size = 0;
        }
    }
}

void proto_bailed_out(size_t index) {
    chat_bailed_out(index);
}
