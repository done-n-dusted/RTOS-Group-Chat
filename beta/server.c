#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include "message.h"

#define PORT 8000

#define MAX_PEOPLE 5

int people_sock[MAX_PEOPLE] = {0};
char people[MAX_PEOPLE][10] = {0};

int people_count = 0;
int socket_fd = 0;

pthread_t client_threads[MAX_PEOPLE];
pthread_mutex_t registration_lock;

message failed;

void close_it(int num){
    //kill threads
    for(int i = 0; i < people_count; i++){
        pthread_kill(client_threads[i], SIGKILL);
    }
    //kill sockets
    for(int i = 0; i < people_count; i++){
        close(people_sock[i]);
    }
    //close server socket
    close(socket_fd);
    printf("Closing all the sockets\n");
    exit(0);
}

void *client_handler(void *socket_fd){
    int client_fd = *(int *)socket_fd;
    char temp_name[10] = {0};
    recv(client_fd, &temp_name, sizeof(temp_name), 0);

    pthread_mutex_lock(&registration_lock);

    if(people_count >= MAX_PEOPLE){
        printf("Group full. Not accepted\n");
        close(client_fd);
        pthread_cancel(client_threads[people_count]);
    }

    printf("%s Joined the group chat\n", temp_name);
    strcpy(people[people_count], temp_name);
    people_sock[people_count++] = client_fd;

    pthread_mutex_unlock(&registration_lock);

    message M;
    while(recv(client_fd, &M, sizeof(M), 0)){
        if(M.type == 'G'){
            for(int i = 0; i < MAX_PEOPLE; i++){
                if(strcmp(people[i], M.snd) != 0){
                    send(people_sock[i], &M, sizeof(M), 0);
                }
            }
        }

        else{
            // int lol = 0;
            int from_fd;
            for(int i = 0; i < people_count; i++){
                if(strcmp(people[i], M.rcv) == 0){
                    send(people_sock[i], &M, sizeof(M), 0);
                    // lol = 1;
                }

                if(strcmp(people[i], M.snd) == 0){
                    from_fd = people_sock[i];
                }
            }

            // if(!lol){
            //     strcpy(failed.msg, "No User found\n");
            //     send(from_fd, &failed.msg, sizeof(failed), 0);
            //     printf("No user online with name: %s\n", M.rcv);
            // }

        }



    }

    printf("%s left the group\n", temp_name);
    pthread_mutex_lock(&registration_lock);

    int exit_person;
    for(int i = 0; i < MAX_PEOPLE; i++){
        if(strcmp(temp_name, people[i]) == 0){
            exit_person = i;
            break;
        }
    }
    for(exit_person; exit_person < people_count; exit_person++){
        people_sock[exit_person] = people_sock[exit_person + 1];
        strcpy(people[exit_person], people[exit_person + 1]);
    }

    people_count--;
    pthread_mutex_unlock(&registration_lock);
}

void print_people(){
    pthread_mutex_lock(&registration_lock);
    printf("People active :\n");
    for(int i = 0; i < people_count; i++){
        printf("%s\n", people[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&registration_lock);
}


int main(int argc, char const *argv[]) {

    int bind_res;
    int sock_d, connection_d, leng;
    struct sockaddr_in serv, clt;
    pthread_t rd, wr;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_fd <= 0){
        printf("Socket couldn't be created\n");
        return -1;
    }
    signal(SIGINT, close_it);

    //
    // int opt = 1;
    // setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORT);

    bind_res = bind(socket_fd, (struct sockaddr *)&serv, sizeof(serv));

    if(bind_res < 0){
        printf("Binding wasn't possible\n");
        return -1;
    }

    int listen_res = listen(socket_fd, 3);

    if(listen_res < 0){
        printf("Listen failed\n");
        return -1;
    }

    leng = sizeof(clt);

    for(;;){
        int *connection_fd = malloc(sizeof(int));
        *connection_fd = accept(socket_fd, (struct sockaddr*)&clt, &leng);

        printf("count: %d\n", people_count + 1);

        if(*connection_fd < 0){
            printf("Client connection failed\n");
            exit(0);
        }

        pthread_create(&client_threads[people_count], NULL, client_handler, (void *)connection_fd);
        print_people();
    }


    /* code */
    return 0;
}
