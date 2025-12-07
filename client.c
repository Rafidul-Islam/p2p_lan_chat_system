    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <pthread.h>

    #define BUFFER_SIZE 1024
    #define NAME_SIZE 50

    char name[NAME_SIZE];
    int clientSocket;

    void *receiveMessages(void *arg) {
        char buffer[BUFFER_SIZE];
        int valread;
        while (1) {
            valread = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
            if (valread <= 0) {
                printf("\nServer disconnected.\n");
                close(clientSocket);
                exit(0);
            }
            buffer[valread] = '\0';
            printf("\n%s\n%s: ", buffer, name);
            fflush(stdout);
        }
        return NULL;
    }

    int main() {
        struct sockaddr_in serverAddr;
        char buffer[BUFFER_SIZE];

        printf("Enter your name: ");
        scanf("%49s", name);
        getchar();

        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket < 0) {
            perror("Socket error");
            exit(EXIT_FAILURE);
        }

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(9001);
        serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Replace with server IP

        if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
        }

        // Send name to server
        send(clientSocket, name, strlen(name), 0);

        printf("Connected to server.\n");

        pthread_t recvThread;
        pthread_create(&recvThread, NULL, receiveMessages, NULL);

        while (1) {
            printf("%s: ", name);
            if (!fgets(buffer, sizeof(buffer), stdin)) break;
            buffer[strcspn(buffer, "\n")] = '\0';
            send(clientSocket, buffer, strlen(buffer), 0);
        }

        close(clientSocket);
        return 0;
    }
