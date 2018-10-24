//
//  main.cpp
//  A2 Part2
//
//  Created by Hassaan on 07/10/2018.
//  Copyright © 2018 HaigaTech. All rights reserved.
//

#ifdef __APPLE__
#  define error printf
#endif

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
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/stat.h>



using namespace std;

#define HOST_NAME_LEN 100 // maximal host name length; can make it variable if you want
#define BUFLEN BUFSIZ // maximum response size; can make it variable if you want
//#define PORT_NO 13 // port of daytime server
#define WELCOME_MSG "*******************************\n**  Welcome from the Server  **      \n*******************************\n"
#define RECEIVING_MSG "Your message received"
//#define CLIENT_LIST_MSG "Client: File List\n"
#define SERVER_LIST_MSG "> Server: File List\n"
#define BASE_PATH "/Volumes/Parhai/Parhai/LUMS/Semester 5 Fall18/Computer Networks/Programming Assignments/A2/16030009/A2 Part4/A2 Part4 Server/A2 Part4 Server/"

void showErrorDetails();
void printCurrentPath();
bool doesFileExist(char* fileName);

int createFile(char* fileName);


int main(int argc, char *argv[])
{
    // check that there are enough parameters
    if (argc != 3) {
        //        fprintf(stderr,"Usage: myserver <hostname><port number between 1024 and 65535>: Success\n");
        fprintf(stderr, "Usage: myserver <hostname> <port number between 1024 and 65535>: Success");
        exit(-1);
    }
//    int value=0;
//    if(sscanf(argv[1], "%d%*c", &value) == 1)
//    {
//        fprintf(stderr,"Usage: myserver <hostname><port number between 1024 and 65535>: Success\n");
//        exit(1);
//    }
//    if((sscanf(argv[2], "%d%*c", &n) != 1) || atoi(argv[2])<=1024 || atoi(argv[2])>=65535)
//    {
//        fprintf(stderr,"Usage: myserver <hostname><port number between 1024 and 65535>: Success\n");
//        exit(1);
//    }

    int portNo = atoi(argv[2]);
    if (portNo<1024 || portNo>65535) {
        fprintf(stderr, "Usage: myserver <hostname> <port number between 1024 and 65535>: Success");
        exit(1);
    }

    // define your variables here

    struct hostent *hostDetails;
    struct sockaddr_in my_addr, remote_addr;
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

    bcopy(hostDetails->h_addr, &my_addr.sin_addr, hostDetails->h_length);
    my_addr.sin_port = htons(portNo);
    my_addr.sin_family = AF_INET;

    printf("Starting to run server at port %d\n", portNo);
    sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

    if(sd == -1){
        perror("setsockopt");
    }
    
    printf(".. binding socket to port:%d\n", portNo);
    if((::bind(sd, (struct sockaddr*)&my_addr, sizeof(my_addr) )) < 0) {
        perror("Server failed to bind\n");
        exit(2);
    }
    
    printf(".. starting to listen at the port\n");
    listen(sd, 5);
    unsigned int size = sizeof (struct sockaddr_in);
    int new_sd=0;
    char buffer[BUFLEN];
    memset(buffer, 0, BUFLEN);

    while(1)
    {
        // accepting the client's request and assigning new socket
        printf(".. waiting for a new connection\n");
        new_sd = accept (sd, (struct sockaddr *) &remote_addr, &size);
        if(new_sd == -1) {
            error("Server. Could not accept the request");
            return 0;
        }
        size_t dataSize = write(new_sd, WELCOME_MSG, strlen(WELCOME_MSG));
        memset(buffer, 0, BUFLEN);
        size_t bytesWritten=0;
        while(1)
        {
            dataSize = read(new_sd, buffer, BUFLEN);
            if (dataSize < 0) {
              error("ERROR reading from socket");
            }
            printf("c: %s\n", buffer);
            if(strcmp(buffer, "exit\n") == 0) {
                printf("The client closed the connection! \n\n");
                close(new_sd);
                break;
            }
            else if (strcmp(buffer, "list server\n") == 0) {
                char temp[BUFLEN];
                memset(temp, 0, BUFLEN);
                char *dirNames = NULL;
                
                printf("%s", SERVER_LIST_MSG);
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
                        if (dirNames == NULL) {
                            dirNames = strcat(temp, SERVER_LIST_MSG);
                            dirNames = strcat(temp, "");
                        }
                        dirNames = strcat(dirNames, "> ");
                        dirNames = strcat(dirNames, dir->d_name);
                        dirNames = strcat(dirNames, "\n");

                    }
                    printf("\n");
                    closedir(directory);
                }
                memset(buffer, 0, BUFLEN);
                ssize_t bytesWritten = 0;
                bytesWritten = write(new_sd, dirNames, strlen(dirNames));
                memset(buffer, 0, BUFLEN);
            }
            else if (strstr(buffer, "create server") != NULL) {
                char fileName[BUFLEN], temp[BUFLEN], temp1[BUFLEN];
                memset(fileName, 0, BUFLEN);
                memset(temp1, 0, BUFLEN);
                sscanf(buffer, "%s %s %s %s\n", temp, temp, fileName, temp1);
                // if incorrect parameters, display an invalid params msg
                if (strlen(fileName)==0 || strlen(temp1) > 0) {
                    printf("> Server: Invalid parameters\n");
                    strcpy(buffer, "> Server: Invalid parameters for creating file\n");
                    write(new_sd, buffer, strlen(buffer)+1);
                    memset(buffer, 0, BUFLEN);
                    continue;
                }
                
                if( access( fileName, F_OK ) != -1 ) {
                    // file exists
                    char response[BUFLEN] = "> Server: The file \'";
                    char *responseStr = strcat(response, fileName);
                    responseStr = strcat(response, "\' already exists.");
                    
                    printf("%s\n", responseStr);
                    if ((bytesWritten = write(new_sd, responseStr, strlen(responseStr)+1))<0)
                        showErrorDetails();
                    memset(buffer, 0, BUFLEN);
                    
                } else {
                    // file doesn't exist
                    int fd;
                    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
                    fd = creat(fileName, mode);
                    if (fd == -1) {
                        printf("Server: Unable to create file\n");
                        strcpy(buffer, "Server: Unable to create file\n");
                        write(new_sd, buffer, strlen(buffer)+1);
                    }
                    char response[BUFLEN] = "> Server: The file \'";
                    char *responseStr = strcat(response, fileName);
                    responseStr = strcat(response, "\' has been created.");
                    
                    printf("%s\n", responseStr);
                    if ((bytesWritten = write(new_sd, responseStr, strlen(responseStr)+1))<0)
                        showErrorDetails();
                    memset(buffer, 0, BUFLEN);
                }
            }
            else if (strstr(buffer, "send") != NULL) {
                char fileName[BUFLEN], temp[BUFLEN], temp1[BUFLEN];
                memset(fileName, 0, BUFLEN);
                memset(temp1, 0, BUFLEN);
                sscanf(buffer, "%s %s %s\n", temp, fileName, temp1);
                if (strlen(fileName)==0 || strlen(temp1) > 0) {
                    printf("> Server: Invalid parameters\n");
                    strcpy(buffer, "> Server: Invalid parameters for receiving file\n");
                    write(new_sd, buffer, strlen(buffer)+1);
                    memset(buffer, 0, BUFLEN);
                    continue;
                }
                char completeFileName[BUFLEN];
                memset(completeFileName, 0, BUFLEN);
                sprintf(completeFileName, "%s%s", BASE_PATH, fileName);

                if (doesFileExist(fileName)) {
                    char response[BUFLEN] = "> Server: The file \'";
                    char *responseStr = strcat(response, fileName);
                    responseStr = strcat(response, "\' already exists.");
                    printf("%s\n", responseStr);
                    
                    memset(response, 0, BUFLEN);
                    strcpy(response, "file exists\n");
                    if ((bytesWritten = write(new_sd, response, strlen(response)+1))<0)
                        showErrorDetails();
                    memset(buffer, 0, BUFLEN);
                    continue;
                }
                else {
                    char response[BUFLEN];
                    memset(response, 0, BUFLEN);
                    strcpy(response, "file does not exist\n");
                    if ((bytesWritten = write(new_sd, response, strlen(response)+1))<0)
                        showErrorDetails();
                    memset(buffer, 0, BUFLEN);
                }
                
                // Its a new file, so get ready to receive file.
                
                dataSize = read(new_sd, buffer, BUFLEN); // read file size
                int fileSize = atoi(buffer);
                dataSize = 0;
                size_t totalBytesReceived=0;
                createFile(fileName);
                //                    FILE* file = fopen(completeFileName, "w+");
                int fd = open(completeFileName, O_WRONLY | O_CREAT);
                if (fd == -1) {
                    perror("Error opening file");
                }
                else {
                    while (((dataSize=read(new_sd, buffer, BUFLEN)) > 0)  && strcmp(buffer, "end\n")!=0 ) {
                        printf("Data Size: %ld\n", dataSize);
                        write(fd, buffer, dataSize);
                        totalBytesReceived += dataSize;
                        printf("TotalBytes Received: %ld\n", totalBytesReceived);
                        printf("File Size: %d\n", fileSize);
                        if (totalBytesReceived >= fileSize) {
                            break;
                        }
                        memset(buffer, 0, BUFLEN);
                    }
                    memset(buffer, 0, BUFLEN);
                    close(fd);
                    
                    char response[BUFLEN] = "> Server: The file \'";
                    char *responseStr = strcat(response, fileName);
                    responseStr = strcat(response, "\' successfully received.");
                    
                    printf("%s\n", responseStr);
                    if ((bytesWritten = write(new_sd, responseStr, strlen(responseStr)+1))<0)
                        showErrorDetails();
                    memset(buffer, 0, BUFLEN);
                }
                

            }
            else {
                dataSize = write(new_sd, " ", 1);
                if (dataSize < 0) {
                    error("ERROR writing to socket");
                }
            }
        }
    }

    close(sd);
    return 0;
}

bool doesFileExist(char* fileName) {
    char completeFileName[BUFLEN];
    memset(completeFileName, 0, BUFLEN);
    sprintf(completeFileName, "%s%s", BASE_PATH, fileName);

    if( access( completeFileName, F_OK ) != -1 ) {
        // file exists
        return true;
    } else {
        // file doesn't exist
        return false;
    }
}

int createFile(char* fileName) {
    char completeFileName[BUFLEN];
    memset(completeFileName, 0, BUFLEN);
    sprintf(completeFileName, "%s%s", BASE_PATH, fileName);

    int fd;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    fd = creat(completeFileName, mode);
    if (fd == -1) {
        printf("Server: Unable to create file\n");
        return fd;
    }
    close(fd);
    return 0;
}


void receiveFile(int sockId) {
    
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

