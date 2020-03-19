
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>


#define PORT 2738

char *GetNewCell(int client, char *cell) {

    int len;

    if (-1 == read(client, &len, sizeof(int))) {
        perror("Error: Could not read from client.\n");
        exit(3);
    }
    if (-1 == read(client, cell, len)) {
        perror("Error: Could not read from client.\n");
        exit(3);
    }
    return cell;

}

void ExecuteSelectQuery(sqlite3 *db, sqlite3_stmt *res, char *query, char *queryArg, char *commandOutput, char *user,
                        int *len) {
    sqlite3_open("/home/stef/CLionProjects/CarolPheasant/Users.db", &db);

    /* prepare res structure for query */
    sqlite3_prepare_v2(db, query, -1, &res, NULL);
    /* replace ? with argument */
    sqlite3_bind_text(res, 1, user, -1, SQLITE_STATIC);
    if (queryArg != NULL)
        sqlite3_bind_text(res, 2, queryArg, -1, SQLITE_STATIC);

    int isRow = 0;

    /* concatenate each line form output */
    while (sqlite3_step(res) == SQLITE_ROW) {
        isRow = 1;
        for (int i = 0; i < 6; ++i) {
            strcat(commandOutput, sqlite3_column_text(res, i));
            strcat(commandOutput, "\n");
        }
    }
    /* prepare for next query and close db */
    sqlite3_finalize(res);
    sqlite3_close(db);

    /* let client know no lines ware returned */
    if (isRow == 0) {
        strcpy(commandOutput, "0 rows");
    }
    *len = strlen(commandOutput);
}

void ExecuteDeleteQuery(sqlite3 *db, sqlite3_stmt *res, char *query, char *queryArg, char *commandOutput, char *user,
                        int *len) {
    if (sqlite3_open("/home/stef/CLionProjects/CarolPheasant/Users.db", &db))
        perror("Error: Could not open database.\n");

    /* prepare res structure for query */
    sqlite3_prepare_v2(db, query, -1, &res, NULL);
    /* replace ? with arguement */
    if (queryArg != NULL)
        sqlite3_bind_text(res, 1, queryArg, -1, SQLITE_STATIC);
    /* execute query */
    sqlite3_step(res);
    /* prepare for next query and close db */
    sqlite3_finalize(res);
    sqlite3_close(db);
    /* write to client that delete query was executed */
    strcpy(commandOutput, "no output delete");
    *len = strlen(commandOutput);
}

int main() {
    struct sockaddr_in server;
    struct sockaddr_in from;
    char *query;
    int sd, len, accessGranted, option, optval = 1;
    pid_t pid;
    char input_master_user[50], input_master_pass[50], command[100], commandOutput[1000000], argument[100];
    char cell1[100], cell2[100], cell3[100], cell4[100], cell5[100], cell6[100];

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    if (listen(sd, 5) == -1) {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    printf("[server]Waiting for connection at port %d...\n", PORT);
    fflush(stdout);

    while (1) {
        int client, bytes;
        int length = sizeof(from);

        client = accept(sd, (struct sockaddr *) &from, &length);
        if (-1 == (pid = fork())) {
            perror("Error at fork");
        }
        if (client < 0) {
            perror("[server]Eroare la accept().\n");
            continue;
        }
        if (pid == 0) {
            sqlite3_stmt *res;
            sqlite3 *db;

            login:
            printf("[%d] Waiting for client request to sign in or sign up...\n", getpid());

            /* client says if he wants to sign in or sign up */
            if (-1 == (bytes = read(client, &option, sizeof(int)))) {
                perror("Error: Could not read from client.\n");
                exit(3);
            }

            if (bytes == 0) {
                printf("[%d] Client disconnected unexpectedly.\n", getpid());
                close(client);
                return 0;
            }

            /* get sign in/up info */
            if (-1 == read(client, &len, sizeof(int))) {
                perror("Error: Could not read from client.\n");
                exit(3);
            }
            if (-1 == read(client, input_master_user, len)) {
                perror("Error: Could not read from client.\n");
                exit(3);
            }

            if (-1 == read(client, &len, sizeof(int))) {
                perror("Error: Could not read from client.\n");
                exit(3);
            }
            if (-1 == read(client, input_master_pass, len)) {
                perror("Error: Could not read from client.\n");
                exit(3);
            }

            /* client wants to sign in */
            if (option == 1) {

                sqlite3_open("/home/stef/CLionProjects/CarolPheasant/Users.db", &db);

                /* check in db if username exists */
                query = "SELECT * FROM users WHERE master_user = ?";
                sqlite3_prepare_v2(db, query, -1, &res, NULL);

                sqlite3_bind_text(res, 1, input_master_user, -1, SQLITE_STATIC);
                if (sqlite3_step(res) != SQLITE_ROW) {
                    accessGranted = 0;
                    sqlite3_finalize(res);
                    sqlite3_close(db);
                    printf("[%d] Connection from client rejected: No such username.\n", getpid());

                } else {
                    /* if user or password dont match */
                    if (strcmp(input_master_user, sqlite3_column_text(res, 0)) != 0 ||
                        strcmp(input_master_pass, sqlite3_column_text(res, 1)) != 0) {

                        accessGranted = 0;
                        sqlite3_finalize(res);
                        sqlite3_close(db);
                        printf("[%d] Connection from client rejected: Password incorrect.\n", getpid());
                    } else {
                        accessGranted = 1;
                        sqlite3_finalize(res);
                        sqlite3_close(db);
                        printf("[%d] Client connected: %s\n", getpid(), input_master_user);
                        fflush(stdout);

                    }

                }

                if (-1 == write(client, &accessGranted, sizeof(int))) {
                    perror("Error: Could not write to client.\n");
                    exit(3);
                }

                if (accessGranted == 0) {

                    goto login;
                }
            }

                /* client wants to sign up */
            else {

                sqlite3_open("/home/stef/CLionProjects/CarolPheasant/Users.db", &db);

                query = "INSERT INTO users VALUES(?,?)";
                sqlite3_prepare_v2(db, query, -1, &res, NULL);

                sqlite3_bind_text(res, 1, input_master_user, -1, SQLITE_STATIC);
                sqlite3_bind_text(res, 2, input_master_pass, -1, SQLITE_STATIC);

                sqlite3_step(res);

                sqlite3_finalize(res);
                sqlite3_close(db);
                printf("[%d] New account created. Client will login to access account.\n", getpid());
                goto login;

            }

            if (accessGranted == 1) {

                while (1) {
                    bzero(cell1, sizeof(cell1));
                    bzero(cell2, sizeof(cell2));
                    bzero(cell3, sizeof(cell3));
                    bzero(cell4, sizeof(cell4));
                    bzero(cell5, sizeof(cell5));

                    bzero(commandOutput, sizeof(commandOutput));
                    bzero(command, sizeof(command));

                    /* read command from client */
                    if (-1 == (bytes = read(client, &len, sizeof(int)))) {
                        perror("Error: Could not read from client.\n");
                        exit(3);
                    }
                    if (bytes == 0) {
                        printf("[%s] Client disconnected unexpectedly.\n", input_master_user);
                        close(client);
                        return 0;
                    }
                    if (-1 == read(client, command, len)) {
                        perror("Error: Could not read from client.\n");
                        exit(3);
                    }


                    bzero(commandOutput, sizeof(commandOutput));

                    if (strstr(command, "print all") != NULL) {

                        query = "SELECT username, password, category, title, url, note FROM accounts WHERE master_user=?";
                        ExecuteSelectQuery(db, res, query, NULL, commandOutput, input_master_user, &len);
                        printf("[%s] Executed command: print all\n", input_master_user);
                    } else if (strstr(command, "print by title ") != NULL) {
                        strcpy(argument, command + 15);
                        argument[strlen(argument) - 1] = '\0';

                        query = "SELECT username, password, category, title, url, note FROM accounts WHERE master_user=? and title = ?";
                        ExecuteSelectQuery(db, res, query, argument, commandOutput, input_master_user, &len);
                        printf("[%s] Executed command: print by title\n", input_master_user);

                    } else if (strstr(command, "print by username ") != NULL) {
                        strcpy(argument, command + 18);
                        argument[strlen(argument) - 1] = '\0';

                        query = "SELECT username, password, category, title, url, note FROM accounts WHERE master_user=? and username = ?";
                        ExecuteSelectQuery(db, res, query, argument, commandOutput, input_master_user, &len);
                        printf("[%s] Executed command: print by username\n", input_master_user);

                    } else if (strstr(command, "print by category ") != NULL) {
                        strcpy(argument, command + 18);
                        argument[strlen(argument) - 1] = '\0';

                        query = "SELECT username, password, category, title, url, note FROM accounts WHERE master_user=? and category = ?";
                        ExecuteSelectQuery(db, res, query, argument, commandOutput, input_master_user, &len);
                        printf("[%s] Executed command: print by category\n", input_master_user);

                    } else if (strstr(command, "add new") != NULL) {

                        /* read from client each table cell to be filled in */
                        GetNewCell(client, cell1);
                        GetNewCell(client, cell2);
                        GetNewCell(client, cell3);
                        GetNewCell(client, cell4);
                        GetNewCell(client, cell5);
                        GetNewCell(client, cell6);

                        sqlite3_open("/home/stef/CLionProjects/CarolPheasant/Users.db", &db);

                        char addQuery[100];
                        sprintf(addQuery,
                                "INSERT INTO accounts VALUES (\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\")",
                                input_master_user, cell1, cell2, cell3, cell4, cell5, cell6);
                        sqlite3_prepare_v2(db, addQuery, -1, &res, NULL);


                        sqlite3_step(res);
                        sqlite3_finalize(res);
                        sqlite3_close(db);

                        strcpy(commandOutput, "no output insert");
                        len = strlen(commandOutput);

                        printf("[%s] Executed command: add new\n", input_master_user);


                    } else if (strstr(command, "remove all") != NULL) {

                        query = "DELETE FROM accounts";
                        ExecuteDeleteQuery(db, res, query, NULL, commandOutput, input_master_user, &len);
                        printf("[%s] Executed command: remove all\n", input_master_user);

                    } else if (strstr(command, "remove by title ") != NULL) {
                        strcpy(argument, command + 16);
                        argument[strlen(argument) - 1] = '\0';

                        query = "DELETE FROM accounts WHERE title = ?";
                        ExecuteDeleteQuery(db, res, query, argument, commandOutput, input_master_user, &len);

                        printf("[%s] Executed command: remove by title\n", input_master_user);


                    } else if (strstr(command, "remove by username ") != NULL) {
                        strcpy(argument, command + 19);
                        argument[strlen(argument) - 1] = '\0';

                        query = "DELETE FROM accounts WHERE username = ?";
                        ExecuteDeleteQuery(db, res, query, argument, commandOutput, input_master_user, &len);
                        printf("[%s] Executed command: remove by username\n", input_master_user);


                    } else if (strstr(command, "remove by category ") != NULL) {
                        strcpy(argument, command + 19);
                        argument[strlen(argument) - 1] = '\0';

                        query = "DELETE FROM accounts WHERE category = ?";
                        ExecuteDeleteQuery(db, res, query, argument, commandOutput, input_master_user, &len);
                        printf("[%s] Executed command: remove by category\n", input_master_user);


                    } else {
                        strcpy(commandOutput, "unknown command");
                        len = strlen(commandOutput);
                        printf("[%s] Unknown command\n", input_master_user);


                    }

                    /* write the response to client */
                    if (-1 == write(client, &len, sizeof(int))) {
                        perror("Error: Could not write to client.\n");
                        exit(3);
                    }
                    if (-1 == write(client, &commandOutput, len)) {
                        perror("Error: Could not write to client.\n");
                        exit(3);
                    }


                }

            }


        }
    }

}

