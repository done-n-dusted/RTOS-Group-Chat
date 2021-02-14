#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>
#include <pthread.h>
#include "message.h"


#define PORT 8000

int socket_fd;
pthread_t rd;
message M;

void *rcv_message(){
    for(;;){
        int msg_res;
        message m;

        msg_res = recv(socket_fd, &m, sizeof(m), 0);
        printf("%s : %s\n", m.snd, m.msg);
    }
}

void close_it(int x){
    close(socket_fd);
    printf("Socket closed.\n");
    exit(0);
}

int main(int argc, char const *argv[]) {
    int inet_res, connect_res, fork_res;
    struct sockaddr_in server_addr;

    char name[10];
    printf("Enter name: \n");
    scanf("%[^\n]%*c", name);
    strcpy(M.snd, name);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_fd <= 0){
        printf("Socket couldn't be created\n");
        return -1;
    }
    signal(SIGINT, close_it);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    inet_res = inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if(inet_res <= 0){
        printf("Invalid address. Terminated\n");
        return -1;
    }

    printf("Attempting to connect\n");
    connect_res = connect(socket_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    if(connect_res < 0){
        printf("Connection Failed. Terminated\n");
        return -1;
    }

    printf("Connection established\n");

    send(socket_fd, &name, sizeof(name), 0);
    printf("Joined the group\n");

    pthread_create(&rd, NULL, rcv_message, NULL);

    for(;;){
        printf("Message: \n");
        scanf("%[^\n]%*c", M.msg);

        send(socket_fd, &M, sizeof(M), 0);
    }

    return 0;
}