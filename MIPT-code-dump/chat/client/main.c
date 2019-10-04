#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include "common/span.h"
#include "common/message.h"
#include "common/message_parse.h"
#include "libls/compdep.h"
#include "libls/string_.h"
#include "libls/vector.h"
#include "libls/errno_utils.h"
#include "my_sscanf.h"
#include "net.h"

// This implementation uses two threads:
//   - reader thread reads commands from stdin and sends messages to the server;
//   - writer thread reads messages from server and writes human-readable messages to the stdout.
//
// Basically, they share nothing but the /connfd/ file descriptor and stdout buffer (both can report
// errors to stdout).

// A simple /exit()/ isn't thread-safe.
static void die(void) {
    _exit(EXIT_FAILURE);
}

// Called on protocol violation.
static void improper(void) {
    fprintf(stdout, "<!> Improper message\n");
    die();
}

// Connection file descriptor.
static int connfd;

// A buffer for the message being read.
static LSString readbuf = LS_VECTOR_NEW();

// Tries to read (as if with /read()/) exactly /nbuf/ bytes from a file descriptor /fd/ to a buffer
// /buf/.
//
// If an error or end of file happens before /nbuf/ bytes were read, prints out an error message and
// dies.
static void fullread(int fd, char *buf, size_t nbuf) {
    for (size_t nread = 0; nread != nbuf;) {
        ssize_t r = read(fd, buf + nread, nbuf - nread);
        if (r < 0) {
            LS_WITH_ERRSTR(s, errno,
                fprintf(stdout, "<!> Read error: %s\n", s);
            );
            die();
        } else if (r == 0) {
            fprintf(stdout, "<!> Server closed the connection\n");
            die();
        } else {
            nread += r;
        }
    }
}

// Tries to write (as if with /write()/) exactly /nbuf/ bytes from a buffer /buf/ to a file
// descriptor /fd/.
//
// If an error happens before /nbuf/ bytes were written, prints out an error message and dies.
static void fullwrite(int fd, const char *buf, size_t nbuf) {
    for (size_t nwritten = 0; nwritten != nbuf;) {
        ssize_t w = write(fd, buf + nwritten, nbuf - nwritten);
        if (w < 0) {
            LS_WITH_ERRSTR(s, errno,
                fprintf(stdout, "<!> Write error: %s\n", s);
            );
            die();
        } else {
            nwritten += w;
        }
    }
}

// Reads next message from /connfd/ to /msg/, storing strings in /readbuf/.
//
// Assumes that /msg->strs/ is already a valid, initialized vector (although possible non-empty)!
//
// Dies on any error.
static void read_next(Message *msg) {
    // Read the header: 1-byte type and 4-byte length of the rest
    char header[5];
    fullread(connfd, header, sizeof(header));
    // Calculate how many bytes are to read...
    size_t toread = ntohl(* (uint32_t *) (header + 1));
    // Reserve the place and read them!
    LS_VECTOR_ENSURE(readbuf, toread);
    fullread(connfd, readbuf.data, toread);
    // /fullread()/ does not update the string size, let's do it
    readbuf.size = toread;
    // Parse /msg/.
    if (!message_parse(msg, header, readbuf.data, toread)) {
        improper();
    }
}

#define NTIMEBUF 128

// Prints human-readable date and time specified by /sec/ (UNIX time) and /usec/ (microseconds)
// arguments, to a buffer /buf/ of size /NTIMEBUF/, or returns a pointer to a static error string.
static const char * format_time(char *buf, time_t sec, uint32_t usec)
{
    (void) usec;
    struct tm *t = localtime(&sec);
    if (!t) {
        goto error;
    }
    if (strftime(buf, NTIMEBUF, "%H:%M %d-%m-%y", t) == 0) {
        goto error;
    }
    return buf;
error:
    return "(can't format time)";
}

// Returns a string description of chat status /status/.
static const char * status_to_str(uint32_t status) {
    switch (status) {
    case 0:
        return "OK";
    case 1:
        return "unknown message type";
    case 2:
        return "not logged in";
    case 3:
        return "auth failed";
    case 4:
        return "registration failed";
    case 5:
        return "not permitted";
    case 6:
        return "invalid message";
    default:
        return "(unknown status)";
    }
}

// Strips "bad" (non-printable) characters from a buffer /s/ of size /ns/, possibly copying it into
// the buffer /buf/.
//
// Returns either /s/ or a pointer pointing inside /buf/.
static const char * strip_badchars(const char *s, size_t ns, LSString *buf)
{
    char *copy = NULL;
    for (size_t i = 0; i < ns; ++i) {
        if ((unsigned char) s[i] >= ' ') {
            continue;
        }
        if (!copy) {
            const size_t old_size = buf->size;
            ls_string_append_b(buf, s, ns);
            copy = buf->data + old_size;
        }
        copy[i] = '.';
    }
    return copy ? copy : s;
}

// The reader thread function.
static void * reader(void *ptr) {
    (void) ptr;
    // The current message
    Message msg = MESSAGE_NEW();
    char time_buf[NTIMEBUF];
    LSString esc_buf = LS_VECTOR_NEW();

    // If the number of strings in the current message is not equal to /N_/, dies.
#define NSTRS(N_) \
    do { \
        if (msg.strs.size != (N_)) { \
            improper(); \
        } \
    } while (0)

    // Creates a /uint32_t/-typed variable with name /Var_/, fetched from the string of the current
    // message with index /Idx_/.
    //
    // If the length of the string mismatches, dies.
    //
    // Does not check if the index is valid!
#define FETCH_INT32(Var_, Idx_) \
    if (msg.strs.data[Idx_].size != 4) { \
        improper(); \
    } \
    uint32_t Var_ = ntohl(* (const uint32_t *) msg.strs.data[Idx_].data)

    // Creates two /uint32_t/-types variables named /SecVar_/ and /USecVar_/, fetches a timestamp
    // from the string of the current message with index /Idx_/, and places the "seconds" part to
    // /SecVar/, and "microseconds" one to /USecVar_/.
    //
    // If the length of the string mismatches, dies.
    //
    // Does not check if the index is valid!
#define FETCH_TIMESTAMP(Idx_, SecVar_, USecVar_) \
    if (msg.strs.data[Idx_].size != 8) { \
        improper(); \
    } \
    uint32_t SecVar_  = ntohl(* (const uint32_t *) msg.strs.data[Idx_].data); \
    uint32_t USecVar_ = ntohl(* (const uint32_t *) (msg.strs.data[Idx_].data + 4))

    // A human-readable time string from seconds and microseconds.
#define TIME_STR(Sec_, USec_) format_time(time_buf, Sec_, USec_)

    // Strips "bad" chars from a span /S_/, and returns the span as a pair suitable for use with
    // printf-like functions and the /%.*s/ format specifier: the size of the string as /int/, and
    // the string itself.
#define ESC_PAIR(S_) (int) (S_).size, strip_badchars((S_).data, (S_).size, &esc_buf)

    while (1) {
        read_next(&msg);
        LS_VECTOR_CLEAR(esc_buf);
        switch (msg.type) {
        case 's':
            NSTRS(1);
            {
                FETCH_INT32(status, 0);
                fprintf(stdout, "(*) %s\n", status_to_str(status));
            }
            break;
        case 'm':
            NSTRS(2);
            {
                FETCH_TIMESTAMP(0, s, us);
                fprintf(stdout, "(!) [%s] %.*s\n", TIME_STR(s, us), ESC_PAIR(msg.strs.data[1]));
            }
            break;
        case 'r':
            NSTRS(3);
            {
                FETCH_TIMESTAMP(0, s, us);
                fprintf(stdout, "<%.*s> [%s]: %.*s\n",
                        ESC_PAIR(msg.strs.data[1]), TIME_STR(s, us), ESC_PAIR(msg.strs.data[2]));
            }
            break;
        case 'h':
            NSTRS(3);
            {
                FETCH_TIMESTAMP(0, s, us);
                fprintf(stdout, "(~) <%.*s> [%s]: %.*s\n",
                        ESC_PAIR(msg.strs.data[1]), TIME_STR(s, us), ESC_PAIR(msg.strs.data[2]));
            }
            break;
        case 'l':
            if (msg.strs.size % 2 != 0) {
                improper();
            }
            for (size_t i = 0; i < msg.strs.size; i += 2) {
                FETCH_INT32(uid, i);
                fprintf(stdout, "(#) %-5zu: <%.*s>\n",
                        (size_t) uid, ESC_PAIR(msg.strs.data[i + 1]));
            }
            fprintf(stdout, "(#) end of list\n");
            break;
        case 'k':
            NSTRS(1);
            fprintf(stdout, "(-) You have been kicked: %.*s\n", ESC_PAIR(msg.strs.data[0]));
            break;
        default:
            improper();
            break;
        }
    }
    return NULL;
#undef NSTRS
#undef FETCH_INT32
#undef FETCH_TIMESTAMP
#undef ESC_PAIR
}

// Sends a message to the server with type /type/ and string passed via variable arguments as
// pointers to /ConstSpan/, followed by a null pointer.
//
// /LS_ATTR_SENTINEL(0)/ is there to check, at the compile-time, that argument list always ends with
// a null pointer.
LS_ATTR_SENTINEL(0) static void say(int type, ...) {
    // We'll need two iterations over the variable arguments list: first one to calculate the size
    // of the message, and the second one to actually send it.
    va_list vl, vl2;
    va_start(vl, type);
    va_copy(vl2, vl);
    // Calculate the total size
    size_t sumsz = 0;
    for (ConstSpan *p; (p = va_arg(vl, ConstSpan *));) {
        // The string plus its 4-byte length header
        sumsz += 4 + p->size;
    }
    // Form the header...
    char header[5] = {type};
    *(uint32_t *) (header + 1) = htonl(sumsz);
    /// and send it.
    fullwrite(connfd, header, sizeof(header));
    // Now, send the rest:
    for (ConstSpan *p; (p = va_arg(vl2, ConstSpan *));) {
        // Send the length of the string...
        fullwrite(connfd, (char *) (uint32_t [1]) {htonl(p->size)}, 4);
        // and the string itself.
        fullwrite(connfd, p->data, p->size);
    }
    va_end(vl);
    va_end(vl2);
}

// Use as variable arguments to /say()/.
#define SAY_CSTR(S_)  (&(ConstSpan) {S_, strlen(S_)})
#define SAY_INT32(I_) (&(ConstSpan) {(const char *) (uint32_t [1]) {htonl(I_)}, 4})
#define SAY_END       ((ConstSpan *) NULL)

// Prints out the help message.
static void help(void) {
    fprintf(stdout,
"<!> ---\n"
"<!> Type in:\n"
"<!>     /login USER PASS    -- to log in as USER with password PASS\n"
"<!>     /logout             -- to log out\n"
"<!>     /list               -- to get the list of online users (with their IDs)\n"
"<!>     /history N          -- to get the last N messages\n"
"<!>     /kick ID REASON     -- to kick user by ID for REASON\n"
"<!>     /help               -- to get this message\n"
"<!>     MESSAGE             -- to send a message to the chatroom\n"
"<!>     //MESSAGE           -- to send a message starting with '/'\n"
"<!> ---\n"
    );
}

// Checks a call to /my_sscanf/.
//
// If a command has not been recognized, returns /false/ and does not touch /*recognized/.
//
// If a command has been recognized, but can't be parsed in some reason, prints an error message,
// sets /*recognized/ to true, and returns /false/.
//
// If a command has been successfully recognized and parsed, /*recognized/ is set to /true/, and
// /true/ is returned.
static bool check_my_sscanf(bool *recognized, int ret)
{
    switch (ret) {
    case MY_SSCANF_OK:
        *recognized = true;
        return true;
    case MY_SSCANF_CANTFOLLOW:
        return false;
    case MY_SSCANF_OVERFLOW:
        fprintf(stdout, "<!> Integer or string argument is too big.\n");
        *recognized = true;
        return false;
    case MY_SSCANF_INVLDFMT:
    default:
        fprintf(stderr, "BUG: incorrect format string or unexpected my_sscanf return code\n");
        abort();
    }
}

// The writer thread.
static void writer(void) {
    char *buf = NULL;
    size_t nbuf = 0;
    for (ssize_t r; (r = getline(&buf, &nbuf, stdin)) > 0;) {
        if (buf[0] == '/' && buf[1] != '/') {
            // This is a command, not a message.

            // Storage for command's arguments...
            union {
                struct {
                    char login[64];
                    char pass[64];
                } cmd_login;
                struct {
                    char reason[128];
                    unsigned id;
                } cmd_kick;
                struct {
                    unsigned nentries;
                } cmd_hist;
            } u;
            bool recognized = false;
#define SSCANF(...) check_my_sscanf(&recognized, my_sscanf(buf, __VA_ARGS__))
#define S_ARG(Arr_) sizeof(Arr_), Arr_

            if (SSCANF("/login %w %w\n", S_ARG(u.cmd_login.login), S_ARG(u.cmd_login.pass))) {
                say('i', SAY_CSTR(u.cmd_login.login), SAY_CSTR(u.cmd_login.pass), SAY_END);

            } else if (SSCANF("/logout\n")) {
                say('o', SAY_END);

            } else if (SSCANF("/list\n")) {
                say('l', SAY_END);

            } else if (SSCANF("/history %u\n", &u.cmd_hist.nentries)) {
                say('h', SAY_INT32(u.cmd_hist.nentries), SAY_END);

            } else if (SSCANF("/kick %u %w\n", &u.cmd_kick.id, S_ARG(u.cmd_kick.reason))) {
                say('k', SAY_INT32(u.cmd_kick.id), SAY_CSTR(u.cmd_kick.reason), SAY_END);

            } else if (SSCANF("/help\n")) {
                help();

            } else if (!recognized) {
                fprintf(stdout, "<!> Unknown command!\n");
            }
        } else {
            // Don't send the trailing newline to the server.
            if (buf[r - 1] == '\n') {
                buf[r - 1] = '\0';
            }

            // Skip the first slash if the input starts with two
            char *msg = buf[0] == '/' ? buf + 1 : buf;

            say('r', SAY_CSTR(msg), SAY_END);
        }
    }
    if (!feof(stdin)) {
        LS_WITH_ERRSTR(s, errno,
            fprintf(stdout, "<!> getline (stdin): %s\n", s);
        );
    }
#undef S_ARG
#undef SSCANF
}

int main(int argc, char **argv) {
    // This is required since the tmux client script redirects our stdout to a FIFO.
    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);

    if (argc != 3) {
        fprintf(stderr, "USAGE: client <host> <service>\n");
        exit(EXIT_FAILURE);
    }
    if ((connfd = tcp_open(argv[1], argv[2], stdout)) < 0) {
        die();
    }

    fprintf(stdout, "<!> Connection established\n");
    help();

    pthread_t t;
    if ((errno = pthread_create(&t, NULL, reader, NULL))) {
        LS_WITH_ERRSTR(s, errno,
            fprintf(stdout, "pthread_create: %s\n", s);
        );
        die();
    }
    writer();
    pthread_join(t, NULL);
    die();
}
