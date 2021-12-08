#ifndef CLIENT_H
#define CLIENT_H

#include "err.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdarg.h>
#include <vector>
#include <set>
#include "crc32.h"

#define BUFFER_SIZE 550
#define MAX_PORT_NUMBER 65535

using namespace std;

class Client{
public:
    Client(const string &player_name_arg, const string &game_server_address_arg, const string &game_server_port_arg, const string &gui_server_address_arg, const string &gui_server_port_arg);
    ~Client();
    uint64_t get_current_time_in_miliseconds();
    bool should_send_message_to_game_server();
    void send_message_to_game_server();
    void receive_and_process_message_from_game_server();
    void receive_and_process_message_from_gui_server();
private:
    set <uint32_t> previous_game_ids;
    string player_name, game_server_address, game_server_port, gui_server_address, gui_server_port;
    uint64_t session_id, time_of_last_sending_to_server;
    uint32_t gui_server_sock, game_server_sock, buffer_len = 0, next_expected_event_no = 0, check_sum=0, maxy = 0, maxx = 0;
    uint8_t buffer[BUFFER_SIZE];
    uint8_t turn_direction = 0;
    struct sockaddr_in6 srvr_address6;
    struct sockaddr_in srvr_address;
    bool game_server_ip_version_6 = false;
    vector<string> players_names;
    uint32_t current_game_id = 0;
    bool in_game = false;
    void check_arguments();
    void check_player_name_argument();
    void check_port_argument(const string &port);
    uint64_t get_current_time_in_microseconds();
    void get_gui_server_sock();
    void get_game_server_sock();
    void make_message_for_game_server();
    void write_number(uint64_t number, uint64_t len);
    void receive_message_from_game_server();
    void process_message_from_game_server();
    uint64_t read_number(uint8_t* &array, uint64_t len);
    uint8_t* process_game_server_event(uint8_t *datagram, uint32_t game_id);
    void send_message_to_gui_server(const string & message);
    void receive_message_from_gui_server();
    void process_gui_server_message();
    void process_key_stroke(const string & key_stroke);
};


#endif //CLIENT_H