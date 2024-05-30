#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 3000

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];
    char message[256];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    while (fgets(message, sizeof(message), stdin) != NULL) {
        send(sock, message, strlen(message), 0);
        if (recv(sock, buffer, sizeof(buffer), 0) > 0) {
            printf("Server: %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}
