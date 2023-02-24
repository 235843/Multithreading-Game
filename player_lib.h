#ifndef PLAYER_LIB_H
#define PLAYER_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <ncurses.h>
#include <pthread.h>
#include <string.h>

#define COLOR_FLOOR 1
#define COLOR_WALL 2
#define COLOR_COIN 3
#define COLOR_BEAST 4
#define COLOR_PLAYER 5
#define COLOR_CAMP 6

#define PlAYER_MAP_X 5
#define PlAYER_MAP_Y 5

struct player_t{
    int player_x;
    int player_y;
    pid_t player_pid;
    int deaths;
    char player_icon;
    char player_move;
    int curr_money;
    int money_in_camp;
    int server_sender;
    int server_reader;
} ;

struct server_t{
    char map[5][5];
    int round_number;
    struct player_t player;
}server;

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

void get_player_number();
void add_info_window();
void add_map();
void read_file();
void write_file();
void* player_input();

#endif