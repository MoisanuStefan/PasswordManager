/* servTCPIt.c - Exemplu de server TCP iterativ
   Asteapta un nume de la clienti; intoarce clientului sirul
   "Hello nume".

   Autor: Lenuta Alboaie  <adria@infoiasi.ro> (c)2009
*/

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

/* portul folosit */
#define PORT 2737

/* codul de eroare returnat de anumite apeluri */
extern int errno;





int main () {
    struct sockaddr_in server;    // structura folosita de server
    struct sockaddr_in from;
    char query[500], *error;        //mesajul primit de la client
    char msgrasp[100] = " ";        //mesaj de raspuns pentru client
    int sd, len, accessGranted, bytes, isLine, option, optval = 1;            //descriptorul de socket
    pid_t pid;
    char input_master_user[50], input_master_pass[50], command[100], commandOutput[1000000];

    sqlite3_stmt *res;
    sqlite3* db;

    if (sqlite3_open("/home/stef/CLionProjects/CarolPheasant/Users.db", &db))
        perror("Error: Could not open database.\n");

    /* crearea unui socket */
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[server]Eroare la socket().\n");
        return errno;
    }

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,&optval,sizeof(optval));
    /* pregatirea structurilor de date */
    bzero(&server, sizeof(server));
    bzero(&from, sizeof(from));

    /* umplem structura folosita de server */
    /* stabilirea familiei de socket-uri */
    server.sin_family = AF_INET;
    /* acceptam orice adresa */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    /* utilizam un port utilizator */
    server.sin_port = htons(PORT);

    /* atasam socketul */
    if (bind(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("[server]Eroare la bind().\n");
        return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen(sd, 5) == -1) {
        perror("[server]Eroare la listen().\n");
        return errno;
    }

    printf("[server]Asteptam la portul %d...\n", PORT);
    fflush(stdout);
    /* servim in mod iterativ clientii... */
    while (1) {
        int client;
        int length = sizeof(from);



        /* acceptam un client (stare blocanta pina la realizarea conexiunii) */
        client = accept(sd, (struct sockaddr *) &from, &length);
        if (-1 == (pid = fork())) {
            perror("Error at fork");
        }
        /* eroare la acceptarea conexiunii de la un client */

        if (client < 0) {
            perror("[server]Eroare la accept().\n");
            continue;
        }
        if (pid == 0) {

            printf("Waiting for client request to sign in or sign up...\n");

            if (-1 == read(client, &option, sizeof(int))) {
                perror("Error: Could not read from client.\n");
                exit(3);
            }

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
            if ( option == 1) {
                sprintf(query, "SELECT * FROM users WHERE master_user = '%s'", input_master_user);
                sqlite3_prepare_v2(db, query, 500, &res, 0);

                if (sqlite3_step(res) != SQLITE_ROW) {
                    accessGranted = 0;
                    printf("Connection from client rejected: No such username.\n");

                } else {

                    if (strcmp(input_master_user, sqlite3_column_text(res, 0)) != 0 ||
                        strcmp(input_master_pass, sqlite3_column_text(res, 1)) != 0) {

                        accessGranted = 0;
                        printf("Connection from client rejected: Password incorrect.\n", client);
                    } else {
                        accessGranted = 1;
                        printf("Client connected: %s\n", input_master_user);
                        fflush(stdout);

                    }

                }

                if (-1 == write(client, &accessGranted, sizeof(int))) {
                    perror("Error: Could not write to client.\n");
                    exit(3);
                }

                if (accessGranted == 0)
                    close(client);
            }

            /* client wants to sign up */
            else{
                bzero(query, sizeof(query));
                sprintf(query, "INSERT INTO users VALUES('%s','%s')", input_master_user, input_master_pass);
                if (sqlite3_exec(db, query, NULL, 0,&error) != SQLITE_OK)
                    printf("%s", sqlite3_errmsg(db));
                printf("New account created. Client will reconnect to sign in.\n");
                close(client);
                exit(2);

            }

            if (accessGranted == 1)
            {

                if (-1 == read(client, &len, sizeof(int))) {
                    perror("Error: Could not read from client.\n");
                    exit(3);
                }
                if (-1 == read(client, command, len)) {
                    perror("Error: Could not read from client.\n");
                    exit(3);
                }

                printf("%s", command);
                fflush(stdout);
                if (strstr(command, "print all") != NULL){
                    strcpy(commandOutput, "good job Stef\n");
                    len = strlen(commandOutput);
                }
                else if (strstr(command, "print by title ") != NULL){

                }
                else if (strstr(command, "print by username ") != NULL){

                }
                else if (strstr(command, "print by category ") != NULL) {

                }
                else if (strstr(command, "add new") != NULL){

                }
                else if (strstr(command, "remove by title ") != NULL){

                }
                else if (strstr(command, "remove by username ") != NULL){

                }
                else if (strstr(command, "print guide") != NULL){

                }
                else{
                    printf("Unknown command. Try again\n");

                }

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

