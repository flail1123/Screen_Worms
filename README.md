# Screen worms
How to run (UI works only on Linux): 

```
git clone https://github.com/flail1123/Screen_Worms.git
cd Screen_Worms
```
to set up aserver:
```
cd server_and_client
make
make clean
./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]
```
where:
  * `-p n` – port number (domyślnie `2021`)
  * `-s n` – random generator seed (default `time(NULL)`)
  * `-t n` – parameter `TURNING_SPEED`, integer, default `6`
  * `-v n` – parameter `ROUNDS_PER_SEC`, integer, default `50`
  * `-w n` – board's width in pixels (default `640`)
  * `-h n` – board's height in pixels (default `480`)

to set up the client:
```
cd server_and_client
make
make clean
 ./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]
```

  * `game_server` – address (IPv4 l IPv6) or name of server
  * `-n player_name` – player's name
  * `-p n` – game server's port (default 2021)
  * `-i gui_server` – address (IPv4 lub IPv6) or name of the UI server (default localhost)
  * `-r n` – UI server's port (default 20210)

to set up UI (only Linux):
```
cd UI
make
make clean
 ./gui2 [port]
```

  * `port` – UI server's port (default 20210)

Description:

Screen Worms was an assignment for "Computer Networks" course. 

The game server part is an orchestrator that sets up the game, lets players in, ends the game and so on. It communicates with the client part via UDP (because it is much faster than TCP and the server can have dozens of players), it gets from the client what is the player's direction and the server sends back to the client all the new events that happened in the game since the last update.

The client part talks with the server as described above, but also with UI via TCP.  The client sends to UI what is happening in the game for it to display the changes. On the other hand, UI sends to the client what buttons were pressed and released by the player.

DISCLAIMER: The objective of the assignment was to write the server and the client part. The UI was not written by me, but provided by the professor.
