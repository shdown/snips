#ifndef my_sscanf_h_
#define my_sscanf_h_

enum {
    MY_SSCANF_OK = 0,
    MY_SSCANF_CANTFOLLOW,
    MY_SSCANF_OVERFLOW,
    MY_SSCANF_INVLDFMT,
};

// A simple sscanf-like scanner that only supports %u and %w format specifiers.
// %u scans for an unsigned integer and takes an /unsigned */ argument.
// %w scans for a word and takes two arguments: buffer length as a /size_t/ and the buffer as a
// /char */.
int my_sscanf(const char *s, const char *fmt, ...);

#endif
