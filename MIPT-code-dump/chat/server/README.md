Basically, the server consists of three layers of logic:

* “net“: maintaining per-connection writing queues, kicking logic;
* “proto”: maintaining per-connection reading buffers, message parsing logic;
* “chat”: actual chat logic.

Per-connection data
===

Per-connection data is also matroska-style subdivided into the “net”, “proto”, and “chat” layers — the `net_Conn_`, `ProtoData` and `ChatData` structures:

````c
// (in chat/chat_data.h)
typedef struct {
    // per-connection “chat”-level data
    // ...
} ChatData;

// (in proto/proto_data.h)
typedef struct {
    // per-connection “proto”-level data
    // ...
    ChatData chat_data;
} ProtoData;

// (in net/net.h)
typedef struct {
    // per-connection “net”-level data
    // ...
    ProtoData proto_data;
} net_Conn_;
````

So, when the “proto” layer needs to access its data for `i`th client, it calls `net_conn_data(i)`, and when the “chat” one needs to do so, it calls `proto_conn_data(i)`.

Modules communication
===

net ← proto
-----------

````c
    size_t      net_nconns(void);
    ProtoData * net_conn_data(size_t index);
    void        net_kick(size_t index);

    LSString *  net_conn_write_begin(size_t index);
    void        net_conn_write_finish(size_t index);
````

net → proto
-----------

````c
    ProtoData proto_data_new(void);
    void      proto_data_destroy(ProtoData *d);

    void proto_get_read_buf(size_t index);
    void proto_process_chunk(size_t index, size_t nread);
    void proto_bailed_out(size_t index);
````

proto ← chat
------------

````c
    size_t     proto_nconns(void);
    ChatData * proto_conn_data(size_t index);
    void       proto_kick(size_t index);

    ProtoEnqueue proto_enqueue_start_conn(size_t index, char type);
    ProtoEnqueue proto_enqueue_start_buf(LSString *buf, char type);
    ProtoEnqueue proto_enqueue_str(ProtoEnqueue *e, ConstSpan s);
    ConstSpan    proto_enqueue_finish(ProtoEnqueue *e);
    ConstSpan    proto_enqueue_raw(size_t index, ConstSpan s);
````

proto → chat
------------

````c
    ChatData chat_data_new(void);
    void     chat_data_destroy(ChatData *d);

    void chat_process_msg(size_t index, Message msg);
    void chat_invalid_msg(size_t index);
    void chat_bailed_out(size_t index);
````

Initialization
===
First, the “chat” logic level should be initialized with chat root’s password by calling:

````c
void chat_init(const char *root_password);
````

Then, “proto” should be initialized:

````c
void proto_init(void);
````

Then, “net” should be initialized and actually run by calling:

````c
void net_run(uint16_t port, bool reuse_addr);
````
