#ifndef net_net_h_
#define net_net_h_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "libls/vector.h"
#include "libls/string_.h"
#include "libls/compdep.h"
#include "../proto/proto_data.h"

void net_run(uint16_t port, bool reuse_addr);

void net_kick(size_t index);

typedef struct {
    // write queue for this connection
    LSString wq;

    // number of bytes already written (once /wq_written/ hits /wq.size/, /wq/ is cleared and /wq_written/ is zeroed).
    size_t wq_written;

    // if the client is not to be kicked yet, /-1/; otherwise, the connection is dropped once /wq_written/ hits
    // /kick_wq_sz/.
    ssize_t kick_wq_sz;

    // is this connection to be dropped on the next /collect_garbage()/ call?
    bool garbage;

    // proto-layer data
    ProtoData proto_data;
} net_Conn_;

typedef LS_VECTOR_OF(net_Conn_) net_conns_vector_;
extern net_conns_vector_ net_conns_;

LS_INHEADER LSString * net_conn_write_begin(size_t index) {
    return &net_conns_.data[index].wq;
}

void net_conn_write_finish(size_t index);

LS_INHEADER size_t net_nconns(void) {
    return net_conns_.size;
}

LS_INHEADER ProtoData * net_conn_data(size_t index) {
    return &net_conns_.data[index].proto_data;
}

#endif
