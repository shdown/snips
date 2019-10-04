#include "chat.h"
#include "chat_data.h"
#include "enqueue_objs.h"
#include "send_msgs.h"
#include "span_utils.h"
#include "users/users.h"
#include "hist/hist.h"
#include <stdio.h>
#include <sys/time.h>
#include "libls/vector.h"
#include "libls/string_.h"

enum { MAX_MESSAGE_LEN = 65535 };
static const char *ROOT_USER_LOGIN = "root";

enum {
    STATUS_OK               = 0,
    STATUS_UNKNOWN_MSG_TYPE = 1,
    STATUS_NOT_LOGGED_IN    = 2,
    STATUS_AUTH_ERROR       = 3,
    STATUS_REG_ERROR        = 4,
    STATUS_NOT_PERMITTED    = 5,
    STATUS_INVALID_MSG      = 6,
};

static size_t root_user_id;

void chat_init(const char *root_password) {
    chat_hist_init();
    chat_users_init();
    root_user_id = chat_user_add((User) {
        .login = spanu_dup(spanu_const_from_s(ROOT_USER_LOGIN)),
        .password = spanu_dup(spanu_const_from_s(root_password)),
        .nconns = 0,
    });
}

static inline bool validate_login(ConstSpan s) {
    if (s.size < 2 || s.size > 31) {
        return false;
    }
    for (size_t i = 0; i < s.size; ++i) {
        if ((unsigned char) s.data[i] < 32) {
            return false;
        }
    }
    return true;
}

static inline bool validate_password(ConstSpan s) {
    return s.size >= 2 && s.size <= 31;
}

static inline struct timeval now(void) {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) {
        tv = (struct timeval) {0}; // dunno
    }
    return tv;
}

static inline void broadcast_m_str(const char *s) {
    broadcast_m(now(), spanu_const_from_s(s));
}

void chat_process_msg(size_t index, Message msg) {
    ChatData *d = proto_conn_data(index);

    // If the number of strings in the message does not equal to /N_/, sends a /STATUS_INVALID_MSG/ status message and
    // returns.
#define NSTRS(N_) \
    do { \
        if (msg.strs.size != (N_)) { \
            send_s(index, STATUS_INVALID_MSG); \
            return; \
        } \
    } while (0)

    // Creates a /uint32_t/ variable named /Var_/ and initializes it from a 4-byte integer in big endian from the
    // message string with index /Idx_/.
    // If the length of that string is not 4, sends a /STATUS_INVALID_MSG/ status message and returns.
    // Does not check the validify of the index!
#define FETCH_INT32(Var_, Idx_) \
    if (msg.strs.data[Idx_].size != 4) { \
        send_s(index, STATUS_INVALID_MSG); \
        return; \
    } \
    uint32_t Var_ = ntohl(* (const uint32_t *) msg.strs.data[Idx_].data)

    if (d->user_id < 0) {
        // a client behind this connection is not logged in yet
        if (msg.type != 'i') {
            // not an 'i'-type message? /STATUS_NOT_LOGGED_IN/.
            send_s(index, STATUS_NOT_LOGGED_IN);
            return;
        }
        // otherwise, require exactly two strings: a login and a password
        NSTRS(2);
        // validate them
        if (!validate_login(msg.strs.data[0]) ||
            !validate_password(msg.strs.data[1]))
        {
            send_s(index, STATUS_INVALID_MSG);
            return;
        }
        // look up the user by the login: is one already registered?
        ssize_t pretend_id = chat_user_lookup(msg.strs.data[0]);
        if (pretend_id >= 0) {
            // yes: compare the password sent
            User *user = chat_user_by_id(pretend_id);
            if (!span_equal(span_to_const(user->password), msg.strs.data[1])) {
                // wrong password! /STATUS_AUTH_ERROR/
                send_s(index, STATUS_AUTH_ERROR);
                return;
            }
            // OK, the password is correct; increment the number of connections and set /d->user_id/.
            ++user->nconns;
            d->user_id = pretend_id;
        } else {
            // register new user
            d->user_id = chat_user_add((User) {
                .login = spanu_dup(msg.strs.data[0]),
                .password = spanu_dup(msg.strs.data[1]),
                .nconns = 1,
            });
        }
        // possibly announce logging in
        User *user = chat_user_by_id(d->user_id);
        if (user->nconns == 1) {
            char buf[1024];
            snprintf(buf, sizeof(buf), "<%.*s> has logged in", SPANU_PAIR(user->login));
            broadcast_m_str(buf);
        }
        send_s(index, STATUS_OK);

    } else {
        // a client behind this connection is already logged in
        User *user = chat_user_by_id(d->user_id);
        switch (msg.type) {
        case 'r':
            // oh, so you want to send a message to the chat!
            // require exactly one string: the message text
            NSTRS(1);
            // check its length
            if (msg.strs.data[0].size > MAX_MESSAGE_LEN) {
                send_s(index, STATUS_INVALID_MSG);
                return;
            }
            {
                struct timeval tv = now();
                // store it in the history
                store_h(tv, span_to_const(user->login), msg.strs.data[0]);
                // and broadcast
                broadcast_r(tv, span_to_const(user->login), msg.strs.data[0]);
            }
            break;

        case 'i':
            // hey, you are already logged in! /STATUS_INVALID_MSG/.
            send_s(index, STATUS_INVALID_MSG);
            break;

        case 'o':
            // log out: require exactly zero strings.
            NSTRS(0);
            // decreate the number of connections and possibly announce logging out
            if (!--user->nconns) {
                char buf[1024];
                snprintf(buf, sizeof(buf), "<%.*s> has quit", SPANU_PAIR(user->login));
                broadcast_m_str(buf);
            }
            // a client behind this connection is not logged in anymore
            d->user_id = -1;
            break;

        case 'h':
            // fetch latest N messages from history
            // require exactly one string: the number of history messages
            NSTRS(1);
            {
                // fetch the asked number of messages...
                FETCH_INT32(nreq, 0);
                // and query the history, where raw 'h'-messages are stored
                size_t navail, startfrom, wrap;
                LSString *entries = chat_hist_query(nreq, &navail, &startfrom, &wrap);
                for (size_t i = 0; i < navail; ++i) {
                    LSString entry = entries[(startfrom + i) % wrap];
                    proto_enqueue_raw(index, (ConstSpan) {entry.data, entry.size});
                };
            }
            break;

        case 'l':
            // list online users: require exactly zero strings
            NSTRS(0);
            send_l(index);
            break;

        case 'k':
            // kick: require exactly two strings: user ID to kick, and the reason text
            NSTRS(2);
            // remember only the root user can do that!
            if (d->user_id != (ssize_t) root_user_id) {
                send_s(index, STATUS_NOT_PERMITTED);
                return;
            }
            {
                // fetch the reason text and the user ID to be kicked
                ConstSpan reason = msg.strs.data[1];
                FETCH_INT32(whom, 0);
                // is the ID valid?
                if (whom >= chat_users_num()) {
                    send_s(index, STATUS_INVALID_MSG);
                    return;
                }
                // fetch the User structure to be kicked
                User *kicked = chat_user_by_id(whom);
                // is the user online?
                if (!kicked->nconns) {
                    // no! /STATUS_INVALID_MSG/.
                    send_s(index, STATUS_INVALID_MSG);
                    return;
                }
                // now iterate over all the connections...
                const size_t n = proto_nconns();
                for (size_t i = 0; i < n; ++i) {
                    ssize_t user_id = proto_conn_data(i)->user_id;
                    if (user_id >= 0 && user_id == whom) {
                        // aha! the client behind this connection is to be kicked!
                        // send 'k'-message to this connection
                        send_k(i, reason);
                        // and kick it!
                        proto_kick(i);
                    }
                }
                // all the connections the user was behind were dropped
                kicked->nconns = 0;
                // let's announce the kicking
                char buf[1024];
                snprintf(buf, sizeof(buf), "<%.*s> has been kicked (reason: %.*s)",
                         SPANU_PAIR(kicked->login), SPANU_PAIR(reason));
                broadcast_m_str(buf);
            }
            // and send /STATUS_OK/ to the root.
            send_s(index, STATUS_OK);
            break;

        default:
            send_s(index, STATUS_UNKNOWN_MSG_TYPE);
            break;
        }
    }
#undef NSTRS
#undef FETCH_INT32
}

void chat_invalid_msg(size_t index) {
    send_s(index, STATUS_INVALID_MSG);
}

void chat_bailed_out(size_t index) {
    ChatData *d = proto_conn_data(index);

    if (d->user_id >= 0) {
        User *user = chat_user_by_id(d->user_id);
        if (!--user->nconns) {
            char buf[1024];
            snprintf(buf, sizeof(buf), "<%.*s> has bailed out", SPANU_PAIR(user->login));
            broadcast_m_str(buf);
        }
    }
}
