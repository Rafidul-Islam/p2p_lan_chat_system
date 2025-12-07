#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/select.h>

#define SERVER_PORT 9898
#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024
#define NAME_BUFFER_SIZE 50

// To store client info
typedef struct
{
    int socket_fd;
    char name[NAME_BUFFER_SIZE];
} ClientInfo;

int main()
{
    int listening_socket;
    int new_client_socket;
    int max_fd, activity, bytes_read;
    int current_socket;

    ClientInfo clients[MAX_CLIENTS];
    char msg_buffer[BUFFER_SIZE];

    // struct sockaddr_in
    // {
    //     short sin_family;        // Address family (AF_INET for IPv4)
    //     unsigned short sin_port; // Port number (in network byte order)
    //     struct in_addr sin_addr; // IPv4 address (in network byte order)
    //     char sin_zero[8];        // Padding to make the structure the same size as sockaddr
    // };
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;
    fd_set read_fds;
    // typedef struct
    // {
    //     unsigned long fds_bits[FD_SETSIZE / (8 * sizeof(long))];
    // } fd_set;

    // Open File
    FILE *log_file = fopen("log_file.txt", "a");
    if (!log_file)
    {
        perror("Failed to open log_file");
        return 1;
    }

    // Initialize client slots
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        clients[i].socket_fd = 0;
        clients[i].name[0] = '\0';
    }

    // Create server listening socket;
    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket < 0)
    {
        perror("Failed to create a server socket");
        exit(EXIT_FAILURE); // return exits the function while exit kills the whole program
    }

    // Configure server address
    server_address.sin_family = AF_INET;          // IPv4
    server_address.sin_addr.s_addr = INADDR_ANY;  // Accenpt connections on any network interfac;
    server_address.sin_port = htons(SERVER_PORT); // Port

    // Bind server socket to IP and Port
    if (bind(listening_socket, (struct sockaddr *)&server_address, sizeof(server_address)) > 0)
    {
        perror("Bind Failed :(");
        exit(EXIT_FAILURE); // EXIT FAILURE = 1;
    }

    // Start Listening to incoming connections
    if (listen(listening_socket, 3)) // 3 is backlog(# client at a time)
    {
        perror("Listen Failed :(");
        exit(1);
    }

    printf("Server started on port %d\n", SERVER_PORT);
    fprintf(log_file, "---Chat Server Started---\n");
    fflush(log_file); // Forces the text to print in log_file

    // We could have used variable but we need struct to store additional info that will be used in accept function
    client_address_length = sizeof(client_address);

    // Main Server Loop
    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(listening_socket, &read_fds);
        max_fd = listening_socket;

        // Add all clients to fd_set
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            current_socket = clients[i].socket_fd; // We just make it more simplier here.
            if (current_socket > 0)
            {
                FD_SET(current_socket, &read_fds);
            }
            if (current_socket > max_fd)
            {
                max_fd = current_socket;
            }
        }

        // Wait for activity in any socket
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        printf("%d", activity);
        if (activity < 0)
        {
            perror("select() error");
        }
    }
}
