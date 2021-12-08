#include "server.h"

#include <string.h>

#include <string>

#include <fcntl.h>

#include <sstream>

#include <cmath>

#include <algorithm>

using namespace std;

Server::~Server() {
  if (close(main_sock) == -1)
    syserr("close");
}

void Server::clear_board() {
  for (int i = 0; i < MAX_X; ++i)
    for (int j = 0; j < MAX_Y; ++j)
      pixels_eaten[i][j] = false;
}

Server::Server(const string & port_number_arg,
  const string & seed_arg,
    const string & turning_speed_arg,
      const string & rounds_per_sec_arg,
        const string & maxx_arg,
          const string & maxy_arg) {
  time_of_last_turn = get_current_time();

  check_arguments(port_number_arg, seed_arg, turning_speed_arg, rounds_per_sec_arg, maxx_arg, maxy_arg);

  get_main_sock();

  generate_crc32_table();

  clear_board();

  memset(datagram, 0, sizeof(datagram));
}

uint32_t Server::give_number(const string &s) {
  if (s.length() > 10)
    syserr("Too big argument!");
  for (const auto l: s)
    if (l < '0' || l > '9')
      syserr("Not a number!");

  uint64_t result = stoull(s);
  if (result > 0xffffffff)
    syserr("Too big number!");
  if (result == 0)
    syserr("Value 0!");
  return result;
}

void Server::check_arguments(const string & port_number_arg,
  const string & seed_arg,
    const string & turning_speed_arg,
      const string & rounds_per_sec_arg,
        const string & maxx_arg,
          const string & maxy_arg) {
  port_number = give_number(port_number_arg);
  if (port_number > MAX_PORT_NUMBER)
    syserr("Port number is to big!");
  seed = give_number(seed_arg);
  turning_speed = give_number(turning_speed_arg);
  if (turning_speed > MAX_TURNING_SPEED)
    syserr("Turning speed exeeded!");
  rounds_per_sec = give_number(rounds_per_sec_arg);
  if (rounds_per_sec > MAX_ROUNDS_PER_SEC)
    syserr("Rounds per second exeeded!");
  maxx = give_number(maxx_arg);
  if (maxx > MAX_X)
    syserr("Max x argument exeeded!");
  maxy = give_number(maxy_arg);
  if (maxy > MAX_Y)
    syserr("Max y argument exeeded!");
}

void Server::get_main_sock() 
{
  memset( & server_address6, 0, sizeof(server_address6));
  memset( & current_client_address6, 0, sizeof(current_client_address6));
  main_sock = socket(AF_INET6, SOCK_DGRAM, 0);

  uint32_t no_opt = 0;
  setsockopt(main_sock, IPPROTO_IPV6, IPV6_V6ONLY, & no_opt, sizeof(no_opt));

  if (main_sock < 0)
    syserr("socket");

  server_address6.sin6_family = AF_INET6;
  server_address6.sin6_addr = in6addr_any;
  server_address6.sin6_port = htons(port_number);

  if (bind(main_sock, (struct sockaddr * ) & server_address6, (socklen_t) sizeof(server_address6)) < 0)
    syserr("bind");

  fcntl(main_sock, F_SETFL, O_NONBLOCK);
  rcva_len = (socklen_t) sizeof(current_client_address6);
}

uint64_t Server::get_current_time() {
  struct timeval start;
  gettimeofday( & start, NULL);
  uint64_t milisecond_start = (uint64_t) start.tv_sec * 1000 + (uint64_t)(start.tv_usec / 1000);
  return milisecond_start;
}

uint32_t Server::rand32() {
  uint32_t result = seed;
  seed = ((uint64_t) seed * 279410273ull) % 4294967291ull;
  return result;
}

ssize_t Server::receive_message_from_client() {
  memset( & current_client_address6, 0, sizeof(current_client_address6));
  ssize_t len = recvfrom(main_sock, (char * ) datagram, sizeof(datagram), 0,
    (struct sockaddr * ) & current_client_address6, & rcva_len);
  return len;
}

void Server::send_message_to_client(uint8_t * message, ssize_t len) {
  sendto(main_sock, message, (size_t) len, 0, (struct sockaddr * ) & current_client_address6, rcva_len);
}

void Server::write_number(uint8_t * & array, uint64_t number, uint64_t len) {
  uint64_t shift = (len * 8 - 8);
  uint64_t and_number = 0xff << (shift);
  for (uint64_t i = 0; i < len; ++i) {
    * array = (number & and_number) >> shift;
    shift -= 8;
    and_number = 0xff << (shift);
    array++;
  }
}

bool Server::is_connected(player &p) {
  return get_current_time() - p.time_of_last_message < 2000;
}

void Server::make_event_player_eliminated(player & p) {
  uint32_t crc32_check_sum;
  uint32_t len, event_no;
  uint8_t event_type, player_number;
  uint16_t size_of_event = sizeof(len) + sizeof(event_no) + sizeof(crc32_check_sum) + sizeof(event_type) + sizeof(player_number);
  uint8_t * event = (uint8_t * ) malloc(size_of_event);
  uint8_t * original_event = event;
  len = size_of_event - sizeof(crc32_check_sum) - sizeof(len);
  write_number(event, len, 4);
  event_no = events.size();
  write_number(event, event_no, 4);
  event_type = 2;
  write_number(event, event_type, 1);
  player_number = p.active_player_info -> player_number;
  write_number(event, player_number, 1);
  crc32_check_sum = update_crc32(original_event, event - original_event);
  write_number(event, crc32_check_sum, 4);
  p.active_player_info -> active = false;
  number_of_active_players--;
  p.active_player_info -> ready = false;
  number_of_ready_players--;
  number_of_not_ready_players++;
  events.push_back(make_pair(original_event, size_of_event));
}

void Server::make_event_game_over() {
  uint32_t crc32_check_sum;
  uint32_t len, event_no;
  uint8_t event_type;
  uint16_t size_of_event = sizeof(len) + sizeof(event_no) + sizeof(crc32_check_sum) + sizeof(event_type);
  uint8_t * event = (uint8_t * ) malloc(size_of_event);
  uint8_t * original_event = event;
  len = size_of_event - sizeof(crc32_check_sum) - sizeof(len);
  write_number(event, len, 4);
  event_no = events.size();
  write_number(event, event_no, 4);
  event_type = 3;
  write_number(event, event_type, 1);
  crc32_check_sum = update_crc32(original_event, event - original_event);
  write_number(event, crc32_check_sum, 4);
  events.push_back(make_pair(original_event, size_of_event));
}

void Server::make_event_pixel(player & p) {
  uint32_t crc32_check_sum;
  uint32_t len, event_no, x, y;
  uint8_t event_type, player_number;
  uint16_t size_of_event = sizeof(len) + sizeof(event_no) + sizeof(crc32_check_sum) + sizeof(event_type) + sizeof(player_number) + sizeof(x) + sizeof(y);
  uint8_t * event = (uint8_t * ) malloc(size_of_event);
  uint8_t * original_event = event;
  len = size_of_event - sizeof(crc32_check_sum) - sizeof(len);
  write_number(event, len, 4);
  event_no = events.size();
  write_number(event, event_no, 4);
  event_type = 1;
  write_number(event, event_type, 1);
  player_number = p.active_player_info -> player_number;
  write_number(event, player_number, 1);
  x = floor(p.active_player_info -> x);
  write_number(event, x, 4);
  y = floor(p.active_player_info -> y);
  write_number(event, y, 4);
  crc32_check_sum = update_crc32(original_event, event - original_event);
  write_number(event, crc32_check_sum, 4);
  events.push_back(make_pair(original_event, size_of_event));
  pixels_eaten[x][y] = 1;
}

void Server::make_event_new_game() {
  uint32_t crc32_check_sum;
  uint32_t sum_len_of_name = 0;
  for (auto & player: players) {
    if (player.active_player_info != NULL)
      sum_len_of_name += player.active_player_info -> player_name.size() + 1;
  }
  uint32_t len, event_no;
  uint8_t event_type;
  uint16_t size_of_event = sizeof(len) + sizeof(event_no) + sizeof(crc32_check_sum) + sizeof(event_type) + sizeof(maxx) + sizeof(maxy) + sum_len_of_name;
  uint8_t * event = (uint8_t * ) malloc(size_of_event);
  uint8_t * original_event = event;
  len = size_of_event - sizeof(crc32_check_sum) - sizeof(len);
  write_number(event, len, 4);
  event_no = events.size();
  write_number(event, event_no, 4);
  event_type = 0;
  write_number(event, event_type, 1);
  write_number(event, maxx, 4);
  write_number(event, maxy, 4);
  for (auto & player: players) {
    if (player.active_player_info != NULL) {
      string name = player.active_player_info -> player_name;
      for (uint32_t i = 0; i < name.size(); ++i) {
        * event = name[i];
        event++;
      }
      * event = '\0';
      event++;
    }
  }
  crc32_check_sum = update_crc32(original_event, event - original_event);
  write_number(event, crc32_check_sum, 4);
  events.push_back(make_pair(original_event, size_of_event));
}

void Server::move_player(player & p) {
  double alfa = M_PI / 180 * p.active_player_info -> current_turn;

  p.active_player_info -> x += cos(alfa);
  p.active_player_info -> y += sin(alfa);
}

uint32_t Server::create_message_for_client(uint8_t * datagram, uint32_t & next_expected_event_no) {
  uint8_t * original_datagram = datagram;
  write_number(datagram, game_id, 4);
  while ((uint32_t)(datagram + events[next_expected_event_no].second - original_datagram) <= BUFFER_SIZE && next_expected_event_no < events.size()) {
    for (uint32_t i = 0; i < events[next_expected_event_no].second; ++i)
      datagram[i] = events[next_expected_event_no].first[i];

    datagram += events[next_expected_event_no].second;
    next_expected_event_no++;
  }
  return datagram - original_datagram;
}

void Server::send_events_to_everybody(uint32_t next_expected_event_no) {
  uint8_t datagram[BUFFER_SIZE];
  while (next_expected_event_no < events.size()) {
    memset(datagram, 0, sizeof(datagram));
    uint32_t len = create_message_for_client(datagram, next_expected_event_no);
    for (auto & player: players) {
      current_client_address6 = player.address;
      send_message_to_client(datagram, len);
    }
  }
}

bool Server::is_player_eliminated(int32_t x, int32_t y) {
  return (x > (int32_t) maxx - 1 || y > (int32_t) maxy - 1 || x < 0 || y < 0 || pixels_eaten[x][y]);
}

void Server::do_turn() {
  time_of_last_turn = get_current_time();
  uint32_t next_expected_event_no = events.size();

  for (auto & player: players) {
    auto info = player.active_player_info;
    if (info != NULL) {
      if (is_connected(player)) {
        if (info -> active) {
          if (info -> turn_direction == 1)
            info -> current_turn = (info -> current_turn + turning_speed) % 360;
          else if (info -> turn_direction == 2)
            info -> current_turn = (info -> current_turn - turning_speed + 360) % 360;
          double x = info -> x, y = info -> y;
          move_player(player);
          if (floor(x) == floor(info -> x) && floor(y) == floor(info -> y))
            continue;
          if (is_player_eliminated(floor(info -> x), floor(info -> y)))
            make_event_player_eliminated(player);
          else
            make_event_pixel(player);
          if (number_of_active_players == 1) {
            make_event_game_over();
            game_is_on = false;
            for (auto event: events)
              free(event.first);
            events.clear();

            clear_board();
            number_of_active_players = 0;
            number_of_ready_players = 0;
            number_of_not_ready_players = 0;
            for (auto & player: players) {
              auto info = player.active_player_info;
              if (info != NULL) {
                info -> active = false;
                info -> ready = false;
                if (is_connected(player))
                  number_of_not_ready_players++;
              }
            }
            break;
          }
        }
      } else if (player.time_of_last_message != 0) {
        if (info -> active) {
          info -> active = false;
          number_of_active_players--;
        }
        player.time_of_last_message = 0;

        if (player.active_player_info -> ready)
          number_of_ready_players--;
        else
          number_of_not_ready_players--;
      
      }
    }
  }
  send_events_to_everybody(next_expected_event_no);
}

uint64_t Server::read_number(uint8_t * & array, uint64_t len) {
  uint64_t result = 0;
  uint64_t shift = (len * 8 - 8);
  for (uint64_t i = 0; i < len; ++i) {
    result += * array << shift;
    shift -= 8;
    array++;
  }
  return result;
}

void Server::add_new_player(const string & name, uint32_t session_id, uint8_t turn_direction, uint64_t time_of_message) {
  player p;
  p.session_id = session_id;
  p.time_of_last_message = time_of_message;
  p.address = current_client_address6;
  if (name != "") {
    p.active_player_info = (active_player * ) malloc(sizeof(active_player));
    p.active_player_info -> player_name = name;
    p.active_player_info -> turn_direction = turn_direction;
    if (turn_direction != 0) {
      p.active_player_info -> ready = true;
      number_of_ready_players++;
    } else {
      p.active_player_info -> ready = false;
      number_of_not_ready_players++;
    }
    p.active_player_info -> active = false;
  } else
    p.active_player_info = NULL;
  players.push_back(p);
}

bool Server::is_address_same(struct sockaddr_in6 a1, struct sockaddr_in6 a2) {
  for (uint32_t i = 0; i < 16; ++i)
    if (a1.sin6_addr.s6_addr[i] != a2.sin6_addr.s6_addr[i])
      return false;
  return a1.sin6_port == a2.sin6_port;
}

bool check_player_name(const string & player_name) {
  int len = player_name.length();
  if (len < 0 || len > 20)
    return false;
  for (int i = 0; i < len; ++i) {
    if (((int) player_name[i]) > 126 || ((int) player_name[i]) < 33)
      return false;
  }
  return true;
}

uint32_t Server::process_client_datagram(uint8_t * datagram, uint32_t len, uint64_t time_of_message) {
  uint8_t * original_datagram = datagram;
  uint64_t session_id = read_number(datagram, 8);
  uint8_t turn_direction = read_number(datagram, 1);
  uint32_t next_expected_event_no = read_number(datagram, 4);
  string player_name = "";
  while ((uint32_t)(datagram - original_datagram) < len) {
    player_name += * datagram;
    datagram++;
  }
  player * p { NULL };
  for (auto & player: players) {
    if (is_address_same(player.address, current_client_address6)) {
      if (is_connected(player)) {
        if (player.active_player_info == NULL || (player.active_player_info != NULL && player.active_player_info -> player_name.compare(player_name) == 0))
          p = & player;
        else if (player.session_id == session_id)
          return events.size(); //new name from same client, ignoring
        else
          p = & player;
      }
    } else if (player.active_player_info != NULL && player.active_player_info -> player_name.compare(player_name) == 0) {
      return events.size(); //same name as other player, ignoring 
    }
  }

  if (p == NULL) {
    if (!check_player_name(player_name))
      return events.size(); //invalid player name, ignoring 
    uint8_t number_of_connected_players = 0;
    for (auto & player: players)
      number_of_connected_players += is_connected(player);
    if (number_of_connected_players < 25)
      add_new_player(player_name, session_id, turn_direction, time_of_message);
  } else {
    if (session_id > p -> session_id) {
      if ((p -> active_player_info == NULL && player_name != "") || (p -> active_player_info != NULL && !p->active_player_info->active)){
        p -> time_of_last_message = 0;
        if (p->active_player_info != NULL) {
            if (p->active_player_info -> ready)
                number_of_ready_players--;
            else
                number_of_not_ready_players--;
        }
        p->address.sin6_addr = in6addr_any;

        add_new_player(player_name, session_id, turn_direction, time_of_message);
        return next_expected_event_no;
      }
      p->session_id = session_id;
      if (p -> active_player_info != NULL)
        p-> active_player_info->player_name = player_name;
    }
    if (session_id == p -> session_id) {
      if (p -> active_player_info != NULL) {
        if (!p -> active_player_info -> ready && turn_direction != 0 && !game_is_on) {
          number_of_ready_players++;
          number_of_not_ready_players--;
          p -> active_player_info -> ready = true;
        }
        p -> active_player_info -> turn_direction = turn_direction;
      }
      p -> time_of_last_message = time_of_message;
    }
  }

  return next_expected_event_no;
}

template < typename T >
  bool compare_players(T p1, T p2) {
    if (p1.active_player_info == NULL && p2.active_player_info != NULL)
      return false;
    else if (p1.active_player_info != NULL && p2.active_player_info == NULL)
      return true;
    else if (p1.active_player_info != NULL && p2.active_player_info != NULL)
      return p1.active_player_info -> player_name < p2.active_player_info -> player_name;
    return true;
  }

void Server::sort_and_number_players() {
  sort(players.begin(), players.end(), compare_players < Server::player > );
  int number = 0;
  for (auto & player: players) {
    if (player.active_player_info == NULL || !player.active_player_info -> ready) {
      break;
    }
    player.active_player_info -> player_number = number;
    number++;
  }
}

void Server::filter_players() {
  vector < player > temp;
  uint64_t current_time = get_current_time();
  for (player p: players) {
    if (current_time - p.time_of_last_message < 2000)
      temp.push_back(p);
    else if (p.active_player_info != NULL) {
      free(p.active_player_info);
    }
  }
  players.clear();
  for (player p: temp)
    players.push_back(p);
}

void Server::set_up_new_game() {
  game_is_on = true;
  filter_players();
  sort_and_number_players();
  make_event_new_game();

  game_id = rand32();
  for (auto & player: players) {
    if (player.active_player_info != NULL) {
      auto info = player.active_player_info;
      info -> x = (rand32() % maxx) + 0.5;
      info -> y = (rand32() % maxy) + 0.5;
      info -> current_turn = (rand32() % 360);
      if (pixels_eaten[(uint32_t) floor(info -> x)][(uint32_t) floor(info -> y)])
        make_event_player_eliminated(player);
      else {
        make_event_pixel(player);
        info -> active = true;
        number_of_active_players++;
      }
    }
  }
  send_events_to_everybody(0);
  time_of_last_turn = get_current_time();
}

void Server::update_on_disconnected() {
  for (auto & p: players) {
    if (!is_connected(p) && p.time_of_last_message != 0) {
      p.address.sin6_addr = in6addr_any;
      p.time_of_last_message = 0;
      if (p.active_player_info != NULL) {
        if (p.active_player_info -> ready)
          number_of_ready_players--;
        else
          number_of_not_ready_players--;
      }
    }
  }
}

bool Server::should_do_turn() {
  if (game_is_on) {
    uint64_t current_time = get_current_time();
    if ((current_time - time_of_last_turn) >= 1000ull / rounds_per_sec)
      return true;
  }
  return false;
}
void Server::receive_process_and_send_back_message_from_client() {
  memset(datagram, 0, sizeof(datagram));
  int32_t result = receive_message_from_client();
  if (result > 0) {
    uint64_t time_of_message = get_current_time();
    uint32_t next_expected_event_no = process_client_datagram(datagram, result, time_of_message);
    while (next_expected_event_no < events.size() && game_is_on) {
      memset(datagram, 0, sizeof(datagram));
      uint32_t len = create_message_for_client(datagram, next_expected_event_no);
      send_message_to_client(datagram, len);
    }
  }
}
bool Server::is_game_on() {
  return game_is_on;
}
bool Server::ready_to_set_up_new_game() {
  return number_of_not_ready_players == 0 && number_of_ready_players >= 2;
}