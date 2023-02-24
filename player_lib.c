#include "player_lib.h"

void add_map()
{
    start_color();
    init_pair(COLOR_PLAYER, COLOR_WHITE, COLOR_CYAN);
    init_pair(COLOR_BEAST, COLOR_BLACK, COLOR_RED);
    init_pair(COLOR_COIN, COLOR_BLACK, COLOR_YELLOW);
    init_pair(COLOR_CAMP, COLOR_WHITE, COLOR_GREEN);
    init_pair(COLOR_WALL, COLOR_BLACK, COLOR_BLACK);
    init_pair(COLOR_FLOOR, COLOR_BLACK, COLOR_WHITE);

    WINDOW *win_map = newwin(27, 53, 3, 3);

        for (int i = 0; i < PlAYER_MAP_Y; i++) {
            for (int j = 0; j < PlAYER_MAP_X; j++) {
                switch (*(*(server.map + i) + j)) {
                    case ' ':
                    case '#':
                        wattron(win_map, COLOR_PAIR(COLOR_FLOOR));
                        mvwprintw(win_map, server.player.player_y-i+2,server.player.player_x-j+2, "%c", *(*(server.map+ i) + j));
                        wattroff(win_map, COLOR_PAIR(COLOR_FLOOR));
                        break;
                    case '_':
                        wattron(win_map, COLOR_PAIR(COLOR_WALL));
                        mvwprintw(win_map, server.player.player_y-i+2,server.player.player_x-j+2, "%c", *(*(server.map+ i) + j));
                        wattroff(win_map, COLOR_PAIR(COLOR_WALL));
                        break;
                    case 'O':
                        wattron(win_map, COLOR_PAIR(COLOR_CAMP));
                        mvwprintw(win_map, server.player.player_y-i+2,server.player.player_x-j+2, "%c", *(*(server.map+ i) + j));
                        wattroff(win_map, COLOR_PAIR(COLOR_CAMP));
                        break;
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                        wattron(win_map, COLOR_PAIR(COLOR_PLAYER));
                        mvwprintw(win_map, server.player.player_y-i+2,server.player.player_x-j+2, "%c", *(*(server.map+ i) + j));
                        wattroff(win_map, COLOR_PAIR(COLOR_PLAYER));
                        break;
                    case '*':
                        wattron(win_map, COLOR_PAIR(COLOR_BEAST));
                        mvwprintw(win_map, server.player.player_y-i+2,server.player.player_x-j+2, "%c", *(*(server.map+ i) + j));
                        wattroff(win_map, COLOR_PAIR(COLOR_BEAST));
                        break;
                    case 'C':
                    case 't':
                    case 'T':
                    case 'D':
                        wattron(win_map, COLOR_PAIR(COLOR_COIN));
                        mvwprintw(win_map, server.player.player_y-i+2,server.player.player_x-j+2, "%c", *(*(server.map+ i) + j));
                        wattroff(win_map, COLOR_PAIR(COLOR_COIN));
                        break;

                }

            }
        }
        box(win_map, 0, 0);


    wrefresh(win_map);

}

void add_info_window()
{
    WINDOW *info = newwin(20,30, 3,65);
        mvwprintw(info, 2, 13, "INFO");
        mvwprintw(info, 3, 0, "______________________________");
        mvwprintw(info, 5, 5, "COORDS X/Y : %d/%d", server.player.player_x, server.player.player_y);
        mvwprintw(info, 7, 5, "ROUND : %d", server.round_number);
        mvwprintw(info, 9, 5, "DEATHS: %d", server.player.deaths);
        mvwprintw(info, 11, 5, "COINS: %d", server.player.curr_money);
        mvwprintw(info, 13, 5, "COINS IN CAMP: %d", server.player.money_in_camp);
        mvwprintw(info, 15, 5,"GAME PID: %d", server.player.player_pid);
        mvwprintw(info, 17, 5,"TYPE: HUMAN");
        box(info,0,0);wrefresh(info);
        refresh();

}

void* player_input()
{
    while (server.player.player_move!='q')
    {
        server.player.player_move = getch();

    }
    return NULL;
}

void read_file()
{
    struct send_t send;
    read(server.player.server_reader, &send, sizeof (struct send_t));


    for(int i=0,ii=PlAYER_MAP_Y-1;i<PlAYER_MAP_Y;i++, ii--)
    {
        for (int j = 0, jj=PlAYER_MAP_X-1;j < PlAYER_MAP_X; j++, jj--)
        {
            server.map[ii][j] = send.map[i][jj];
        }
    }
    server.player.player_x = send.x;
    server.player.player_y = send.y;
    server.player.deaths = send.deaths;
    server.round_number = send.round;
    server.player.curr_money = send.money;
    server.player.money_in_camp = send.camp_money;

    server.player.player_pid = getpid();
}

void get_player_number()
{
    int fd = open("fifo_player_numbers",O_RDONLY);
    read(fd,&server.player.player_icon, sizeof (char));

    close(fd);
}

void write_file()
{
    write(server.player.server_sender, &server.player.player_move, sizeof(char));
    if(server.player.player_move!='q')
        server.player.player_move='\0';
}