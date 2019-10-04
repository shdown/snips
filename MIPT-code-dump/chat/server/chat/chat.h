#ifndef chat_chat_h_
#define chat_chat_h_

#include "../proto/proto.h"
#include "common/message.h"
#include <stddef.h>

void chat_init(const char *root_password);
void chat_process_msg(size_t index, Message msg);
void chat_invalid_msg(size_t index);
void chat_bailed_out(size_t index);

#endif
