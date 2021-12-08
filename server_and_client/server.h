#ifndef SERVER_H
#define SERVER_H

#include "err.h"

#include <sys/types.h>

#include <sys/socket.h>

#include <netdb.h>

#include <stdio.h>

#include <unistd.h>

#include <stdlib.h>

#include <sys/time.h>

#include <string>

#include <iostream>

#include <vector>

#include "crc32.h"

#define BUFFER_SIZE 550
#define MAX_X 1920//
#define MAX_Y 1080//
#define MAX_TURNING_SPEED 90
#define MAX_ROUNDS_PER_SEC 250
#define MAX_PORT_NUMBER 65535

using namespace std;

class Server {
  public:
    Server(const string & port_number_arg,
      const string & seed_arg,
        const string & turning_seed_arg,
          const string & rounds_per_sec_arg,
            const string & maxx_arg,
              const string & maxy_arg);
  ~Server();

  bool should_do_turn();
  void do_turn();
  void receive_process_and_send_back_message_from_client();
  bool is_game_on();
  void update_on_disconnected();
  bool ready_to_set_up_new_game();
  void set_up_new_game();
  private:
    struct active_player {
      double x;
      double y;
      uint32_t current_turn;
      uint8_t turn_direction;
      string player_name;
      uint8_t player_number;
      bool active;
      bool ready;
    };

  struct player {
    uint32_t session_id;
    active_player * active_player_info;
    uint64_t time_of_last_message;
    struct sockaddr_in6 address;
  };
  bool pixels_eaten[MAX_X][MAX_Y];
  vector < pair < uint8_t * , uint16_t > > events;
  vector < player > players;
  struct sockaddr_in6 server_address6;
  struct sockaddr_in6 current_client_address6;
  socklen_t rcva_len;
  int32_t main_sock;
  uint32_t port_number, seed, turning_speed, rounds_per_sec, maxx, maxy, game_id;
  uint32_t number_of_active_players = 0, number_of_ready_players = 0, number_of_not_ready_players = 0;
  bool game_is_on = false;
  uint8_t datagram[BUFFER_SIZE];
  uint64_t time_of_last_turn;

  void check_arguments(const string & port_number_arg,
    const string & seed_arg,
      const string & turning_speed_arg,
        const string & rounds_per_sec_arg,
          const string & maxx_arg,
            const string & maxy_arg);
  uint32_t give_number(const string & s);
  void get_main_sock();
  uint64_t get_current_time();
  uint32_t rand32();

  ssize_t receive_message_from_client();
  void send_message_to_client(uint8_t * message, ssize_t len);
  void write_number(uint8_t * & array, uint64_t number, uint64_t len);
  bool is_connected(player & p);
  void make_event_player_eliminated(player & p);
  void make_event_game_over();
  void make_event_pixel(player & p);
  void make_event_new_game();
  void move_player(player & p);
  uint32_t create_message_for_client(uint8_t * datagram, uint32_t & next_expected_event_no);
  void send_events_to_everybody(uint32_t next_expected_event_no);
  bool is_player_eliminated(int32_t x, int32_t y);
  uint64_t read_number(uint8_t * & array, uint64_t len);
  void add_new_player(const string & name, uint32_t session_id, uint8_t turn_direction, uint64_t time_of_message);
  bool is_address_same(struct sockaddr_in6 a1, struct sockaddr_in6 a2);
  uint32_t process_client_datagram(uint8_t * datagram, uint32_t len, uint64_t time_of_message);
  void sort_and_number_players();
  void filter_players();
  void clear_board();
};

#endif //SERVER_H