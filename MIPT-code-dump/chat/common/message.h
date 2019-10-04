#ifndef common_message_h_
#define common_message_h_

#include "libls/vector.h"
#include "span.h"

// A chat message.
typedef struct {
    char type;
    LS_VECTOR_OF(ConstSpan) strs;
} Message;

#define MESSAGE_NEW() {.strs = LS_VECTOR_NEW()}

#endif
