A simple client-server multi-user chat program using TCP, supporting login/password authentication, history and kicking.

This is my “laboratory work” for the “Computer Architecture and Operating Systems” MIPT course.

For server usage info, run: `server/server --help`.

For client usage info, run: `client/client`.

The client writes to stdout and reads from stdin. For a saner interface, run `client/tmux_iface`. It uses tmux and looks like this:

![screenshot](https://user-images.githubusercontent.com/5462697/34220199-cf0b8b7e-e5c4-11e7-961b-4f74b11f3ba9.png)

See protocol desciption in `laba2-protocol.md` (but note I didn’t design it!).
