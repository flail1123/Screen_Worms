#include <stdarg.h>

#include "server.h"

using namespace std;


int main(int argc, char * argv[]) {

  int opt;
  string port_number = "2021";
  string seed = to_string(time(NULL));
  string turning_speed = "6";
  string rounds_per_sec = "50";
  string maxx = "640";
  string maxy = "480";


  while ((opt = getopt(argc, argv, "p:s:t:v:w:h:")) != -1) {

    switch (opt) {
    case 'p':
      port_number = string(optarg);
      break;
    case 's':
      seed = string(optarg);
      break;
    case 't':
      turning_speed = string(optarg);
      break;
    case 'v':
      rounds_per_sec = string(optarg);
      break;
    case 'w':
      maxx = string(optarg);
      break;
    case 'h':
      maxy = string(optarg);
      break;
    case ':':
      syserr("option needs a value");
      break;
    case '?':
      syserr("unknown option");
      break;
    }
  }

  if (argv[optind] != NULL)
    syserr("some extra arguments");


  Server server(port_number, seed, turning_speed, rounds_per_sec, maxx, maxy);


  while (true) {
    if (server.should_do_turn())
      server.do_turn();

    server.receive_process_and_send_back_message_from_client();

    if (!server.is_game_on()) {
      server.update_on_disconnected();
      if (server.ready_to_set_up_new_game())
        server.set_up_new_game();
    }
  }

  return 0;
}