#ifndef Server_H
#define Server_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <ncurses.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <math.h>

#define COLOR_FLOOR 1
#define COLOR_WALL 2
#define COLOR_COIN 3
#define COLOR_BEAST 4
#define COLOR_PLAYER 5
#define COLOR_CAMP 6

#define MAP_X 53
#define MAP_Y 27
#define BEASTS_MAX_QUANTITY 10
#define PLAYERS_MAX_QUANTITY 4
#define DROP_MAX_QUANTITY 100


struct map_t{
    int camp_x;
    int camp_y;
    char** map_arr;
};

struct send_t
{
    char map[5][5];
    int x;
    int y;
    int round;
    int deaths;
    int money;
    int camp_money;
};

struct player_t{
    int is_connected;
    pid_t player_pid;
    int player_x;
    int player_y;
    int bushes;
    char player_icon;
    int deaths;
    int curr_money;
    int money_in_camp;
    char player_move;
    char prev_place;
    int client_sender;
    int client_reader;
    struct send_t send;
    pthread_mutex_t move;
};

struct beast_t{
    int beast_x;
    int beast_y;
    char last_char;
    int last_move;
    pthread_mutex_t move;
    pthread_mutex_t has_moved;
    int bushes;

};

struct drop_t{
    int amount;
    int x;
    int y;
    char prev_char;
    int was_picked;
};

struct server_t{
    struct map_t* map;
    struct beast_t beasts[BEASTS_MAX_QUANTITY];
    struct player_t players[PLAYERS_MAX_QUANTITY];
    struct drop_t drops[DROP_MAX_QUANTITY];
    int drop_count;
    int player_count;
    int beast_count;
    int round;
    char option;
    pid_t server_pid;
}server;



struct map_t* load_map(const char* filename);
void put_coins();
void put_coin( char coin_type);
void put_drop(int index);
void show_map(struct map_t* map);
void show_players_info();
void* add_beast();
void* move_beast(void *arg);
int chase_player (struct beast_t* beast);
int is_only_one_move(struct beast_t* beast);
void *send_client_number();
void *listen_for_option();
void server_proccess();
void* player_handler(void* arg);
void set_player_info(int i);


#endif