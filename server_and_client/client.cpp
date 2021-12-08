#include "client.h"
#include <string.h>
#include <sys/time.h>
#include <stdarg.h>
#include <iostream>
#include <fcntl.h>
#include <sstream>

using namespace std;

Client::~Client()
{
    if (close(gui_server_sock) == -1)
        syserr("close");

    if (close(game_server_sock) == -1)
        syserr("close");
}

Client::Client(const string &player_name_arg, const string &game_server_address_arg, const string &game_server_port_arg, const string &gui_server_address_arg, const string &gui_server_port_arg) : 
player_name(player_name_arg), game_server_address(game_server_address_arg), game_server_port(game_server_port_arg), gui_server_address(gui_server_address_arg), 
gui_server_port(gui_server_port_arg)
{
    check_arguments();

    generate_crc32_table();

    session_id = get_current_time_in_microseconds();

    get_gui_server_sock();

    get_game_server_sock();

    time_of_last_sending_to_server = get_current_time_in_microseconds();
}

bool Client::should_send_message_to_game_server()
{
    uint64_t current_time = get_current_time_in_miliseconds();
    return current_time - time_of_last_sending_to_server > 29;
}

void Client::send_message_to_game_server()
{
    time_of_last_sending_to_server = get_current_time_in_miliseconds();
    make_message_for_game_server();

    ssize_t snd_len;
    if (game_server_ip_version_6){
      socklen_t rcva_len = (socklen_t) sizeof(srvr_address6);
      snd_len = sendto(game_server_sock, buffer, buffer_len, 0,
	     (struct sockaddr *) &srvr_address6, rcva_len);
    }
    else{
      socklen_t rcva_len = (socklen_t) sizeof(srvr_address);
      snd_len = sendto(game_server_sock, buffer, buffer_len, 0,
	     (struct sockaddr *) &srvr_address, rcva_len);
    }
    if (snd_len < 0)
      syserr("Send to game server error!");
    
}

void Client::write_number(uint64_t number, uint64_t len)
{
  uint64_t shift = (len * 8 - 8);
  uint64_t and_number = 0xff << (shift);
  for (uint64_t i = 0; i < len; ++i)
  {
    buffer[buffer_len + i] = (number & and_number) >> shift;
    shift -= 8;
    and_number = 0xff << (shift);
  }
  buffer_len += len;
}

void Client::make_message_for_game_server()
{
    uint32_t len = player_name.length();
    memset(buffer, 0, (uint32_t)sizeof(buffer));
    buffer_len = 0;
    write_number(session_id, 8);

    write_number(turn_direction, 1);

    write_number(next_expected_event_no, 4);

    for (uint32_t i = 0; i < len; ++i)
        buffer[i + buffer_len] = player_name[i];
    buffer_len += len;
}

void Client::check_arguments()
{
    check_player_name_argument();
    check_port_argument(gui_server_port);
    check_port_argument(game_server_port);
}

void Client::check_port_argument(const string &port)
{
    uint32_t len = port.length();
    if (len > 10)
        syserr("Port is too big.");
    for (const auto &l : port)
        if (l < '0' || l > '9')
            syserr("Port is not a number.");
    uint64_t result = strtoll(port.c_str(), NULL, 10);
    if (result > MAX_PORT_NUMBER)
        syserr("Port is too big.");
    if (result == 0)
        syserr("Port is equal 0.");
}

void Client::check_player_name_argument()
{
    int len = player_name.length();
    if (len < 0 || len > 20)
        syserr("Wrong player name");
    for (int i = 0; i < len; ++i)
    {
        if (((int)player_name[i]) > 126 || ((int)player_name[i]) < 33)
            syserr("Wrong player name");
    }
}

uint64_t Client::get_current_time_in_microseconds()
{
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return current_time.tv_sec;
}
uint64_t Client::get_current_time_in_miliseconds()
{
    struct timeval start;
    gettimeofday(&start, NULL);
    uint64_t miliseconds = (uint64_t)start.tv_sec * 1000 + (uint64_t)(start.tv_usec / 1000);
    return miliseconds;
}

void Client::get_gui_server_sock()
{
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;

    int err;

    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_UNSPEC; 
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
    err = getaddrinfo(gui_server_address.c_str(), gui_server_port.c_str(), &addr_hints, &addr_result);
    if (err == EAI_SYSTEM or err != 0){
        syserr("Gui getaddrinfo: %s.", gai_strerror(err));
    }
    
    int32_t sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock < 0)
        syserr("Gui socket.");
    gui_server_sock = sock;
    
    if (connect(gui_server_sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
        syserr("Gui connect.");
    fcntl(gui_server_sock, F_SETFL, O_NONBLOCK);
    freeaddrinfo(addr_result);
}

void Client::get_game_server_sock()
{
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;

    
    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_UNSPEC; 
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;
    if (getaddrinfo(game_server_address.c_str(), NULL, &addr_hints, &addr_result) != 0)
        syserr("getaddrinfo");
    
    if (addr_result->ai_family == AF_INET6){
      game_server_ip_version_6 = true;
      memset(&srvr_address6, 0, sizeof(struct sockaddr_in6));
      srvr_address6.sin6_family = addr_result->ai_family; 
      srvr_address6.sin6_addr =
          ((struct sockaddr_in6 *)(addr_result->ai_addr))->sin6_addr;
      srvr_address6.sin6_port = htons((uint16_t)atoi(game_server_port.c_str()));
    }
    else if(addr_result->ai_family == AF_INET){
      memset(&srvr_address, 0, sizeof(struct sockaddr_in));
      srvr_address.sin_family = addr_result->ai_family; 
      srvr_address.sin_addr =
          ((struct sockaddr_in *)(addr_result->ai_addr))->sin_addr;
      srvr_address.sin_port = htons((uint16_t)atoi(game_server_port.c_str()));
    }
    else
      syserr("Not ipv6 neider ipv4!");
    

    int32_t sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock < 0)
        syserr("game_server_sock");
    game_server_sock = sock;
    fcntl(game_server_sock, F_SETFL, O_NONBLOCK);

    freeaddrinfo(addr_result);
}

void Client::receive_message_from_game_server()
{
  ssize_t rcv_len;
  if (game_server_ip_version_6){
    socklen_t rcva_len = (socklen_t) sizeof(srvr_address6);
    rcv_len = recvfrom(game_server_sock, buffer,  sizeof(buffer), 0,
	       (struct sockaddr *) &srvr_address6, &rcva_len);
  }
  else{
    socklen_t rcva_len = (socklen_t) sizeof(srvr_address);
    rcv_len = recvfrom(game_server_sock, buffer,  sizeof(buffer), 0,
	       (struct sockaddr *) &srvr_address, &rcva_len);
  }
  
  if (rcv_len < 0)
    rcv_len = 0;
  
  buffer_len = rcv_len;

}

uint64_t Client::read_number(uint8_t* &array, uint64_t len){
  uint64_t result = 0;
  uint64_t shift = (len * 8 - 8);
  for (uint64_t i = 0; i < len; ++i){
    result += *array << shift;
    shift -= 8;
    array++;
  }
  return result;
}

void Client::process_message_from_game_server(){
  uint8_t *datagram = buffer;
  uint32_t game_id = read_number(datagram, 4);
  if (current_game_id != game_id && current_game_id != 0){
    if (previous_game_ids.count(game_id) == 0){
      previous_game_ids.insert(current_game_id);
      current_game_id = game_id;
      next_expected_event_no = 0;
      in_game = true;
    }
    else{
      return;
    }
  }
  else if(!in_game && current_game_id != 0){
    return;
  }

  while(datagram - buffer < buffer_len){
    uint8_t *datagram2 = process_game_server_event(datagram, game_id);
    if (datagram2 == datagram)
      return;
    datagram = datagram2;
  }
}

void Client::receive_and_process_message_from_game_server(){
    memset(buffer, 0, (uint32_t)sizeof(buffer));
    receive_message_from_game_server();

    if (buffer_len > 0){
        process_message_from_game_server();
    }
}

uint8_t* Client::process_game_server_event(uint8_t *datagram, uint32_t game_id){
  uint8_t *original_datagram = datagram;
  uint32_t len = read_number(datagram, 4);
  uint32_t event_no = read_number(datagram, 4);

  if (event_no > next_expected_event_no){
    return original_datagram;
  }

  uint8_t event_type = read_number(datagram, 1);
  string message = "";
  vector<string> new_names;
  if (event_type == 0){
    message += "NEW_GAME ";
    maxx = read_number(datagram, 4);
    message += to_string(maxx) + " ";
    maxy = read_number(datagram, 4);
    message += to_string(maxy) + " ";
    len -= 13;
    string name = "";
    while (len > 0){
      if ((char)datagram[0] == '\0'){
        message += name + ' ';
        new_names.push_back(name);
        name = "";
      }
      else
        name += (char) datagram[0];
      datagram++;
      len--;
    }
    message[message.length() - 1] = char(10);
  }
  else if (event_type == 1){
    message += "PIXEL ";
    uint8_t player_number = read_number(datagram, 1);
    uint32_t x = read_number(datagram, 4);
    message += to_string(x) + " ";
    uint32_t y = read_number(datagram, 4);
    message += to_string(y) + " ";
    if (x >= maxx || y >= maxy)
      syserr("Player out of bounds!");
    if (player_number >= players_names.size())
      syserr("Invalid player id!");
    message += players_names[player_number];
    message += char(10);
  }
  else if (event_type == 2){
    message += "PLAYER_ELIMINATED ";
    uint8_t player_number = read_number(datagram, 1);
    if (player_number >= players_names.size())
      syserr("Invalid player id!");
    message += players_names[player_number];
    message += char(10);
  }
  uint32_t crc32_check_sum = read_number(datagram, 4);

  if (update_crc32(original_datagram, (uint32_t)((datagram - 4) - original_datagram)) != crc32_check_sum){
    return original_datagram;
  }
  if (event_no < next_expected_event_no || (event_type != 0 && !in_game)){ //ignore event
    return datagram;
  }
  if (event_type <= 2){
    send_message_to_gui_server(message);
  }
  next_expected_event_no ++;
  if (event_type == 3){
    previous_game_ids.insert(current_game_id);
    next_expected_event_no = 0;
    in_game = false;
  }
  else if (event_type == 0){
    next_expected_event_no = 1;
    in_game = true;
    current_game_id = game_id;
    players_names.clear();
    for (auto p : new_names)
      players_names.push_back(p);
  }
  return datagram;
}

void Client::send_message_to_gui_server(const string & message){
  ssize_t len = message.length();

  if (write(gui_server_sock, message.c_str(), len) != len)
    syserr("Write in send_to_gui_server failed.");

}

void Client::receive_message_from_gui_server(){
    ssize_t rcv_len = read(gui_server_sock, buffer, sizeof(buffer) - 1);
    rcv_len--;
    if (rcv_len == -2){// read didn't read anything so ignore that
      rcv_len = 0;
    }
    else if (rcv_len < 0)
      syserr("Read from gui error!");
    
    buffer_len = rcv_len;
}

void Client::process_gui_server_message(){
  string gui_message(buffer, buffer + buffer_len);
  stringstream s(gui_message);
  string key_stroke;
  while(getline(s, key_stroke, char(10)))
    process_key_stroke(key_stroke);
}

void Client::process_key_stroke(const string &key_stroke){
  if (key_stroke.compare("LEFT_KEY_DOWN") == 0)
    turn_direction = 2;
  else if (key_stroke.compare("RIGHT_KEY_DOWN") == 0)
    turn_direction = 1;
  else if (key_stroke.compare("RIGHT_KEY_UP") == 0 && turn_direction == 1)
    turn_direction = 0;
  else if (key_stroke.compare("LEFT_KEY_UP") == 0 && turn_direction == 2)
    turn_direction = 0;
}

void Client::receive_and_process_message_from_gui_server(){
    memset(buffer, 0, sizeof(buffer));
    receive_message_from_gui_server();
    if (buffer_len > 0){
        process_gui_server_message();
    }
    
}