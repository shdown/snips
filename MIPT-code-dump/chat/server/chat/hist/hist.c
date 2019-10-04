#include "hist.h"

#include <unistd.h>
#include "libls/algo.h"

enum { HIST_SIZE = 50 };

static struct {
    LSString msgs[HIST_SIZE];
    size_t next;
    size_t size;
} hist;

void chat_hist_init(void) {
    for (size_t i = 0; i < HIST_SIZE; ++i) {
        LS_VECTOR_INIT(hist.msgs[i]);
    }
    hist.next = 0;
    hist.size = 0;
}

LSString * chat_hist_populate(void) {
    LSString *ret = hist.msgs + hist.next;
    LS_VECTOR_CLEAR(*ret);
    hist.next = (hist.next + 1) % HIST_SIZE;
    hist.size = LS_MIN(hist.size + 1, HIST_SIZE);
    return ret;
}

LSString * chat_hist_query(size_t n, size_t *navail, size_t *startfrom, size_t *wrap) {
    *navail = LS_MIN(n, hist.size);

    ssize_t from = (ssize_t) hist.next - (ssize_t) *navail;
    if (from < 0) {
        from += hist.size;
    }
    *startfrom = from;

    *wrap = hist.size;
    return hist.msgs;
}
