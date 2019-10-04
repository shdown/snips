#ifndef chat_users_users_h_
#define chat_users_users_h_

#include <stddef.h>
#include <unistd.h>
#include "common/span.h"
#include "libls/compdep.h"
#include "libls/vector.h"

typedef struct {
    Span login;
    Span password;
    size_t nconns;
} User;

typedef LS_VECTOR_OF(User) chat_user_UsersVector_;

extern chat_user_UsersVector_ chat_user_users_;

LS_INHEADER void chat_users_init(void)
{
    LS_VECTOR_INIT(chat_user_users_);
}

LS_INHEADER ssize_t chat_user_lookup(ConstSpan login) {
    for (size_t i = 0; i < chat_user_users_.size; ++i) {
        if (span_equal(span_to_const(chat_user_users_.data[i].login), login)) {
            return i;
        }
    }
    return -1;
}

LS_INHEADER User * chat_user_by_id(size_t id)
{
    return &chat_user_users_.data[id];
}

LS_INHEADER size_t chat_users_num(void)
{
    return chat_user_users_.size;
}

LS_INHEADER size_t chat_user_add(User user)
{
    LS_VECTOR_PUSH(chat_user_users_, user);
    return chat_user_users_.size - 1;
}

#endif
