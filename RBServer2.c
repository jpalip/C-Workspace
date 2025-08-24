#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <linux/stat.h>

const int PORT = 8080;

int sockfd, accepted, len;
struct sockaddr_in servaddr, cli;

// Allows server to talk to client
void* serverTalk(int sockfd, char *dstpath) {
    char buff[80];
    int n;
    // Loop for talk
    for (;;) {
        bzero(buff, 80);
        // read the message from client and copy it in buffer
        read(sockfd, buff, sizeof(buff)); 
        // if msg contains "Exit" then Client exits and chat ends.
        if ((strncmp("quit", buff, 4)) == 0 || (strncmp(" ", buff, 4)) == 0) {
            printf("Client Exited.\n");
            break;
        }
        // Variables for commands
        char filename[20];
        char srcpath[100];
        int src_fd, dst_fd;
        char dst_pathfile[100];
        /*char buffer[1000000];
        int remaining = 0;
        int file_size;
        ssize_t len;*/
        // Accepts PUSH command
        if((strncmp("PUSH", buff, 4)) == 0) {
            sscanf(buff, "%*[^\"]\"%31[^\"]\"", filename); // Get file name from text in quotes
            realpath(filename, srcpath);

            if((src_fd = open(srcpath, O_RDONLY)) < 0) {
                printf("open erro1!");
            }
            strcpy(dst_pathfile, dstpath); // Refresh destination path for files
            *dst_pathfile = *strcat(dst_pathfile, filename); // Gets BU path + filename
            if((dst_fd = open(dst_pathfile, O_WRONLY | O_CREAT)) < 0) { // Creates file w same name as filename
                printf("open error2!");
            }
            /*recv(sockfd, buffer, 1000000, 0);
            file_size = atoi(buffer);
            remaining = file_size;
            while((remaining > 0) && (len = recv(sockfd, buffer, 1000000, 0)) > 0) {
                write(dst_fd, buffer, sizeof(char));
                remaining -= len;
                printf("%ld bytes", len);
            }*/
            // function to copy and paste data into new file
            sendfile(dst_fd, src_fd, 0, 1000000);
            close(src_fd);
            close(dst_fd);
            strcpy(buff, "OK\n");
            write(sockfd, buff, sizeof(buff));
            printf("[Client -> Server] PUSH COMMAND EXECUTED\n");
        }
        // Accepts PULL command
        else if((strncmp("PULL", buff, 4)) == 0) {
            sscanf(buff, "%*[^\"]\"%31[^\"]\"", filename);
            strcpy(srcpath, dstpath);
            struct stat stat;

            *srcpath = *strcat(srcpath, filename);

            if((src_fd = open(srcpath, O_RDONLY)) < 0) {
                printf("Opening failed!");
            }
            fstat(src_fd, &stat);
            int size = stat.st_size;
            char buffer[50];
            snprintf(buffer, 50, "FileSize %d bytes\n", size);
            write(sockfd, buffer, 50);
            size = 0;
            //snprintf(buff, 20, "Filesize: %ld\n", size);
            //write(sockfd, buff, sizeof(size));
            bzero(buff, 80);
            realpath(filename, dst_pathfile);
            if((dst_fd = open(dst_pathfile, O_WRONLY | O_CREAT)) < 0) {
                printf("Opening2 failed!");
            }
            sendfile(dst_fd, src_fd, 0, 1000000);
            close(src_fd);
            close(dst_fd);
            strcpy(buff, "OK\n");
            write(sockfd, buff, sizeof(buff));
            printf("[Client -> Server] PULL COMMAND EXECUTED\n");    
        }
        // Reset command variables
        else if((strncmp("OK\n", buff, 4)) == 0) {
            bzero(filename, 20);
            bzero(srcpath, 100);
            bzero(dst_pathfile, 100);
        }
        // If no command entered, user is CHATting
        else {
            if ((strncmp("quit", buff, 4)) == 0 || (strncmp("", buff, 4)) == 0) {
                printf("Client Exited.\n");
                break;
            }
            printf("[Client -> Server] %sTo client : ", buff); // print buffer containing clients text
            bzero(buff, 80); // resets buff
            n = 0;
            // copy Server msg into buffer, then write it over
            while ((buff[n++] = getchar()) != '\n');
            write(sockfd, buff, sizeof(buff));
            if ((strncmp("quit", buff, 4)) == 0 || (strncmp("", buff, 4)) == 0) {
                printf("Server Exited.\n");
                break;
            }
        }
    }
    close(sockfd);
}

// Allows client to talk to server
void clientTalk(int sockfd) {
    char buff[80];
    int n;
    for (;;) {
        if ((strncmp(buff, "", 4)) == 0) {
            printf("Server Exits...\n");
            break;
        }
        bzero(buff, sizeof(buff));
        printf("[MSG -> Server]: ");
        n = 0;
        while ((buff[n++] = getchar()) != '\n');
        if ((strncmp(buff, "quit", 4)) == 0) {
            printf("Client Exits...\n");
            break;
        }
        write(sockfd, buff, sizeof(buff));
        bzero(buff, sizeof(buff));
        read(sockfd, buff, sizeof(buff));
        printf("[Server -> YOU]: %s", buff);
    }
    close(sockfd);
}

int main(int argc, char** argv) {

    // Do Server Stuff if just PORT and directory
    if(argc == 3) {
        //struct dirent *de;
        DIR *pDir;
        pDir = opendir(argv[2]);
        char *dstpath = argv[2];
        const int PORT = atoi(argv[1]);

        printf("SERVER\n");
        // socket creation and verification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            printf("! Socket Creation Failed !\n");
            exit(0);
        }
        else
            printf("* Socket Created *\n");
        bzero(&servaddr, sizeof(servaddr));
  
        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servaddr.sin_port = htons(PORT);
  
        // Binding newly created socket to given IP and verification
        if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
            printf("! Socket Binding Failed !\n");
            exit(0);
        }
        else
            printf("* Socket Binded *\n");
  
        // Now server is ready to listen and verification
        if ((listen(sockfd, 5)) != 0) {
            printf("! Listen failed !\n");
            exit(0);
        }
        else
            printf("Listening...\n");
        len = sizeof(cli);
  
        // Accept the data packet from client and verification
        // ACCEPT LOOP - CAN ACCEPT MULTIPLE CLIENTS AT ONCE
        // SERVER TALKS TO ONE CLIENT AT A TIME
        while((accepted = accept(sockfd, (struct sockaddr*)&cli, &len)) > 0) {
            serverTalk(accepted, dstpath);
        }
        if (accepted < 0) {
            printf("! Server Acccept Failed !\n");
            exit(0);
        }
  
        // After chatting close the socket
        close(sockfd);
    }

    // Do client stuff if args = 3 (port n IP)
    if(argc == 2) {
        int port = atoi(argv[1]);
        struct hostent *address;
        address = gethostbyname("127.0.0.1");

        printf("CLIENT\n");
        // socket create and varification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1) {
            printf("! Socket Creation Failed !\n");
            exit(0);
        }
        else
            printf("* Socket Created *\n");
        bzero(&servaddr, sizeof(servaddr));
  
        // assign IP, PORT
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
        servaddr.sin_addr.s_addr = INADDR_ANY;

        int connect_status = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  
        // connect the client socket to server socket
        if (connect_status == -1) {
            printf("! Failed to connect to Server !\n");
            exit(0);
        }
        else
            printf("* Established connection with Server *\n");
  
        clientTalk(sockfd);

        // close the socket
        close(sockfd);
    }
    return 0;
}