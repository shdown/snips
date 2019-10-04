#ifndef read_pass_h_
#define read_pass_h_

// Read a password from the terminal, assuming stdin and stderr are connected to it.
//
// If /prompt/ is not /NULL/, prints it before reading the password.
//
// Returns an allocated (as if with /malloc/) C string on success, /NULL/ on failure.
char * read_pass(const char *prompt);

#endif
