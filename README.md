# Screen worms
To use (UI works only on Linux): 

```
git clone https://github.com/flail1123/Screen_Worms.git
cd Screen_Worms
```
now to set up server:
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

and to set up client:
```
cd server_and_client
make
make clean
 ./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]
```

  * `game_server` – address (IPv4 lub IPv6) or name of server
  * `-n player_name` – player's name
  * `-p n` – game server's port (default 2021)
  * `-i gui_server` – address (IPv4 lub IPv6) or name of the UI server (default localhost)
  * `-r n` – UI server's port (default 20210)

and to set up UI:
```
cd UI
make
make clean
 ./gui2 [port]
```

  * `port` – UI server's port (default 20210)

Description:
TODO
