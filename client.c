
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

extern int errno;

int port = 2737;

void GetData(char *input_user, char *input_pass)
{
    printf("Enter username: ");
    scanf("%s", input_user);
    getchar();
    printf("Enter password: ");
    scanf("%s", input_pass);
    getchar();



}


void PrintGuide() {
    printf("List of commands:\n\t- 'print all': displays all information about all accounts\n");
    printf("\t- 'print by title input_title': displays all information about account with title = input_title\n");
    printf("\t- 'print by username input_username': displays all information about accounts with username = input_username\n");
    printf("\t- 'print by category input_category': displays all information about all accounts from input_category category\n");
    printf("\t- 'add new': will allow you to fill each field for a new account\n");
    printf("\t- 'remove by title input_title': removes all information about account with title = input_title\n");
    printf("\t- 'remove by username input_username': removes all information about account with username = user_name\n");
    printf("\t- 'print guide': prints guide again\n");
    printf("\t- 'exit': logs out and ends session\n");
    fflush(stdout);
}
int main (int argc, char *argv[])
{
    int sd, len, accessGranted = 0, option, code;			// descriptorul de socket
    struct sockaddr_in server;	// structura folosita pentru conectare
    char msg[100];		// mesajul trimis
    char input_master_user[50], input_master_pass[50], command[100], commandOutput[1000000];
    size_t size = 100;
    
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("Eroare la socket().\n");
        return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons (port);

    if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
        perror ("[client]Eroare la connect().\n");
        return errno;
    }

    printf("Welcome to your secure password manager!\nSelect option 1 to sign in, or option 2 to sign up.\n");

    printf("Option: ");
    scanf("%d", &option);
    getchar();
    GetData(input_master_user, input_master_pass);

    /* tell server: incoming sign up or sign in */
    if (-1 == write(sd, &option, sizeof(int)))
    {
        perror("Error: Could not write to server.\n");
        exit(3);
    }

    /* send master user and pass to server */
    len = strlen(input_master_user) + 1;
    if (-1 == write(sd, &len, sizeof(int)))
    {
        perror("Error: Could not write to server.\n");
        exit(3);
    }
    if (-1 == write(sd, input_master_user, len))
    {
        perror("Error: Could not write to server.\n");
        exit(3);
    }
    len = strlen(input_master_pass) + 1;
    if (-1 == write(sd, &len, sizeof(int)))
    {
        perror("Error: Could not write to server.\n");
        exit(3);
    }
    if (-1 == write(sd, input_master_pass, len))
    {
        perror("Error: Could not write to server.\n");
        exit(3);
    }

    if (option == 2)
    {
        close(sd);
        printf("Signed up. Reconnect to sign in.\n");
        return 0;
    }

    /* wait for confirmation from server that client logged in successfully */
    if (-1 == read(sd, &accessGranted, sizeof(int)))
    {
        perror("Error: Could not read from server.\n");
        exit(3);
    }

    if (accessGranted == 0)
    {
        printf("Username or password incorrect. Reconnect to try again.\n");
        close(sd);
    }

        /* client logged in successfully */
    else{

        printf("Logged in successfully.\n");
        PrintGuide();

        while(1)
        {
            printf("Command: ");
            fgets(command, 100 ,stdin);


            printf("%s", command);
            fflush(stdout);
            if (strcmp(command, "exit") == 0)
                break;

            len = strlen(command);
            if (-1 == write(sd, &len, sizeof(int)))
            {
                perror("Error: Could not write to server.\n");
                exit(3);
            }
            if (-1 == write(sd, command, len))
            {
                perror("Error: Could not write to server.\n");
                exit(3);
            }

            if (-1 == read(sd, &len, sizeof(int)))
            {
                perror("Error: Could not read from server.\n");
                exit(3);
            }
            if (-1 == read(sd, &commandOutput, len))
            {
                perror("Error: Could not read from server.\n");
                exit(3);
            }

            if (strstr(commandOutput, "unknown command") != NULL)
            {
                printf("Unknown command. Try again.\n");
                fflush(stdout);
            }
            else if (strstr(commandOutput, "wrong argument") != NULL)
            {
                printf("Wrong argument. Try again.\n");
                fflush(stdout);
            }

            else{
                printf("%s", commandOutput);
                fflush(stdout);

            }



        }



    }

    return 0;

}





