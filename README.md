# eos-eac-example-game-proxy-cheat
*An example of the good client/bad client proxy cheating method.*

This involves two instances of the game client running:
1) A normal ("good") game, with anti-cheat running but connected to the "bad" game as it's game server.
2) A cheatable ("bad") game, with anti-cheat disabled connected to the real game server.  This client has the malicious shared object file which forwards anti-cheat messages to the good game while sending gameplay related messages to the server as normal.

## Usage
(First open `setup.sh` and edit the path to the game client and server folders.)
```bash
# setup.sh contains functions to build and run the setup
$> source setup.sh

# build the malicious shared object file (proxy.so)
$> build

# start the game server
$> start_server

# start the "bad" client where cheats can be applied
$> start_bad_client

# start the "good" client which will handle anti-cheat communication
$> start_good_client

# find the pid of the bad client by checking the parent process of the two clients (The bad client will not have start_protected_game as its parent)
$> pidof MyEOSGame
$> cat /proc/$(cat /proc/xx/ppid)/comm
$> cat /proc/$(cat /proc/yy/ppid)/comm

# A tracer can then be attached to the bad game
$> sudo gdb -q -p xx
```
