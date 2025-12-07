#include <stdio.h>      // Standard I/O functions
#include <string.h>     // String functions
#include <stdlib.h>     // exit()
#include <unistd.h>     // read(), close()
#include <arpa/inet.h>  // socket(), bind(), listen(), accept(), htons(), INADDR_ANY
#include <sys/select.h> // select(), fd_set, FD_* macros

#define SERVER_PORT 9001
#define MAX_CLIENTS 10
#define MESSAGE_BUFFER_SIZE 1024
#define NAME_BUFFER_SIZE 50

// Structure to store client info
typedef struct
{
    int socket_fd;               // Client socket descriptor
    char name[NAME_BUFFER_SIZE]; // Client's name
} ClientInfo;

int main()
{
    int listening_socket;  // Socket for the server to listen on
    int new_client_socket; // Socket returned by accept() for a new client
    int max_fd, activity, bytes_read;
    int current_socket;

    ClientInfo clients[MAX_CLIENTS];                   // Array to store connected clients
    char message_buffer[MESSAGE_BUFFER_SIZE];          // Buffer to read client messages
    struct sockaddr_in server_address, client_address; // Server & client addresses
    socklen_t client_address_length;                   // Length of client_address
    fd_set read_fds;                                   // Set of file descriptors for select()

    // Open chat log file in append mode
    FILE *chat_log_file = fopen("chat_log.txt", "a");
    if (!chat_log_file)
    {
        perror("Failed to open chat log file");
        return 1;
    }

    // Initialize all client slots
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].socket_fd = 0;  // 0 = empty slot
        clients[i].name[0] = '\0'; // empty string
    }

    // ----------------------------
    // Create server listening socket
    // ----------------------------
    listening_socket = socket(AF_INET, SOCK_STREAM, 0); // TCP/IPv4 socket
    if (listening_socket < 0)
    {
        perror("Failed to create server socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_address.sin_family = AF_INET;          // IPv4
    server_address.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any network interface
    server_address.sin_port = htons(SERVER_PORT); // Port (network byte order)

    // Bind server socket to IP and port
    if (bind(listening_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(listening_socket, 3) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %d\n", SERVER_PORT);
    fprintf(chat_log_file, "---- Chat Server Started ----\n");
    fflush(chat_log_file);

    client_address_length = sizeof(client_address);

    // ----------------------------
    // Main server loop
    // ----------------------------
    while (1)
    {
        FD_ZERO(&read_fds);                  // Clear the set
        FD_SET(listening_socket, &read_fds); // Add server socket to set
        max_fd = listening_socket;           // Track highest fd for select()

        // Add all connected clients to fd_set
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            current_socket = clients[i].socket_fd;
            if (current_socket > 0)
                FD_SET(current_socket, &read_fds);
            if (current_socket > max_fd)
                max_fd = current_socket;
        }

        // Wait for activity on any socket
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0)
        {
            perror("select() error");
        }

        // If there is a new incoming connection
        if (FD_ISSET(listening_socket, &read_fds))
        {
            new_client_socket = accept(listening_socket, (struct sockaddr *)&client_address, &client_address_length);
            if (new_client_socket < 0)
            {
                perror("accept() failed");
                exit(EXIT_FAILURE);
            }

            // Read client's name
            char client_name[NAME_BUFFER_SIZE];
            int name_length = read(new_client_socket, client_name, sizeof(client_name) - 1);
            if (name_length <= 0)
            {
                close(new_client_socket);
                continue;
            }
            client_name[name_length] = '\0';

            // Store client info in first available slot
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (clients[i].socket_fd == 0)
                {
                    clients[i].socket_fd = new_client_socket;
                    strncpy(clients[i].name, client_name, NAME_BUFFER_SIZE - 1);
                    clients[i].name[NAME_BUFFER_SIZE - 1] = '\0';
                    printf("%s connected. Socket fd = %d\n", clients[i].name, new_client_socket);
                    break;
                }
            }
        }

        // Check all clients for incoming messages
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            current_socket = clients[i].socket_fd;
            if (current_socket > 0 && FD_ISSET(current_socket, &read_fds))
            {
                bytes_read = read(current_socket, message_buffer, sizeof(message_buffer) - 1);
                if (bytes_read <= 0)
                {
                    // Client disconnected
                    printf("%s disconnected. Socket fd = %d\n", clients[i].name, current_socket);
                    close(current_socket);
                    clients[i].socket_fd = 0;
                    clients[i].name[0] = '\0';
                }
                else
                {
                    // Null-terminate the message
                    message_buffer[bytes_read] = '\0';

                    // Log message
                    fprintf(chat_log_file, "%s: %s\n", clients[i].name, message_buffer);
                    fflush(chat_log_file);

                    // Broadcast message to other clients
                    char broadcast_message[MESSAGE_BUFFER_SIZE + NAME_BUFFER_SIZE + 3];
                    snprintf(broadcast_message, sizeof(broadcast_message), "%s: %s", clients[i].name, message_buffer);
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        if (clients[j].socket_fd != 0 && j != i)
                        {
                            send(clients[j].socket_fd, broadcast_message, strlen(broadcast_message), 0);
                        }
                    }
                }
            }
        }
    }

    // Cleanup (never reached)
    fclose(chat_log_file);
    close(listening_socket);
    return 0;
}
