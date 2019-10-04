#ifndef common_message_parse_h_
#define common_message_parse_h_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "libls/compdep.h"
#include "libls/vector.h"
#include "span.h"
#include "message.h"

// Parses a chat message from 5-byte /header/ and the /rest/ buffer of size /nrest/.
//
// Assumes that /msg->strs/ is an already initialized (although possibly non-empty) vector. It is
// recommended to use the /MESSAGE_NEW/ macro to initialize a /Message/ object once and then call
// this function on it for an arbitrary number of times.
//
// Returns /true/ on success and /false/ on failure.
LS_INHEADER bool message_parse(Message *msg, const char *header, const char *rest, size_t nrest) {
    msg->type = header[0];
    LS_VECTOR_CLEAR(msg->strs);
    for (size_t offset = 0; offset != nrest;) {
        if (nrest - offset < 4) {
            return false;
        }
        const size_t str_sz = ntohl(* (const uint32_t *) (rest + offset));
        offset += 4;
        if (nrest - offset < str_sz) {
            return false;
        }
        LS_VECTOR_PUSH(msg->strs, ((ConstSpan) {rest + offset, str_sz}));
        offset += str_sz;
    }
    return true;
}

#endif
