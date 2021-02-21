#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <json-c/json.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include "message.h"

#define PORT 8000

#define MAX_PEOPLE 256

#define N_GROUPS 5

int people_count = 0;
int socket_fd = 0;
char people[MAX_PEOPLE][20];

pthread_t client_threads[MAX_PEOPLE];
pthread_mutex_t registration_lock;

message failed;

// int n_groups = 5;
int g_sizes[N_GROUPS];
char all_users[N_GROUPS][100][20]; //n_groups, n_users, u_len
int people_sock[MAX_PEOPLE];

char groupnames[N_GROUPS][20];

void process_json(){
    FILE *fp;
    char buffer[1024];

    struct json_object *parsed_json; //entire json document
    struct json_object *groups;

    struct json_object *group;
    struct json_object *user;

    // char all_users[n_groups][100][100]; //n_groups, n_users, u_len

    // int g_sizes[5];

    fp = fopen("server_groups.json", "r");
    fread(buffer, 1024, 1, fp);
    fclose(fp);

    parsed_json = json_tokener_parse(buffer);

    json_object_object_get_ex(parsed_json, "groups", &groups);

    for(int i = 0; i < 5; i++){
        char gname[100];
        sprintf(gname, "group_%d", i+1);
        // printf("%s\n", gname);
        json_object_object_get_ex(groups, gname, &group);

        int n_users = json_object_array_length(group);
        // printf("%s : found %d users\n", gname, n_users);
        g_sizes[i] = n_users;
        strcpy(groupnames[i], gname);
        for(int j = 0; j < n_users; j++){
            user = json_object_array_get_idx(group, j);
            strcpy(all_users[i][j], json_object_get_string(user));
            // printf("%s\t", json_object_get_string(user));
        }
        // printf("\n");
    }

    for(int i = 0; i < N_GROUPS; i++){
        printf("group %d, users = %d\n", i+1, g_sizes[i]);
        for(int j = 0; j < g_sizes[i]; j++){
            printf("%s\t", all_users[i][j]);
        }
        printf("\n-\n");
    }
    // printf("-\n");
}

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

void print_people(){
    pthread_mutex_lock(&registration_lock);
    printf("People Active :\n");
    for(int i = 0; i < people_count; i++){
        printf("%s\n", people[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&registration_lock);
}

int find_index(char name[20]){
    for(int i = 0; i < people_count; i++){
        if(strcmp(people[i], name) == 0){
            return i;
        }
    }
    return -1;
}

void *client_handler(void *socket_fd){
    int client_fd = *(int *)socket_fd;
    char temp_name[20] = {0};
    recv(client_fd, &temp_name, sizeof(temp_name), 0);

    pthread_mutex_lock(&registration_lock);

    if(people_count >= MAX_PEOPLE){
        printf("Group full. Not accepted\n");
        close(client_fd);
        pthread_cancel(client_threads[people_count]);
    }

    int check = 0;
    for(int i = 0; i < N_GROUPS; i++){
        // printf("group %d, users = %d\n", i+1, g_sizes[i]);
        for(int j = 0; j < g_sizes[i]; j++){
            if(!strcmp(temp_name, all_users[i][j])){
                check = 1;
            }
        }
    }

    if(check == 0){
        printf("User invalid\n");
        close(client_fd);
        pthread_cancel(client_threads[people_count]);
    }

    printf("%s joined the group chat\n", temp_name);
    strcpy(people[people_count], temp_name);
    people_sock[people_count++] = client_fd;

    pthread_mutex_unlock(&registration_lock);

    message M;
    while(recv(client_fd, &M, sizeof(M), 0)){
        if(M.msgtype == 0){
            for(int i = 0; i < N_GROUPS; i++){
                for(int j = 0; j < g_sizes[i]; j++){
                    if(!strcmp(M.name, all_users[i][j])){
                        strcpy(M.group_id, groupnames[i]);
                        for(int k = 0; k < g_sizes[i]; k++){
                            if(strcmp(all_users[i][k], M.recipient_id) != 0){
                                int sock_idx = find_index(all_users[i][k]);
                                send(people_sock[sock_idx], &M, sizeof(M), 0);
                            }
                        }
                    }
                }
            }
        }

        else{//direct
            int from_fd;
            for(int i = 0; i < people_count; i++){
                if(strcmp(people[i], M.recipient_id) == 0){
                    int sock_idxd = find_index(people[i]);
                    send(people_sock[sock_idxd], &M, sizeof(M), 0);
                }

                if(strcmp(people[i], M.name) == 0){
                    from_fd = people_sock[find_index(people[i])];
                }
            }
        }
    }

    printf("%s left the chat\n", temp_name);
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

int main(int argc, char const *argv[]) {
    process_json();

    printf("Waiting for connections....\n");

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

    return 0;

}