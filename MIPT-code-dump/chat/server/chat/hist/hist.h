#ifndef chat_hist_hist_h_
#define chat_hist_hist_h_

#include "libls/string_.h"
#include "libls/compdep.h"
#include <stddef.h>

void chat_hist_init(void);

LSString * chat_hist_populate(void);

LSString * chat_hist_query(size_t n, size_t *navail, size_t *startfrom, size_t *wrap);

#endif
