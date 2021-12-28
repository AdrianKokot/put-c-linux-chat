#include "shared/shared.h"

#define MAX_USERS 5

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int messageQueueId;

void terminateServer() {
    msgctl(messageQueueId, IPC_RMID, NULL);
    signal(SIGINT,SIG_DFL);
    kill(-getpid(), SIGINT);
}

int currentUsers = 0;
typedef struct {
    long id;
    char username[255];
    int channelId;
} User;
User users[MAX_USERS];

bool isServerFull() {
    return currentUsers + 1 >= MAX_USERS;
}

bool isUsernameUnique(char *username) {

    for (int i = 0; i < currentUsers; i++) {
        if (strcmp(users[i].username, username) == 0) {
            return false;
        }
    }

    return true;
}

long addUser(char *username, long userId) {
    currentUsers++;
    users[currentUsers].id = userId;
    strcpy(users[currentUsers].username, username);
    return users[currentUsers].id;
}

int nextFreeType = 1000;

int main(int argc, char *argv[]) {


    signal(SIGINT, terminateServer);

    messageQueueId = msgget(0x1234, IPC_CREAT | 0644);

    if (messageQueueId == -1) {
        printf("Error occurred during creation of message queue.\n");
        return 0;
    }

    nextFreeType += messageQueueId * 100;

    Request request;
    char *responseBody = malloc(REQUEST_BODY_MAX_SIZE * sizeof(char));


    while (true) {

        (int) msgrcv(messageQueueId, &request, REQUEST_SIZE, 0, 0);
        memset(responseBody, 0, REQUEST_BODY_MAX_SIZE);

        printfDebug("Received request of type %lu: %.*s\n", request.type, request.bodyLength, request.body);

        switch (request.rtype) {
            case R_Init: {
                printfDebug("%s\n", "Received R_Init message");

                if (isServerFull()) {
                printfDebug("%s\n", "Server is full. Sending response.");
                    sendResponse(messageQueueId, R_Init, "", R_Init, StatusServerFull);
                } else {
                printfDebug("%s\n", "Server is not full. Sending response.");
                    sprintf(responseBody, "%d", nextFreeType++);
                    sendResponse(messageQueueId, R_Init, responseBody, R_Init, StatusOK);
                }

                break;
            }
            case R_RegisterUser: {
                printfDebug("%s\n", "Received R_RegisterUser message");

                if (isServerFull()) {
                    printfDebug("%s\n", "Server is full. Sending response.");

                    sendResponse(messageQueueId, request.type, "Server is full.", R_RegisterUser, StatusServerFull);
                } else {
                    char *requestedUsername = malloc(sizeof(char) * request.bodyLength);
                    snprintf(requestedUsername, request.bodyLength + 1, "%s", request.body);

                    if (isUsernameUnique(requestedUsername)) {
                        printfDebug("Username (%s) is unique. Sending response.\n", requestedUsername);

                        addUser(requestedUsername, request.type);

                        sendResponse(messageQueueId, request.type, "", R_RegisterUser, StatusOK);
                    } else {
                        printfDebug("%s\n", "Username is not unique. Sending response.");
                        sendResponse(messageQueueId, request.type, "taken", R_RegisterUser, StatusValidationError);
                    }
                }

                break;
            }
        }

        sleep(2);

    }

    return 0;
}

#pragma clang diagnostic pop