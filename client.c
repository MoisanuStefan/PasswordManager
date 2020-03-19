
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <stdbool.h>

int port = 2738;

void GetData(char *input_user, char *input_pass) {
    printf("Enter username: ");
    scanf("%s", input_user);
    getchar();
    printf("Enter password: ");
    scanf("%s", input_pass);
    getchar();


}

void PrintCommandOutput(char *commandOutput) {
    char *p;
    int step = 1;

    printf("|%-30s|%-30s|%-30s|%-30s|%-30s|%-30s|\n ", "Username", "Passsword", "Category", "Title", "URL", "Note");
    for (int i = 1; i <= 92; ++i) printf("__");
    printf("_");
    printf("\n");
    p = strtok(commandOutput, "\n");
    while (p != NULL) {
        if (step == 1) printf("|");
        printf("%-30s|", p);
        if (step < 6)
            step++;
        else {
            printf("\n ");
            step = 1;
            for (int i = 1; i <= 92; ++i) printf("__");
            printf("\n");
        }
        p = strtok(NULL, "\n");
    }
    printf("\n");
    fflush(stdout);
}

void GetAndSendNewCell(int sd, char *column) {
    char cell[100];
    int len;

    printf("Insert %s: ", column);
    fflush(stdout);

    fgets(cell, 100, stdin);

    /* eliminate \n */
    len = strlen(cell) - 1;
    cell[len] = '\0';
    if (-1 == write(sd, &len, sizeof(int))) {
        perror("Error: Could not write to server.\n");
        exit(3);
    }
    if (-1 == write(sd, cell, len)) {
        perror("Error: Could not write to server.\n");
        exit(3);
    }
}

void PrintGuide() {
    printf("List of commands:\n\t- 'print all': displays all information about all accounts\n");
    printf("\t- 'print by title input_title': displays all information about account with title = input_title\n");
    printf("\t- 'print by username input_username': displays all information about accounts with username = input_username\n");
    printf("\t- 'print by category input_category': displays all information about all accounts from category = input_category\n");
    printf("\t- 'add new': will allow you to fill each cell for a new account; insert 'x' for blank cells\n");
    printf("\t- 'remove all': removes all information about all accounts. (Requires confirmation)\n");
    printf("\t- 'remove by title input_title': removes all information about account with title = input_title\n");
    printf("\t- 'remove by username input_username': removes all information about account with username = input_username\n");
    printf("\t- 'remove by category input_category': removes all information about accounts from category = input_category\n");
    printf("\t- 'print guide': prints guide again\n");
    printf("\t- 'exit': logs out and ends session\n");
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    int sd, len, accessGranted = 0, option;
    struct sockaddr_in server;
    char input_master_user[50], input_master_pass[50], command[100], commandOutput[1000000];

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Eroare la socket().\n");
        exit(3);
    }


    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("[client]Eroare la connect().\n");
        exit(3);
    }

    printf("Welcome to your secure password manager!\nSelect option 1 to sign in, or option 2 to sign up.\n");

    printf("Option: ");
    scanf("%d", &option);
    getchar();
    login:
    GetData(input_master_user, input_master_pass);

    /* tell server: incoming sign up or sign in */
    if (-1 == write(sd, &option, sizeof(int))) {
        perror("Error: Could not write to server.\n");
        exit(3);
    }

    /* send master user and pass to server */
    len = strlen(input_master_user) + 1;
    if (-1 == write(sd, &len, sizeof(int))) {
        perror("Error: Could not write to server.\n");
        exit(3);
    }
    if (-1 == write(sd, input_master_user, len)) {
        perror("Error: Could not write to server.\n");
        exit(3);
    }
    len = strlen(input_master_pass) + 1;
    if (-1 == write(sd, &len, sizeof(int))) {
        perror("Error: Could not write to server.\n");
        exit(3);
    }
    if (-1 == write(sd, input_master_pass, len)) {
        perror("Error: Could not write to server.\n");
        exit(3);
    }

    if (option == 2) {
        printf("Signed up. Log in to enter account.\n");
        option = 1;
        goto login;
    }

    /* wait for confirmation from server that client logged in successfully */
    if (-1 == read(sd, &accessGranted, sizeof(int))) {
        perror("Error: Could not read from server.\n");
        exit(3);
    }

    if (accessGranted == 0) {
        char c;
        printf("Username or password incorrect. Try again?\t[y/n]\t");
        c = getchar();
        getchar();
        if (c == 'y')
            goto login;
        close(sd);
    }

        /* client logged in successfully */
    else {

        printf("Logged in successfully.\n");
        PrintGuide();

        while (1) {
            bzero(command, sizeof(command));
            bzero(commandOutput, sizeof(commandOutput));

            printf("Command: ");
            fgets(command, 100, stdin);

            if (strcmp(command, "print guide\n") == 0) {
                PrintGuide();
                continue;
            } else if (strcmp(command, "remove all\n") == 0) {
                char c;
                printf("Are you sure you want to delete all your accounts?\t[y/n]\t");
                c = getchar();
                getchar();
                if (c == 'n')
                    continue;
            }

            len = strlen(command);
            if (-1 == write(sd, &len, sizeof(int))) {
                perror("Error: Could not write to server.\n");
                exit(3);
            }
            if (-1 == write(sd, command, len)) {
                perror("Error: Could not write to server.\n");
                exit(3);
            }

            if (strcmp(command, "exit\n") == 0)
                break;
            else if (strstr(command, "add new") != NULL) {

                GetAndSendNewCell(sd, "username");
                GetAndSendNewCell(sd, "password");
                GetAndSendNewCell(sd, "category");
                GetAndSendNewCell(sd, "title");
                GetAndSendNewCell(sd, "URL");
                GetAndSendNewCell(sd, "note");

            }

            /* get response from server */
            if (-1 == read(sd, &len, sizeof(int))) {
                perror("Error: Could not read from server.\n");
                exit(3);
            }
            if (-1 == read(sd, &commandOutput, len)) {
                perror("Error: Could not read from server.\n");
                exit(3);
            }


            if (strstr(commandOutput, "unknown command") != NULL) {
                printf("Unknown command. Try again.\n\n");
                fflush(stdout);
            } else if (strstr(commandOutput, "0 rows") != NULL) {
                printf("No rows selected.\n\n");
                fflush(stdout);
            } else {
                if (strstr(commandOutput, "no output") == NULL) {
                    PrintCommandOutput(commandOutput);
                } else if (strstr(commandOutput, "insert") != NULL)
                    printf("New line inserted\n\n");
                else
                    printf("Account/accounts deleted\n\n");


            }


        }


    }

    return 0;

}





