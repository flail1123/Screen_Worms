#include <stdarg.h>

#include "client.h"


using namespace std;

int main(int argc, char * argv[]) {
  if (argc < 2)
    syserr("No game_server arg!");
  int opt;
  string game_address = string(argv[1]);
  string player_name = "", game_port = "2021", gui_address = "localhost", gui_port = "20210";

  while ((opt = getopt(argc, argv, "n:p:i:r:")) != -1) {
    switch (opt) {
    case 'n':
      player_name = string(optarg);
      break;
    case 'p':
      game_port = string(optarg);
      break;
    case 'i':
      gui_address = string(optarg);
      break;
    case 'r':
      gui_port = string(optarg);
      break;
    case ':':
      syserr("option needs a value");
      break;
    case '?':
      syserr("unknown option");
      break;
    }
  }

  Client client(player_name, game_address, game_port, gui_address, gui_port);

  int i = 0;
  while (true) {
    if (client.should_send_message_to_game_server())
      client.send_message_to_game_server();

    client.receive_and_process_message_from_game_server();

    client.receive_and_process_message_from_gui_server();

    i++;
  }

  return 0;
}