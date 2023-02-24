#include "player_lib.h"

int main() {
    get_player_number();
    if(server.player.player_icon=='5'){printf("%c", server.player.player_icon);
        printf("Server is full");

        return 1;
    }
    initscr();

    char fifo_to_client[50] = "fifo_to_client";
    strcat(fifo_to_client, &(server.player.player_icon));
    strcat(fifo_to_client, "\0");

    server.player.server_reader = open(fifo_to_client, O_RDONLY);

    char fifo_to_server[50] = "fifo_to_server";
    strcat(fifo_to_server, &server.player.player_icon);
    strcat(fifo_to_server, "\0");

    server.player.server_sender = open(fifo_to_server, O_WRONLY);

    pthread_t pd;
    pthread_create(&pd, NULL, player_input, NULL);

    while(server.player.player_move!='q')
    {
        read_file();

        add_map();
        add_info_window();

        write_file();
    }
    close(server.player.server_reader);
    close(server.player.server_sender);

    endwin();

    return 0;
}