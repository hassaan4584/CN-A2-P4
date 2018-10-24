//
//  main.cpp
//  A2 Part3
//
//  Created by Hassaan on 23/10/2018.
//  Copyright © 2018 HaigaTech. All rights reserved.
//

#include <stdio.h> // basic I/O
#include <stdlib.h>
#include <sys/types.h> // standard system types
#include <netinet/in.h> // Internet address structures
#include <sys/socket.h> // socket API
#include <arpa/inet.h>
#include <netdb.h> // host to IP resolution
#include <string.h>
#include <unistd.h>
#include <iostream>
//#include <netinet/in.h>
#include <netinet/tcp.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

#define HOST_NAME_LEN 100 // maximal host name length; can make it variable if you want
#define BUFLEN 1024 // maximum response size; can make it variable if you want
//#define PORT_NO 13 // port of daytime server
#define CLIENT_LIST_MSG "Client: File List\n"
#define SERVER_LIST_MSG "Server: File List\n"
#define HELP_MSG "List of commands:\n list client\n list server\n create client file.txt\n create server file.txt\n exit\n"
#define BASE_PATH "/Volumes/Parhai/Parhai/LUMS/Semester 5 Fall18/Computer Networks/Programming Assignments/A2/16030009/A2 Part4/A2 Part4 Client/A2 Part4 Client/"

void showErrorDetails();
void printCurrentPath();

int main(int argc, char *argv[])
{
    // check that there are enough parameters
    if (argc != 3) {
        fprintf(stderr, "Usage: simpleclient <servername> <port number>\n");
        exit(-1);
    }

    // define your variables here

    int portNo = atoi(argv[2]);
    struct hostent *hostDetails;
    struct sockaddr_in addr;
    int on = 1;
    int sd; // Socket descriptor that would be used to communicate with the server
    char inputHostname[HOST_NAME_LEN+1];
    if (strlen(argv[1]) > HOST_NAME_LEN) {
        printf("Invalid Hostname. Too Long\n");
        exit(-1);
    }
    strcpy(inputHostname, argv[1]);

    if((hostDetails = gethostbyname(inputHostname)) == NULL){
        printf("The server can't be contacted\n");
        exit(-1);
    }

    // Write your code here

    bcopy(hostDetails->h_addr, &addr.sin_addr, hostDetails->h_length);
    addr.sin_port = htons(portNo);
    addr.sin_family = AF_INET;

    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if(sd == -1){
        perror("setsockopt");
    }
    if(connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
        showErrorDetails();
        close(sd);
        exit(-1);
    }
    else {
        char buffer[BUFLEN];
        memset(buffer, 0, BUFLEN);
        
        while(true)
        {
            memset(buffer, 0, BUFLEN);
            ssize_t bytesRead = 0;
            size_t bytesWritten = 0;
            if ( (bytesRead = read(sd, buffer, BUFLEN-1))< 0) {
                showErrorDetails();
            }
            if (strcmp(buffer, " ") != 0) {
                printf("%s\n",buffer);
            }
            printf("> ");
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN-3, stdin);
            int i=0;
            while( buffer[i] ) {
                buffer[i] = (tolower(buffer[i]));
                i++;
            }
            if(strcmp(buffer, "help\n") == 0) {
                printf(HELP_MSG);
                write(sd, buffer, strlen(buffer)+1);
                continue;
            }
            else if(strcmp(buffer, "exit\n") == 0) {
                write(sd, buffer, strlen(buffer)+1);
                break;
            }
            else if (strcmp(buffer, "list client\n") == 0) {
                printf("> %s", CLIENT_LIST_MSG);
                DIR *directory;
                struct dirent *dir;
                directory = opendir(BASE_PATH);
                if (directory)
                {
                    while ((dir = readdir(directory)) != NULL) {
                        if (strcmp(dir->d_name, ".")==0 || strcmp(dir->d_name, "..")==0) {
                            continue;
                        }
                        printf("> %s\n", dir->d_name);
                    }
                    printf("\n");
                    closedir(directory);
                }
                memset(buffer, 0, BUFLEN);
                strcpy(buffer,"Client printed it's own directory contents \n");
                ssize_t bytesWritten = 0;
                bytesWritten = write(sd, buffer, strlen(buffer));
                memset(buffer, 0, BUFLEN);
            }
            else if (strcmp(buffer, "list server\n") == 0) {
                bytesWritten = 0;
                if ( (bytesWritten = write(sd,buffer,strlen(buffer)+1)) < 0 ) {
                    showErrorDetails();
                }
            }
            else if (strstr(buffer, "create client") != NULL) {
                char fileName[BUFLEN], temp[BUFLEN], temp1[BUFLEN];
                memset(fileName, 0, BUFLEN);
                memset(temp1, 0, BUFLEN);
                sscanf(buffer, "%s %s %s %s\n", temp, temp, fileName, temp1);
                // if incorrect parameters, display and send server an invalid params msg
                if (strlen(fileName)==0 || strlen(temp1) > 0) {
                    printf("Client: Invalid parameters\n");
                    strcpy(buffer, "Client: Invalid parameters for creating file\n");
                    write(sd, buffer, strlen(buffer)+1);
                    memset(buffer, 0, BUFLEN);
                    continue;
                }

                if( access( fileName, F_OK ) != -1 ) {
                    // file exists
                    char response[BUFLEN] = "Client: The file \'";
                    char *responseStr = strcat(response, fileName);
                    responseStr = strcat(response, "\' already exists.");
                    
                    printf("> %s\n", responseStr);
                    if ((bytesWritten = write(sd, responseStr, strlen(responseStr)+1))<0)
                        showErrorDetails();
                    memset(buffer, 0, BUFLEN);

                } else {
                    // file doesn't exist
                    int fd;
                    mode_t mode = S_IRUSR | S_IWUSR | S_IRWXU | S_IRGRP | S_IRWXG | S_IRWXO | S_IROTH;
                    fd = creat(fileName, mode);
                    if (fd == -1) {
                        printf("Client: Unable to create file\n");
                        strcpy(buffer, "Client: Unable to create file\n");
                        write(sd, buffer, strlen(buffer)+1);
                    }
                    write(fd, buffer, strlen(buffer));
                    close(fd);
                    char response[BUFLEN] = "Client: The file \'";
                    char *responseStr = strcat(response, fileName);
                    responseStr = strcat(response, "\' has been created.");
                    
                    printf("> %s\n", responseStr);
                    if ((bytesWritten = write(sd, responseStr, strlen(responseStr)+1))<0)
                        showErrorDetails();
                    memset(buffer, 0, BUFLEN);
                }
            }
            else if (strstr(buffer, "create server") != NULL) {
                if ((bytesWritten = write(sd, buffer, strlen(buffer)+1))<0)
                    showErrorDetails();
                memset(buffer, 0, BUFLEN);
            }
            else if (strstr(buffer, "send") != NULL) {
                char fileName[BUFLEN], temp[BUFLEN], temp1[BUFLEN];
                memset(fileName, 0, BUFLEN);
                memset(temp1, 0, BUFLEN);
                sscanf(buffer, "%s %s %s\n", temp, fileName, temp1);
                if (strlen(fileName)==0 || strlen(temp1) > 0) {
                    printf("> Usage: send <filename>\n");
                    strcpy(buffer, "> Client: Invalid parameters for sending file\n");
                    write(sd, buffer, strlen(buffer)+1);
                    memset(buffer, 0, BUFLEN);
                    continue;
                }

                int fd;
                char cfile_size[BUFLEN], completeFileName[BUFLEN];
                memset(cfile_size, 0, BUFLEN);
                memset(completeFileName, 0, BUFLEN);
                sprintf(completeFileName, "%s%s", BASE_PATH, fileName);
                struct stat file_stat;
                off_t offset = 0;

                fd = open(completeFileName, O_RDONLY);
                if (fd == -1) {
                    fprintf(stderr, "> Error. %s\n", strerror(errno));
                    strcpy(buffer, "> Error. No such file");
                    write(sd, buffer, strlen(buffer)+1);
                    memset(buffer, 0, BUFLEN);
                    continue;
                }
                /* Get file stats */
                if (fstat(fd, &file_stat) < 0) {
                    fprintf(stderr, "Error. %s", strerror(errno));
                    strcpy(buffer, "> Error. No such file");
                    write(sd, buffer, strlen(buffer)+1);
                    memset(buffer, 0, BUFLEN);
                    continue;
                }
                
                // prepare server to receive the file
                if ((bytesWritten = write(sd, buffer, strlen(buffer)+1))<0) {
                    showErrorDetails();
                    strcpy(buffer, "> Error. No such file");
                    write(sd, buffer, strlen(buffer)+1);
                    memset(buffer, 0, BUFLEN);
                    continue;
                }
                memset(buffer, 0, BUFLEN);
                
                // learn from server if the file is already present or not.
                if ( (bytesRead = read(sd, buffer, BUFLEN-1))< 0) {
                    showErrorDetails();
                    strcpy(buffer, "> Error. No such file");
                    write(sd, buffer, strlen(buffer)+1);
                    memset(buffer, 0, BUFLEN);
                    continue;
                }
                else if (strcmp(buffer, "file exists\n") == 0) {
                    // If server already has the file
                    char response[BUFLEN] = "> Server: File \'";
                    char *responseStr = strcat(response, fileName);
                    responseStr = strcat(response, "\' already exists on the server.");
                    printf("%s\n", responseStr);
                    write(sd, responseStr, strlen(responseStr));
                    memset(buffer, 0, BUFLEN);
                    continue;
                }
                
                
                
                


                sprintf(cfile_size, "%lld", file_stat.st_size);
                printf("\nFile size : %s\n", cfile_size);
                write(sd, cfile_size, strlen(cfile_size)); // send file name
                char* ptr[BUFLEN];
                memset(ptr, 0, BUFLEN);
                
                size_t bytesSent = sendfile(fd, sd, 0, &offset, NULL, 0);
                close(fd);
                printf("BytesSent: %ld\n", bytesSent);
                printf("Offset: %lld\n", offset);
                if (bytesSent == -1) {
                    showErrorDetails();
                }
                else {
                    // send end signal
                    sleep(1);
                    write(sd, "end\n", strlen("end\n"));
                    printf("> File successfully sent\n");
                }
                memset(buffer, 0, BUFLEN);

            }
            else {
                printf("Unknown command. Type \'help\' for a list of available commands.\n");
                if ((bytesWritten = write(sd, buffer, strlen(buffer)+1))<0)
                    showErrorDetails();
                memset(buffer, 0, BUFLEN);
            }
        }
        printf("\n\nAlright... see you later!\n\n");
        printf("let's do this again sometime! \n");
        sleep(1);
    }

    close(sd);
    return 0;

}


void showErrorDetails() {
    if (errno == EACCES) {
        cout<<"Error. The destination address is a broadcast address.";
    }
    else if (errno == ETIMEDOUT) {
        cout<<"Error. Could not connect. Connection timed out.\n";
    }
    else if (errno == EADDRINUSE) {
        cout<<"Error. The address is already in use.\n";
    }
    else if (errno == ENOTCONN) {
        cout<<"Error. The socket is not connected.\n";
    }
    else if (errno == ENOTSOCK) {
        cout<<"Error. Socket operation on non-socket.\n";
    }
    else if (errno == ENETUNREACH) {
        cout<<"Error. The network is unreachable.\n";
    }
    else if (errno == ECONNREFUSED) {
        cout<<"Error. The server refused the connection.\n";
    }
    else if (errno == ENOENT) {
        cout<<"Error. The named socket does not exist.\n";
    }
    else {
        perror("");
    }

}

void printCurrentPath() {
    char cwd[BUFLEN];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
    } else {
        perror("getcwd() error");
    }
}
