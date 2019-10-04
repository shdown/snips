#ifndef chat_msgs_h_
#define chat_msgs_h_

#include "enqueue_objs.h"
#include "hist/hist.h"
#include "users/users.h"

LS_INHEADER ConstSpan send_s(size_t index, unsigned status) {
    ProtoEnqueue e = proto_enqueue_start_conn(index, 's');
    enqueue_int(&e, status);
    return proto_enqueue_finish(&e);
}

LS_INHEADER ConstSpan send_m(size_t index, struct timeval tv, ConstSpan text) {
    ProtoEnqueue e = proto_enqueue_start_conn(index, 'm');
    enqueue_timeval(&e, tv);
    proto_enqueue_str(&e, text);
    return proto_enqueue_finish(&e);
}

LS_INHEADER ConstSpan store_h(struct timeval tv, ConstSpan login, ConstSpan text) {
    LSString *buf = chat_hist_populate();
    ProtoEnqueue e = proto_enqueue_start_buf(buf, 'h');
    enqueue_timeval(&e, tv);
    proto_enqueue_str(&e, login);
    proto_enqueue_str(&e, text);
    return proto_enqueue_finish(&e);
}

LS_INHEADER ConstSpan send_r(size_t index, struct timeval tv, ConstSpan login, ConstSpan text) {
    ProtoEnqueue e = proto_enqueue_start_conn(index, 'r');
    enqueue_timeval(&e, tv);
    proto_enqueue_str(&e, login);
    proto_enqueue_str(&e, text);
    return proto_enqueue_finish(&e);
}

LS_INHEADER ConstSpan send_l(size_t index) {
    ProtoEnqueue e = proto_enqueue_start_conn(index, 'l');
    for (size_t i = 0; i < chat_users_num(); ++i) {
        User* user = chat_user_by_id(i);
        if (!user->nconns) {
            continue;
        }
        enqueue_int(&e, i);
        proto_enqueue_str(&e, span_to_const(user->login));
    }
    return proto_enqueue_finish(&e);
}

LS_INHEADER ConstSpan send_k(size_t index, ConstSpan text) {
    ProtoEnqueue e = proto_enqueue_start_conn(index, 'k');
    proto_enqueue_str(&e, text);
    return proto_enqueue_finish(&e);
}

LS_INHEADER void broadcast_r(struct timeval tv, ConstSpan login, ConstSpan text) {
    const size_t n = proto_nconns();
    if (!n) {
        return;
    }
    ConstSpan msg = send_r(0, tv, login, text);
    for (size_t i = 1; i < n; ++i) {
        proto_enqueue_raw(i, msg);
    }
}

LS_INHEADER void broadcast_m(struct timeval tv, ConstSpan text) {
    const size_t n = proto_nconns();
    if (!n) {
        return;
    }
    ConstSpan msg = send_m(0, tv, text);
    for (size_t i = 1; i < n; ++i) {
        proto_enqueue_raw(i, msg);
    }
}

#endif
