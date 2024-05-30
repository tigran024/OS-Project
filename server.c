#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 3000
#define MAX_CLIENTS 10

typedef struct {
    int current_balance;
    int min_balance;
    int max_balance;
    int is_frozen;
} Account;

Account *accounts;
int num_accounts;

void init_accounts(int n) {
    num_accounts = n;
    accounts = (Account *)malloc(n * sizeof(Account));
    for (int i = 0; i < n; i++) {
        accounts[i].current_balance = 0;
        accounts[i].min_balance = 0;
        accounts[i].max_balance = 1000;
        accounts[i].is_frozen = 0;
    }
}

void handle_request(char *request, char *response) {
    char command[256];
    int account_number, amount;

    sscanf(request, "%s %d %d", command, &account_number, &amount);

    if (strcmp(command, "print_balance") == 0) {
        if (account_number >= 0 && account_number < num_accounts) {
            sprintf(response, "Current balance of account %d: %d\n", account_number, accounts[account_number].current_balance);
        } else {
            sprintf(response, "Invalid account number.\n");
        }
    } else if (strcmp(command, "freeze_account") == 0) {
        if (account_number >= 0 && account_number < num_accounts) {
            accounts[account_number].is_frozen = 1;
            sprintf(response, "Account %d frozen.\n", account_number);
        } else {
            sprintf(response, "Invalid account number.\n");
        }
    } else if (strcmp(command, "unfreeze_account") == 0) {
        if (account_number >= 0 && account_number < num_accounts) {
            accounts[account_number].is_frozen = 0;
            sprintf(response, "Account %d unfrozen.\n", account_number);
        } else {
            sprintf(response, "Invalid account number.\n");
        }
    } else if (strcmp(command, "transfer") == 0) {
        int to_account;
        sscanf(request, "%*s %d %d %d", &account_number, &to_account, &amount);
        if (account_number >= 0 && account_number < num_accounts && to_account >= 0 && to_account < num_accounts) {
            if (!accounts[account_number].is_frozen && !accounts[to_account].is_frozen) {
                if (accounts[account_number].current_balance >= amount && (accounts[to_account].current_balance + amount) <= accounts[to_account].max_balance) {
                    accounts[account_number].current_balance -= amount;
                    accounts[to_account].current_balance += amount;
                    sprintf(response, "Transfer successful.\n");
                } else {
                    sprintf(response, "Transfer failed due to balance limits.\n");
                }
            } else {
                sprintf(response, "One of the accounts is frozen.\n");
            }
        } else {
            sprintf(response, "Invalid account number.\n");
        }
    } else {
        sprintf(response, "Unknown command.\n");
    }
}

void *handle_client(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);

    char buffer[1024];
    char response[1024];
    int bytes_read;

    while ((bytes_read = read(client_sock, buffer, sizeof(buffer))) > 0) {
        buffer[bytes_read] = '\0';
        handle_request(buffer, response);
        ssize_t bytes_written = write(client_sock, response, strlen(response));
        if (bytes_written <= 0) {
            perror("write");
            }
    }

    close(client_sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_accounts>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int n = atoi(argv[1]);
    init_accounts(n);

    int server_sock, client_sock, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    pthread_t thread_id;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while ((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_size)) > 0) {
        printf("Client connected\n");
        new_sock = (int*)malloc(sizeof(int));
        *new_sock = client_sock;
        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) < 0) {
            perror("pthread_create");
            free(new_sock);
        }
    }

    if (client_sock < 0) {
        perror("accept");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    close(server_sock);
    free(accounts);
    return 0;
}
