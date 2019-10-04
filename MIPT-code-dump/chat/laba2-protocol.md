BE stands for “big endian” in this document.

A *message* consists of 1-byte *type* and a (possibly empty) list of byte strings.

Each *message* starts with a 5-byte header: `<1-byte type> <4-byte length of the rest in BE>`.

The rest is zero or more of byte strings, each of them in format: `<4-byte length of next string in BE> <byte string of that length>`.

*Timestamp* is `<4-byte Unix time in BE> <4-byte microseconds in BE>`.

# Client-to-server messages

| Type | No. of byte strings | Description                                                                                | Response                                 |
|------|---------------------|--------------------------------------------------------------------------------------------|------------------------------------------|
| `i`  | 2                   | Log **i**n. Byte strings are login and password.                                           | `s`-message                              |
| `r`  | 1                   | **R**egular message. Byte string is the text of the message.                               | `s`-message or echo `r`-message          |
| `h`  | 1                   | Request last N messages from **h**istory. Byte string is 4-byte N in BE.                   | `s`-message or zero or more `h`-messages |
| `l`  | 0                   | Request **l**ist of online users.                                                          | `s`-message or `l`-message               |
| `k`  | 2                   | **K**ick a user. First byte string is 4-byte user ID in BE, the second one is reason text. | `s`-message                              |
| `o`  | 0                   | Log **o**ut.                                                                               | No                                       |

# Server-to-client messages

| Type | No. of byte strings | Description                                                                                                      |
|------|---------------------|------------------------------------------------------------------------------------------------------------------|
| `s`  | 1                   | **S**tatus message. Byte string is 4-byte *status* (see below) in BE.                                            |
| `m`  | 2                   | **M**essage from server. Byte strings are timestamp and the text of the message.                                 |
| `r`  | 3                   | **R**egular message. Byte strings are timestamp, login of the sender, text.                                      |
| `h`  | 3                   | Message from **h**istory. Byte strings are same to those of `r`-message.                                         |
| `l`  | arbitrary (even)    | **L**ist of online users. For each user, two byte strings are appended: the 4-byte user ID in BE, and the login. |
| `k`  | 1                   | Sent when the user has been **k**icked. Byte string is the reason text.                                          |

# Statuses

| Status | Description          |
|--------|----------------------|
| 0      | OK                   |
| 1      | Unknown message type |
| 2      | Not logged in        |
| 3      | Authentication error |
| 4      | Registration error   |
| 5      | Not permitted        |
| 6      | Invalid message      |

# Some details

Whenever a user logs in, logs out, disconnects, or gets kicked, the server broadcasts an appropriate `m`-message.

Chat uses TCP, default port is 1337.

Only `root` user can kick other users. `root` password is asked on server startup.
