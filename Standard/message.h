#pragma once
#define BUFSIZE 1024

typedef struct Message {
    char name[20];
    int msgtype; // 0 for group, 1 for personal
    char recipient_id[20];
    char group_id[20];
    unsigned long long timestamp;
    unsigned char msg[BUFSIZE];
    int voice_or_text; // 0 for voice, 1 for text message
} message;

typedef struct Init {
    char user_id[20];
    char group_id[20];
} init;
