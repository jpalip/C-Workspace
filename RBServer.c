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
#include <errno.h>
#include <time.h>
#include <sys/wait.h>

const int PORT = 8080;

int sockfd, accepted, len;
struct sockaddr_in servaddr, cli;
int login = 0;

// Allows server to talk to client
void *serverTalk(int sockfd, char *dstpath, DIR *pDir)
{
    char buff[80];
    int n;
    // Loop for talk
    for (;;)
    {
    start:
        bzero(buff, 80);
        // read the message from client and copy it in buffer
        read(sockfd, buff, sizeof(buff));

        // if msg contains "quit" then Client exited and chat ends.
        if ((strncmp("quit", buff, 4)) == 0 || (strncmp("", buff, 1)) == 0)
        {
            printf("Client Exited.\n");
            login = 0;
            break;
        }
        // Variables for commands
        char filename[20];
        char srcpath[100];
        int src_fd, dst_fd;
        char dst_pathfile[100];
        char actual[100];
        char user[20];
        char pass[20];
        FILE *fp;
        struct stat info;
        struct dirent *de;
        char buffer[256];

        fp = fopen("login.txt", "r");
        // Ensures correct LOGIN info for user
        if ((strncmp("LOGIN", buff, 4)) == 0)
        {
            bzero(buff, 80);
            strcat(buff, "Enter username: ");
            write(sockfd, buff, sizeof(buff)); // Write "Enter username"
            bzero(buff, sizeof(buff));         // clear buffer
            wait(NULL);
            read(sockfd, user, 80); // read username
            while (fscanf(fp, "%s", actual) != EOF && login != 1)
            {
                //printf("ACTUAL : %s\n", actual);
                char *token = strtok(actual, ":");
                while (token != NULL)
                {
                    // If username matches an Account
                    if (strcmp(user, token) == 0)
                    {
                        printf("USER: %s\n", user); // User accepted
                        bzero(buff, sizeof(buff));
                        write(sockfd, "Enter your password: ", 22);
                        read(sockfd, pass, sizeof(pass));
                        token = strtok(NULL, ":");
                        while (read(sockfd, buff, sizeof(buff)) != 0)
                        {
                            if (strncmp(pass, token, sizeof(pass)) == 0)
                            {
                                printf("LOGIN SUCCESS\n"); // Password accepted
                                write(sockfd, "* Login Successful *\n", 21);
                                login = 1;
                                break;
                            }
                            write(sockfd, "Invalid password - Try again: ", 31);
                            bzero(pass, sizeof(pass));
                            bzero(buff, sizeof(buff));
                            read(sockfd, pass, sizeof(pass));
                        }
                    }
                    else
                    {
                        write(sockfd, "Invalid User - Try again: ", 27);
                        bzero(user, sizeof(user));
                        bzero(buff, sizeof(buff));
                        read(sockfd, user, sizeof(buff));
                    }
                    if (login == 1)
                    {
                        break;
                    }
                }
            }
        }
        // Accepts PUSH command
        else if ((strncmp("PUSH", buff, 4)) == 0)
        {
            if (login != 1)
            {
                write(sockfd, "Error: Login before using commands.\n", 37);
                goto start;
            }

            sscanf(buff, "%*[^\"]\"%31[^\"]\"", filename); // Get file name from text in quotes
            char cwd[80];
            getcwd(cwd, sizeof(cwd));
            bzero(srcpath, sizeof(srcpath));
            strcat(srcpath, cwd);
            strcat(srcpath, "/");
            strcat(srcpath, filename);
            //realpath(filename, srcpath);                   // Get the path of the file being sent
            printf("Filename: %s\nPath: %s\n", filename, srcpath);

            if ((src_fd = open(srcpath, O_RDONLY)) < 0)
            {
                printf("open error1!\n");
            }
            strcpy(dst_pathfile, dstpath);                   // Refresh destination path for files
            *dst_pathfile = *strcat(dst_pathfile, filename); // Gets BU path + filename
            printf("DST PATH: %s\n", dst_pathfile);
            if ((dst_fd = open(dst_pathfile, O_WRONLY | O_CREAT)) < 0)
            { // Creates file w same name as filename
                printf("open error2!\n");
            }
            // function to copy and paste data into new file
            sendfile(dst_fd, src_fd, 0, 1000000);
            close(src_fd);
            close(dst_fd);
            bzero(buff, sizeof(buff));
            strcpy(buff, "OK\n");
            write(sockfd, buff, sizeof(buff));
            printf("[Client -> Server] PUSH COMMAND EXECUTED\n");
            bzero(buff, sizeof(buff));
        }
        // Accepts PULL command
        else if ((strncmp("PULL", buff, 4)) == 0)
        {
            if (login != 1)
            {
                write(sockfd, "Error: Login before using commands.\n", 37);
                goto start;
            }
            sscanf(buff, "%*[^\"]\"%31[^\"]\"", filename);
            strcpy(srcpath, dstpath);
            struct stat stat;
            printf("Filename: %s\n", filename);

            *srcpath = *strcat(srcpath, filename);
            printf("SRC PATH: %s\n", srcpath);

            if ((src_fd = open(srcpath, O_RDONLY)) < 0)
            {
                printf("Opening failed!");
            }
            fstat(src_fd, &stat);
            int size = stat.st_size;
            char buffer[50];
            snprintf(buffer, 50, "FileSize %d bytes\n", size);
            write(sockfd, buffer, 50);
            size = 0;
            bzero(buffer, sizeof(buffer));
            bzero(buff, sizeof(buff));
            realpath(filename, dst_pathfile);
            if ((dst_fd = open(dst_pathfile, O_WRONLY | O_CREAT)) < 0)
            {
                printf("Opening2 failed!\n");
            }
            sendfile(dst_fd, src_fd, 0, 1000000);
            close(src_fd);
            close(dst_fd);
            write(sockfd, "OK\n", 4);
            printf("[Client -> Server] PULL COMMAND EXECUTED\n");
            bzero(buff, sizeof(buff));
        }
        // Accepts MKDIR Command
        else if ((strncmp("MKDIR", buff, 5)) == 0)
        {
            if (login != 1)
            {
                write(sockfd, "Error: Login before using commands.\n", 37);
                goto start;
            }
            char dirName[20];
            memcpy(dirName, &buff[6], sizeof(buff) - 8);
            dirName[strcspn(dirName, "\n")] = 0;
            mkdir(dirName, 0700);
            bzero(buff, sizeof(buff));
            write(sockfd, "OK\n", 4);
            printf("[Client -> Server] DIRECTORY \"%s\" CREATED\n", dirName);
        }
        // Accepts CD Command
        else if ((strncmp("CD", buff, 2)) == 0)
        {
            if (login != 1)
            {
                write(sockfd, "Error: Login before using commands.\n", 37);
                goto start;
            }
            char newpath[30];
            memcpy(newpath, &buff[3], sizeof(buff) - 3);
            newpath[strcspn(newpath, "\n")] = 0;
            if (newpath == NULL)
            {
                printf("Failed to change Directory!\n");
                bzero(buff, sizeof(buff));
                write(sockfd, "FAILED\n", 8);
            }
            else
            {
                bzero(buff, sizeof(buff));
                write(sockfd, "OK\n", 4);
                pDir = opendir(newpath);
                strcat(dstpath, newpath);
                strcat(dstpath, "/");
                printf("NEW DST PATH : %s\n", dstpath);
                printf("[Client -> Server] CURRENT WORKING DIRECTORY \"%s\" CHANGED\n", newpath);
            }
        }
        // Accepts LS Command
        else if ((strncmp("LS", buff, 2)) == 0)
        {
            if (login != 1)
            {
                write(sockfd, "Error: Login before using commands.\n", 37);
                goto start;
            }
            write(sockfd, "START:\n", 8);
            while ((de = readdir(pDir)) != NULL)
            {
                char buf[80];
                // really dumb way to build the LS command
                stat(de->d_name, &info);
                bzero(buffer, sizeof(buffer));
                strcat(buffer, de->d_name);
                strcat(buffer, "\t\t");
                sprintf(buf, "%ld", info.st_size);
                strcat(buffer, buf);
                strcat(buffer, " bytes\t\t");
                bzero(buf, sizeof(buf));
                snprintf(buf, sizeof(buf), "Date: %s", asctime(localtime(&info.st_mtime)));
                strcat(buffer, buf);
                bzero(buf, sizeof(buf));
                write(sockfd, buffer, sizeof(buffer));
            }
            rewinddir(pDir);
            write(sockfd, "OK\n", 4);
            printf("[CLIENT -> Server] LS COMMAND EXECUTED\n");
        }
        // Reset command variables
        else if ((strncmp("OK\n", buff, 4)) == 0)
        {
            bzero(filename, 20);
            bzero(srcpath, 100);
            bzero(dst_pathfile, 100);
        }
    }
    close(sockfd);
}

// Allows client to talk to server
void clientTalk(int sockfd, DIR *pDir)
{
    char buff[80];
    int n;
    struct stat info;
    struct dirent *de;
    for (;;)
    {
        // Enters username
        if ((strncmp("Enter username: ", buff, 17)) == 0)
        {
            bzero(buff, 80);
            scanf("%s", buff);
            send(sockfd, buff, sizeof(buff), 0); // sends username
            bzero(buff, sizeof(buff));
            read(sockfd, buff, sizeof(buff)); // reads response
        }
        // Invalid Username
        else if (strncmp("Invalid User - Try again: ", buff, 27) == 0)
        {
            printf("[Server -> YOU]: %s", buff);
            bzero(buff, sizeof(buff));
            scanf("%s", buff);
            write(sockfd, buff, sizeof(buff)); // resends user
            bzero(buff, sizeof(buff));
            read(sockfd, buff, sizeof(buff)); // reads response
        }
        // Enters password
        else if (strncmp("Enter your password: ", buff, 22) == 0)
        {
            printf("[Server -> YOU]: %s", buff);
            bzero(buff, sizeof(buff));
            scanf("%s", buff);
            write(sockfd, buff, sizeof(buff)); // sends password
            bzero(buff, 80);
            read(sockfd, buff, sizeof(buff)); // reads response
        }
        // Invalid Password
        else if (strncmp("Invalid password - Try again: ", buff, 31) == 0)
        {
            printf("[Server -> YOU]: %s", buff);
            bzero(buff, sizeof(buff));
            scanf("%s", buff);
            write(sockfd, buff, sizeof(buff)); // resends password
            bzero(buff, 80);
            read(sockfd, buff, sizeof(buff)); // reads response
        }
        // Login Success
        else if (strncmp("* Login Successful *\n", buff, 21) == 0)
        {
            printf("[Server -> YOU]: %s", buff);
            bzero(buff, sizeof(buff));
            n = 0;
            while ((buff[n++] = getchar()) != '\n')
                ;
        }
        // PULL CMD Response - Filesize x Bytes \n OK
        else if (strstr(buff, "FileSize") != NULL)
        {
            fflush(stdout);
            bzero(buff, sizeof(buff));
            read(sockfd, buff, sizeof(buff));
            printf("[Server -> YOU]: %s", buff);
        }
        // LS CMD Response
        else if (strcmp(buff, "START:\n") == 0)
        {
            char buffer[256];
            while (strcmp(buffer, "OK\n") != 0)
            {
                printf("%s", buffer);
                read(sockfd, buffer, sizeof(buffer));
            }
            printf("[Server -> YOU]: %s", buffer);
            bzero(buffer, sizeof(buffer));
            goto input;
        }
        // User entered CMDs
        else
        {
            if ((strncmp(buff, "", 1)) == 0)
            {
                printf("Server Closed.\n");
                bzero(buff, sizeof(buff));
                break;
            }
        input:
            printf("[MSG -> Server]: ");
            bzero(buff, sizeof(buff));
            n = 0;
            while ((buff[n++] = getchar()) != '\n'); // User input MSG
            if ((strncmp(buff, "quit", 4)) == 0)
            {
                printf("Client Exits...\n");
                bzero(buff, sizeof(buff));
                break;
            }
            else if (strncmp(buff, "LOCALLS\n", 9) == 0)
            {
                while ((de = readdir(pDir)) != NULL)
                {
                    bzero(buff, sizeof(buff));
                    stat(de->d_name, &info);
                    printf("FileName: %s\t\tSize: %ld Bytes\t\tDate: %s", de->d_name, info.st_size, asctime(localtime(&info.st_mtime)));
                }
                rewinddir(pDir);
                goto input;
            }
            else if (strstr(buff, "LOCALCD") != NULL)
            {
                char dirName[50];
                memcpy(dirName, &buff[8], sizeof(buff) - 8);
                bzero(buff, sizeof(buff));
                dirName[strcspn(dirName, "\n")] = 0;
                if (chdir(dirName) == -1)
                {
                    printf("Failed to change Directory!\n");
                }
                else
                {
                    char cwd[100];
                    getcwd(cwd, sizeof(cwd));
                    strcat(cwd, "/");
                    pDir = opendir(cwd);
                    printf("Local Directory Changed - OK\n");
                }
                goto input;
            }
            write(sockfd, buff, sizeof(buff)); // Write user MSG over socket
            bzero(buff, sizeof(buff));
            read(sockfd, buff, sizeof(buff)); // Read & Print response
            printf("[Server -> YOU]: %s", buff);
        }
    }
    close(sockfd);
}

int main(int argc, char **argv)
{

    char user[20];
    char pass[50];
    char actual[100];
    int login = 0;
    // Do Server Stuff if just PORT and directory
    if (argc == 3)
    {
        DIR *pDir;
        pDir = opendir(argv[2]);
        char *dstpath = argv[2];
        const int PORT = atoi(argv[1]);

        printf("SERVER\n");
        // socket creation and verification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
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
        if ((bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        {
            printf("! Socket Binding Failed !\n");
            exit(0);
        }
        else
            printf("* Socket Binded *\n");

        // Now server is ready to listen and verification
        if ((listen(sockfd, 5)) != 0)
        {
            printf("! Listen failed !\n");
            exit(0);
        }
        else
            printf("Listening...\n");
        len = sizeof(cli);

        // Accept the data packet from client and verification
        // ACCEPT LOOP - CAN ACCEPT MULTIPLE CLIENTS AT ONCE
        // SERVER TALKS TO ONE CLIENT AT A TIME
        while ((accepted = accept(sockfd, (struct sockaddr *)&cli, &len)) > 0)
        {
            printf("* Client Accepted *\n");
            fflush(stdout);
            serverTalk(accepted, dstpath, pDir);
        }
        if (accepted < 0)
        {
            printf("! Server Acccept Failed !\n");
            exit(0);
        }

        // After chatting close the socket
        close(sockfd);
    }

    // Do client stuff if args = 2 (port)
    if (argc == 2)
    {
        int port = atoi(argv[1]);
        struct hostent *address;
        address = gethostbyname("127.0.0.1");
        DIR *pDir;
        pDir = opendir(".");

        printf("CLIENT\n");
        // socket create and varification
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd == -1)
        {
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

        int connect_status = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

        // connect the client socket to server socket
        if (connect_status == -1)
        {
            printf("! Failed to connect to Server !\n");
            exit(0);
        }
        else
            printf("* Connected To Server *\n");

        clientTalk(sockfd, pDir);

        // close the socket
        close(sockfd);
    }
    return 0;
}