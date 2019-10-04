#ifndef proto_proto_h_
#define proto_proto_h_

#include "../net/net.h"
#include <stddef.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "proto_data.h"
#include "libls/string_.h"
#include "libls/compdep.h"
#include "common/span.h"

void proto_init(void);
Span proto_get_read_buf(size_t index);
void proto_process_chunk(size_t index, size_t nread);
void proto_bailed_out(size_t index);

//------------------------------------------------------------------------------

typedef struct {
    LSString *buf_;
    size_t prevsz_;
    ssize_t index_;
} ProtoEnqueue;

LS_INHEADER ProtoEnqueue proto_enqueue_start_conn(size_t index, char type) {
    LSString *buf = net_conn_write_begin(index);
    ls_string_append_b(buf, (char [5]) {type}, 5);
    return (ProtoEnqueue) {
        .buf_ = buf,
        .prevsz_ = buf->size - 5,
        .index_ = index,
    };
}

LS_INHEADER ProtoEnqueue proto_enqueue_start_buf(LSString *buf, char type) {
    ls_string_append_b(buf, (char [5]) {type}, 5);
    return (ProtoEnqueue) {
        .buf_ = buf,
        .prevsz_ = buf->size - 5,
        .index_ = -1,
    };
}

LS_INHEADER void proto_enqueue_str(ProtoEnqueue *e, ConstSpan s) {
    ls_string_append_b(e->buf_, (char *) (uint32_t [1]) {htonl(s.size)}, 4);
    ls_string_append_b(e->buf_, s.data, s.size);
}

LS_INHEADER ConstSpan proto_enqueue_finish(ProtoEnqueue *e) {
    char *start = e->buf_->data + e->prevsz_;
    size_t nmsg = e->buf_->size - e->prevsz_;
    *(uint32_t *) (start + 1) = htonl(nmsg - 5);
    if (e->index_ >= 0) {
        net_conn_write_finish(e->index_);
    }
    return (ConstSpan) {start, nmsg};
}

LS_INHEADER ConstSpan proto_enqueue_raw(size_t index, ConstSpan s) {
    LSString *buf = net_conn_write_begin(index);
    size_t prevsz = buf->size;
    ls_string_append_b(buf, s.data, s.size);
    net_conn_write_finish(index);
    return (ConstSpan) {buf->data + prevsz, s.size};
}

LS_INHEADER size_t proto_nconns(void) {
    return net_nconns();
}

LS_INHEADER ChatData * proto_conn_data(size_t index) {
    return &net_conn_data(index)->chat_data;
}

LS_INHEADER void proto_kick(size_t index) {
    net_kick(index);
}

#endif
