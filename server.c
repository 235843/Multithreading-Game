#include "server.h"

void server_proccess()
{
    server.map = load_map("map.txt");
    put_coins();
    server.player_count=0;
    server.beast_count=0;
    server.drop_count=0;
    server.round=0;
    server.server_pid = getpid();
    server.option='\0';

    for(int i=0;i<PLAYERS_MAX_QUANTITY;i++)
    {
        server.players[i].player_icon = (char) ('1' + i);
        pthread_mutex_init(&server.players[i].move, NULL);
        set_player_info(i);
    }
    initscr();

    pthread_t sender, option;
    pthread_create(&sender, NULL, send_client_number, NULL);
    pthread_create(&option, NULL, listen_for_option, NULL);

    while(server.option!='q')
    {
        sleep(1);
        show_map(server.map);
        show_players_info();
        server.round++;

        for(int i=0;i<server.beast_count;i++)
            pthread_mutex_unlock(&server.beasts[i].move);

        for(int i=0;i<server.beast_count;i++)
            pthread_mutex_lock(&server.beasts[i].has_moved);

        for(int i=0;i<PLAYERS_MAX_QUANTITY;i++)
        {
            pthread_mutex_lock(&server.players[i].move);
        }

    }

    for(int i=0;i<PLAYERS_MAX_QUANTITY;i++)
    {
        pthread_mutex_destroy(&server.players[i].move);
        if(server.players[i].is_connected)
        {
            close(server.players[i].client_sender);
            close(server.players[i].client_reader);
        }
    }

    for(int i=0;i<server.beast_count;i++){
        pthread_mutex_destroy(&server.beasts[i].move);
        pthread_mutex_destroy(&server.beasts[i].has_moved);
    }


    for(int i=0;i<MAP_Y;i++)
        free(server.map->map_arr[i]);
    free(server.map->map_arr);
    free(server.map);

    endwin();
}

void set_player_info(int i) {
    server.players[i].player_y = server.map->camp_y;
    server.players[i].player_x = server.map->camp_x;
    server.players[i].deaths = 0;
    server.players[i].player_move = '\0';
    server.players[i].money_in_camp = 0;
    server.players[i].bushes = 0;
    server.players[i].curr_money = 0;
    server.players[i].prev_place = 'O';
    server.players[i].is_connected = 0;
}

struct map_t* load_map(const char* filename)
{
    if(!filename)
        return NULL;
    struct map_t *map = calloc(1, sizeof (struct map_t));
    if(!map)
        return NULL;

    char** tmap = calloc(MAP_Y, sizeof(char *));
    if(!tmap)
        return NULL;

    for(int i=0;i<MAP_Y;i++)
    {
        *(tmap+i) = calloc(MAP_X,1);
        if(!*(tmap+i))
            return NULL;
    }

    FILE *f;
    if(!(f = fopen(filename, "r")))
        return NULL;

    for(int i=0;i<MAP_Y;i++)
    {
        for(int j=0;j<MAP_X;j++)
        {
            *(*(tmap+i)+j) = (char)getc(f);

        }
        getc(f);
    }
    fclose(f);

    map->map_arr = tmap;
    map->camp_x=26;
    map->camp_y=14;


    return map;
}

void *listen_for_option()
{
    server.player_count=0;
    server.beast_count=0;
    while(server.option!='q') {
        server.option = getch();
        if(server.option=='c')
            server.option='C';
        switch(server.option){
            case 'b':
                if(server.beast_count<BEASTS_MAX_QUANTITY) {
                    pthread_t pd;
                    pthread_create(&pd, NULL, add_beast, NULL);
                }
                break;
            case 'C':
            case 't':
            case 'T':
                put_coin(server.option);
                break;
        }

    }
    return NULL;
}

void put_coins()
{
    srand(time(NULL));

    for(int i=0;i<15;i++)
    {
        int x = rand()%53;
        int y = rand()%27;
        if(*(*(server.map->map_arr+y)+x)!=' ')
        {
            i--;
            continue;
        }
        *(*(server.map->map_arr+y)+x)='C';
    }

    for(int i=0;i<3;i++)
    {
        int x = rand()%53;
        int y = rand()%27;
        if(*(*(server.map->map_arr+y)+x)!=' ')
        {
            i--;
            continue;
        }
        *(*(server.map->map_arr+y)+x)='t';
    }

    for(int i=0;i<1;i++)
    {
        int x = rand()%53;
        int y = rand()%27;
        if(*(*(server.map->map_arr+y)+x)!=' ')
        {
            i--;
            continue;
        }
        *(*(server.map->map_arr+y)+x)='T';
    }

}

void put_drop(int index)
{
    server.drops[server.drop_count].prev_char=server.players[index].prev_place;
    server.drops[server.drop_count].x=server.players[index].player_x;
    server.drops[server.drop_count].y=server.players[index].player_y;
    server.drops[server.drop_count].amount=server.players[index].curr_money;
    server.drops[server.drop_count].was_picked=0;
    server.drop_count++;
}

void* send_client_number()
{
    if(mkfifo("fifo_player_numbers", 0777)==-1)
    {
        if(errno!=EEXIST)
            return NULL;
    }
    while(server.option!='q') {

        if(server.player_count==4){
            int fd = open("fifo_player_numbers", O_WRONLY);
            char t = '5';
            write(fd, &t, sizeof (char));
            close(fd);
            continue;
        }

        int fd = open("fifo_player_numbers", O_WRONLY);

        int index=0;
        for(;index<PLAYERS_MAX_QUANTITY;index++){
            mvprintw(2,index*2,"%d", index);
            if(!server.players[index].is_connected)
                break;
        }

        write(fd,  &server.players[index].player_icon,sizeof (char));
        close(fd);

        server.player_count++;



        pthread_t player;
        server.players[index].is_connected = 1;
        char temp[2] = {0};
        *temp = server.players[index].player_icon;

        char fifo_to_client[50] = "fifo_to_client";
        strcat(fifo_to_client, temp);

        if (mkfifo(fifo_to_client, 0777) == -1) {
            if (errno != EEXIST)
                return NULL;
        }
        server.players[index].client_sender = open(fifo_to_client, O_WRONLY);

        char fifo_to_server[50] = "fifo_to_server";
        strcat(fifo_to_server, temp);
        if (mkfifo(fifo_to_server, 0777) == -1) {
            if (errno != EEXIST)
                return NULL;
        }
        server.players[index].client_reader = open(fifo_to_server, O_RDONLY);

        int* ind = malloc(4);
        *ind = index;

        pthread_t pt;
        pthread_create(&pt, NULL, player_handler, (void*)ind);
    }
    return NULL;
}

void* player_handler(void* arg)
{
    int index = *(int*)arg;
    free(arg);
    server.map->map_arr[server.players[index].player_y][server.players[index].player_x] = server.players[index].player_icon;

    while (server.players[index].player_move!='q')
    {
        pthread_mutex_unlock(&server.players[index].move);

        for (int i = 0, yp = server.players[index].player_y-2; i < 5; i++, yp++) {
            for (int j = 0, xp = server.players[index].player_x-2; j < 5; j++, xp++) {
                server.players[index].send.map[i][j] = server.map->map_arr[yp][xp];
            }
        }
        server.players[index].send.x = server.players[index].player_x;
        server.players[index].send.y = server.players[index].player_y;
        server.players[index].send.round = server.round;
        server.players[index].send.deaths = server.players[index].deaths;
        server.players[index].send.money = server.players[index].curr_money;
        server.players[index].send.camp_money = server.players[index].money_in_camp;

        write(server.players[index].client_sender, &server.players[index].send, sizeof (struct send_t));

        read(server.players[index].client_reader, &server.players[index].player_move,sizeof (char ));

        if(server.players[index].player_move=='q')
        {
            close(server.players[index].client_reader);
            close(server.players[index].client_sender);
            server.map->map_arr[server.players[index].player_y][server.players[index].player_x] = server.players[index].prev_place;
            set_player_info(index);
            server.players[index].is_connected=0;
            server.player_count--;
            break;
        }

        if(server.players[index].bushes)
        {
            server.players[index].bushes=0;
            continue;
        }

        switch (server.players[index].player_move) {
            case 'w':case 'W'://up
                if(server.map->map_arr[server.players[index].player_y-1][server.players[index].player_x]!='_')
                {
                    server.map->map_arr[server.players[index].player_y][server.players[index].player_x]=server.players[index].prev_place;
                    server.players[index].player_y--;
                    server.players[index].prev_place = server.map->map_arr[server.players[index].player_y][server.players[index].player_x];
                }break;
            case 's':case 'S'://down
                if(server.map->map_arr[server.players[index].player_y+1][server.players[index].player_x]!='_')
                {
                    server.map->map_arr[server.players[index].player_y][server.players[index].player_x]=server.players[index].prev_place;
                    server.players[index].player_y++;
                    server.players[index].prev_place = server.map->map_arr[server.players[index].player_y][server.players[index].player_x];
                }break;
            case 'a':case 'A'://left
                if(server.map->map_arr[server.players[index].player_y][server.players[index].player_x-1]!='_')
                {
                    server.map->map_arr[server.players[index].player_y][server.players[index].player_x]=server.players[index].prev_place;
                    server.players[index].player_x--;
                    server.players[index].prev_place = server.map->map_arr[server.players[index].player_y][server.players[index].player_x];
                }break;
            case 'd':case 'D'://right
                if(server.map->map_arr[server.players[index].player_y][server.players[index].player_x+1]!='_') {
                    server.map->map_arr[server.players[index].player_y][server.players[index].player_x] = server.players[index].prev_place;
                    server.players[index].player_x++;
                    server.players[index].prev_place = server.map->map_arr[server.players[index].player_y][server.players[index].player_x];
                }break;
        }
        server.map->map_arr[server.players[index].player_y][server.players[index].player_x]=server.players[index].player_icon;
        switch (server.players[index].prev_place)
        {
            case'C':
                server.players[index].curr_money++;
                server.players[index].prev_place=' ';
                put_coin('c');
                break;
            case 't':
                server.players[index].curr_money+=10;
                server.players[index].prev_place=' ';
                put_coin('t');
                break;
            case'T':
                server.players[index].curr_money+=50;
                server.players[index].prev_place=' ';
                put_coin('T');
                break;
            case 'D':
                    for(int i=0;i<server.drop_count;i++)
                    {
                        if(server.players[index].player_x==server.drops[i].x && server.players[index].player_y==server.drops[i].y && !server.drops[i].was_picked)
                        {
                            server.players[index].curr_money+=server.drops[i].amount;
                            server.players[index].prev_place=server.drops[i].prev_char;
                            server.drops[i].was_picked=1;
                            if(server.players[index].prev_place=='D')
                                server.players[index].prev_place=' ';
                        }
                    }
                break;
            case 'O':
                server.players[index].money_in_camp+=server.players[index].curr_money;
                server.players[index].curr_money=0;
                break;
            case '#':
                if(!server.players[index].bushes)
                    server.players[index].bushes=1;
                break;
            case '*':
                server.players[index].deaths++;
                if(server.players[index].curr_money)
                    put_drop((index));
                server.players[index].curr_money=0;
                server.map->map_arr[server.players[index].player_y][server.players[index].player_x] = 'D';
                server.players[index].player_x = server.map->camp_x;
                server.players[index].player_y = server.map->camp_y;
                server.players[index].prev_place = 'O';
                break;
        }
    }


}

void show_map(struct map_t* map) {

   WINDOW *win_map = newwin(27, 53, 5, 5);

    start_color();
    init_pair(COLOR_WALL, COLOR_BLACK, COLOR_BLACK);
    init_pair(COLOR_FLOOR, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_COIN, COLOR_BLACK, COLOR_YELLOW);
    init_pair(COLOR_BEAST, COLOR_BLACK, COLOR_RED);
    init_pair(COLOR_PLAYER, COLOR_BLACK, COLOR_MAGENTA);
    init_pair(COLOR_CAMP, COLOR_WHITE, COLOR_GREEN);

    for (int i = 0; i < MAP_Y; i++) {
        for (int j = 0; j < MAP_X; j++) {
            switch (*(*(map->map_arr + i) + j)) {
                case ' ':
                case '#':
                    wattron(win_map, COLOR_PAIR(COLOR_FLOOR));
                    mvwprintw(win_map, i, j, "%c", *(*(map->map_arr + i) + j));
                    wattroff(win_map, COLOR_PAIR(COLOR_FLOOR));
                    break;
                case '_':
                    wattron(win_map, COLOR_PAIR(COLOR_WALL));
                    mvwprintw(win_map, i, j, "%c", *(*(map->map_arr + i) + j));
                    wattroff(win_map, COLOR_PAIR(COLOR_WALL));
                    break;
                case 'O':
                    wattron(win_map, COLOR_PAIR(COLOR_CAMP));
                    mvwprintw(win_map, i, j, "%c", *(*(map->map_arr + i) + j));
                    wattroff(win_map, COLOR_PAIR(COLOR_CAMP));
                    break;
                case '1':
                case '2':
                case '3':
                case '4':
                    wattron(win_map, COLOR_PAIR(COLOR_PLAYER));
                    mvwprintw(win_map, i, j, "%c", *(*(map->map_arr + i) + j));
                    wattroff(win_map, COLOR_PAIR(COLOR_PLAYER));
                    break;
                case '*':
                    wattron(win_map, COLOR_PAIR(COLOR_BEAST));
                    mvwprintw(win_map, i, j, "%c", *(*(map->map_arr + i) + j));
                    wattroff(win_map, COLOR_PAIR(COLOR_BEAST));
                    break;
                case 'C':
                case 't':
                case 'T':
                case 'D':
                    wattron(win_map, COLOR_PAIR(COLOR_COIN));
                    mvwprintw(win_map, i, j, "%c", *(*(map->map_arr + i) + j));
                    wattroff(win_map, COLOR_PAIR(COLOR_COIN));
                    break;

            }

        }

    }
    box(win_map, 0, 0);

    wrefresh(win_map);
}

void show_players_info()
{

    WINDOW *info = newwin(30,100, 5, 70);
    mvwprintw(info, 2, 48, "INFO");
    mvwprintw(info, 3, 5, "__________________________________________________________________________________________");
    mvwprintw(info, 5, 5, "ROUND : %d", server.round);
    mvwprintw(info, 7, 5, "CAMP X/Y : %d/%d", server.map->camp_x, server.map->camp_y);
    mvwprintw(info, 9, 5, "GAME PID : %d", server.server_pid);
    mvwprintw(info, 11, 5, "BEASTS: %d/10", server.beast_count);
    mvwprintw(info, 12, 48, "STATS");
    mvwprintw(info, 13, 5, "__________________________________________________________________________________________");
    for(int i=0;i<4;i++)
    {
        mvwprintw(info, 15, 5+(i*25), "PLAYER %d:", i+1);
        if(server.players[i].is_connected==0)
        {
            mvwprintw(info, 17, 5+(i*25), "COORDS : --/--");
            mvwprintw(info, 19, 5+(i*25), "DEATHS: -");
            mvwprintw(info, 21, 5+(i*25), "COINS: -");
            mvwprintw(info, 23, 5+(i*25), "SAVED COINS: -");
            mvwprintw(info, 25, 5+(i*25),"TYPE: -");
            continue;
        }
        mvwprintw(info, 17, 5+(i*25), "COORDS : %d/%d", server.players[i].player_x, server.players[i].player_y);
        mvwprintw(info, 19, 5+(i*25), "DEATHS: %d", server.players[i].deaths);
        mvwprintw(info, 21, 5+(i*25), "COINS: %d", server.players[i].curr_money);
        mvwprintw(info, 23, 5+(i*25), "SAVED COINS: %d", server.players[i].money_in_camp);
        mvwprintw(info, 25, 5+(i*25),"TYPE: HUMAN");

    }
    box(info,0,0);
    refresh();
    wrefresh(info);

}

void* add_beast()
{
    int x, y;
    while(1)
    {
        x = rand()%MAP_X;
        y = rand()%MAP_Y;
        if(*(*(server.map->map_arr+y)+x)==' ')
            break;
    }
    struct beast_t *beast = &server.beasts[server.beast_count];

    *(*(server.map->map_arr+y)+x) = '*';
    beast->beast_x=x;
    beast->beast_y=y;
    beast->last_move=4;
    beast->last_char=' ';
    beast->bushes=0;
    pthread_mutex_init(&beast->move, NULL);
    pthread_mutex_init(&beast->has_moved, NULL);
    server.beasts[server.beast_count] = *beast;
    server.beast_count++;
    pthread_t beast_pt;
    pthread_create(&beast_pt, NULL, move_beast, (void*)&server.beasts[server.beast_count-1]);
}

void *move_beast(void * arg) {

    struct beast_t *beast = (struct beast_t *) arg;
        while(server.option!='q'){
            if(beast->bushes){
                beast->bushes=0;
                continue;
            }
            pthread_mutex_lock(&beast->move);
            int move;
            while (server.option!='q') {
                int did_moved = 0;
                move = rand() % 4;
                int temp_move;
                if ((temp_move = chase_player(beast) )!= 4) {
                    beast->last_move=4;
                    move = temp_move;
                }
                else if (is_only_one_move(beast) != -1) {
                    move = is_only_one_move(beast);
                    beast->last_move = 4;
                }
                switch (move) {
                    case 0:
                        if (*(*(server.map->map_arr + beast->beast_y + 1) + beast->beast_x) != '_' &&
                            beast->last_move != 1) {
                            did_moved = 1;
                            *(*(server.map->map_arr + beast->beast_y) + beast->beast_x) = beast->last_char;
                            beast->beast_y++;
                        }
                        break;
                    case 1:
                        if (*(*(server.map->map_arr + beast->beast_y - 1) + beast->beast_x) != '_' &&
                            beast->last_move != 0) {
                            did_moved = 1;
                            *(*(server.map->map_arr + beast->beast_y) + beast->beast_x) = beast->last_char;
                            beast->beast_y--;
                        }
                        break;
                    case 2:
                        if (*(*(server.map->map_arr + beast->beast_y) + beast->beast_x + 1) != '_' &&
                            beast->last_move != 3) {
                            did_moved = 1;
                            *(*(server.map->map_arr + beast->beast_y) + beast->beast_x) = beast->last_char;
                            beast->beast_x++;
                        }
                        break;
                    case 3:
                        if (*(*(server.map->map_arr + beast->beast_y) + beast->beast_x - 1) != '_' &&
                            beast->last_move != 2) {
                            did_moved = 1;
                            *(*(server.map->map_arr + beast->beast_y) + beast->beast_x) = beast->last_char;
                            beast->beast_x--;
                        }
                        break;
                    default:
                        break;
                }
                if(did_moved)
                    break;
            }
            beast->last_move = move;
            beast->last_char = *(*(server.map->map_arr + beast->beast_y) + beast->beast_x);
            if (beast->last_char == '*')
                beast->last_char = ' ';
            *(*(server.map->map_arr + beast->beast_y) + beast->beast_x) = '*';

            if(beast->last_char>='1' && beast->last_char<='4')
            {
                int index = beast->last_char-'1';
                server.players[index].deaths++;
                if(!server.players[index].curr_money)
                    put_drop(index);
                server.players[index].curr_money=0;
                beast->last_char='D';
                *(*(server.map->map_arr + beast->beast_y) + beast->beast_x) = 'D';
                server.players[index].player_x = server.map->camp_x;
                server.players[index].player_y = server.map->camp_y;
                server.players[index].prev_place = 'O';
            }
            if(beast->last_char=='#')
                beast->bushes=1;
            pthread_mutex_unlock(&beast->has_moved);
        }
}

int chase_player(struct beast_t* beast)
{
    for(int i=0,ii=beast->beast_y-2;i<5;i++, ii++)
    {
        for (int j = 0, jj=beast->beast_x-2;j < 5; j++, jj++)
        {
            if(server.map->map_arr[ii][jj]>='1' && server.map->map_arr[ii][jj]<='4')
            {
                if(server.map->map_arr[beast->beast_y-2][beast->beast_x]>='1' && server.map->map_arr[beast->beast_y-2][beast->beast_x]<='4' &&
                        server.map->map_arr[beast->beast_y-1][beast->beast_x]!='_')
                    return 1;
                if(server.map->map_arr[beast->beast_y+2][beast->beast_x]>='1' && server.map->map_arr[beast->beast_y+2][beast->beast_x]<='4' &&
                        server.map->map_arr[beast->beast_y+1][beast->beast_x]!='_')
                    return 0;
                if(server.map->map_arr[beast->beast_y][beast->beast_x-2]>='1' && server.map->map_arr[beast->beast_y][beast->beast_x-2]<='4' &&
                        server.map->map_arr[beast->beast_y][beast->beast_x-1]!='_')
                    return 3;
                if(server.map->map_arr[beast->beast_y][beast->beast_x+2]>='1' && server.map->map_arr[beast->beast_y][beast->beast_x+2]<='4' &&
                        server.map->map_arr[beast->beast_y][beast->beast_x+1]!='_')
                    return 2;

                if( server.map->map_arr[beast->beast_y+1][beast->beast_x+1]>='1' && server.map->map_arr[beast->beast_y+1][beast->beast_x+1]<='4')
                {
                    if(server.map->map_arr[beast->beast_y+1][beast->beast_x]!='_')
                        return 0;
                    else if(server.map->map_arr[beast->beast_y][beast->beast_x+1]!='_')
                        return 2;
                }

                else if( server.map->map_arr[beast->beast_y-1][beast->beast_x+1]>='1' && server.map->map_arr[beast->beast_y-1][beast->beast_x+1]<='4')
                {
                    if(server.map->map_arr[beast->beast_y-1][beast->beast_x]!='_')
                        return 1;
                    else if(server.map->map_arr[beast->beast_y][beast->beast_x+1]!='_')
                        return 2;
                }

                else if( server.map->map_arr[beast->beast_y+1][beast->beast_x-1]>='1' && server.map->map_arr[beast->beast_y+1][beast->beast_x-1]<='4')
                {
                    if(server.map->map_arr[beast->beast_y+1][beast->beast_x]!='_')
                        return 0;
                    else if(server.map->map_arr[beast->beast_y][beast->beast_x-1]!='_')
                        return 3;
                }

                else if( server.map->map_arr[beast->beast_y-1][beast->beast_x-1]>='1' && server.map->map_arr[beast->beast_y-1][beast->beast_x-1]<='4')
                {
                    if(server.map->map_arr[beast->beast_y-1][beast->beast_x]!='_')
                        return 1;
                    else if(server.map->map_arr[beast->beast_y][beast->beast_x-1]!='_')
                        return 3;
                }

                 double distance = (double)sqrtl(pow(ii-beast->beast_y,2) + pow(jj - beast->beast_x,2));

                if(distance < (double )sqrtl(pow(ii-beast->beast_y+1,2) + pow(jj-beast->beast_x, 2)) &&
                   server.map->map_arr[beast->beast_y+1][beast->beast_x]!='_' ||
                   server.map->map_arr[beast->beast_y+1][beast->beast_x]>='1' && server.map->map_arr[beast->beast_y+1][beast->beast_x]<='4')
                    return 0;

                 if(distance < (double )sqrtl(pow(ii-beast->beast_y-1,2) + pow(jj-beast->beast_x, 2)) &&
                   server.map->map_arr[beast->beast_y-1][beast->beast_x]!='_' ||
                        server.map->map_arr[beast->beast_y-1][beast->beast_x]>='1' && server.map->map_arr[beast->beast_y-1][beast->beast_x]<='4')
                    return 1;

                 if(distance < (double )sqrtl(pow(ii-beast->beast_y,2) + pow(jj-beast->beast_x+1, 2)) &&
                   server.map->map_arr[beast->beast_y][beast->beast_x+1]!='_' ||
                        server.map->map_arr[beast->beast_y][beast->beast_x+1]>='1' && server.map->map_arr[beast->beast_y][beast->beast_x+1]<='4')
                    return 2;

                 if(distance < (double )sqrtl(pow(ii-beast->beast_y,2) + pow(jj-beast->beast_x-1, 2)) &&
                   server.map->map_arr[beast->beast_y][beast->beast_x-1]!='_' ||
                        server.map->map_arr[beast->beast_y][beast->beast_x-1]>='1' && server.map->map_arr[beast->beast_y][beast->beast_x-1]<='4')
                    return 3;
                return 4;
            }
        }
    }
    return 4;
}

int is_only_one_move(struct beast_t* beast)
{
    if(*(*(server.map->map_arr+beast->beast_y)+beast->beast_x+1) != '_' &&
       *(*(server.map->map_arr+beast->beast_y)+beast->beast_x-1) == '_' &&
       *(*(server.map->map_arr+beast->beast_y+1)+beast->beast_x) == '_' &&
       *(*(server.map->map_arr+beast->beast_y-1)+beast->beast_x) == '_')
        return 2;
    if(*(*(server.map->map_arr+beast->beast_y)+beast->beast_x+1) == '_' &&
       *(*(server.map->map_arr+beast->beast_y)+beast->beast_x-1) != '_' &&
       *(*(server.map->map_arr+beast->beast_y+1)+beast->beast_x) == '_' &&
       *(*(server.map->map_arr+beast->beast_y-1)+beast->beast_x) == '_')
        return 3;
    if(*(*(server.map->map_arr+beast->beast_y)+beast->beast_x+1) == '_' &&
       *(*(server.map->map_arr+beast->beast_y)+beast->beast_x-1) == '_' &&
       *(*(server.map->map_arr+beast->beast_y+1)+beast->beast_x) != '_' &&
       *(*(server.map->map_arr+beast->beast_y-1)+beast->beast_x) == '_')
        return 0;
    if(*(*(server.map->map_arr+beast->beast_y)+beast->beast_x+1) == '_' &&
       *(*(server.map->map_arr+beast->beast_y)+beast->beast_x-1) == '_' &&
       *(*(server.map->map_arr+beast->beast_y+1)+beast->beast_x) == '_' &&
       *(*(server.map->map_arr+beast->beast_y-1)+beast->beast_x) != '_')
        return 1;
    return -1;
}

void put_coin(char coin_type)
{
    for(;;)
    {
        int x = rand()%53;
        int y = rand()%27;
        if(*(*(server.map->map_arr+y)+x)!=' ')
            break;
        *(*(server.map->map_arr+y)+x)=coin_type;
    }
}