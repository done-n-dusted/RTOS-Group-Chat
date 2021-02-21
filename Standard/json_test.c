#include <stdio.h>
#include <json-c/json.h>
#include <string.h>

int main(int argc, char const *argv[]) {
    FILE *fp;
    char buffer[1024];

    struct json_object *parsed_json; //entire json document
    struct json_object *groups;

    struct json_object *group;
    struct json_object *user;

    int n_groups = 5;
    // size_t i;

    char all_users[n_groups][100][100]; //n_groups, n_users, u_len

    int g_sizes[5];

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
        for(int j = 0; j < n_users; j++){
            user = json_object_array_get_idx(group, j);
            strcpy(all_users[i][j], json_object_get_string(user));
            // printf("%s\t", json_object_get_string(user));
        }
        // printf("\n");
    }

    for(int i = 0; i < n_groups; i++){
        printf("group %d, users = %d\n", i+1, g_sizes[i]);
        for(int j = 0; j < g_sizes[i]; j++){
            printf("%s\t", all_users[i][j]);
        }
        printf("\n\n");
    }
    
    return 0;
}