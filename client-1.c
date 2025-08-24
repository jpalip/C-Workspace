#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include<sys/stat.h>



#define BACKLOG 10	 // how many pending connections queue will hold


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char const *argv[])
{

	int sockfd, numbytes,rv;
	//char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];

	if (argc != 3) {
		fprintf(stderr,"usage: client hostname\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure


	//infinite loop
	while(1){

		//get command type from user
		char input[4];
		printf("If you want to send a file type 'PUSH', to receive a file type 'PULL', type 'QUIT' to leave \n");
	    fflush(stdout);
		scanf("%s", input);


        //if the user wants to send a push command
		if(strcmp(input,"PUSH") == 0){

			FILE *fp;//Declare a file pointer
			char fileName[20];
		    //get fileName to send from user
			printf("Enter the file Name: \n");
			fflush(stdout);
			scanf("%s", fileName);

            //open the file for reading
			fp = fopen(fileName, "r");
			if(fp==NULL){
				perror("Error");
				return 1;
			}

			//get the file size
			struct stat sfile;
			stat(fileName, &sfile);
			long int numOfBytes = sfile.st_size;
			printf("st_size = %ld", numOfBytes);

            //parts of command to construct
			char str[] = "PUSH ";
			char str1[] = " fileSize <<<";
			char str2[] = ">>>\0";
			char numInStr[25];
			sprintf(numInStr, "%ld", numOfBytes);

			char command[57] = "";
			//building the command
			strcat(command, str);
			strcat(command, fileName);
			strcat(command, str1);
			strcat(command, numInStr);
			strcat(command, str2);

			//printing the command and sending it
			printf("Command sent: %s \n", command);
			send(sockfd , command , strlen(command) , 0 );

            //reading the data in the file and storing it
			char buffer[numOfBytes];
			int count = fread(&buffer, sizeof(char), numOfBytes, fp);
			fclose(fp);

			//reading the OK response from the server
			char buff[2];
			read( sockfd , buff, strlen(buff)); //read message from server
			printf("Command sent: %s \n", command);
			printf("SERVER RESPONSE: %s\n",buff ); //print the read message from the server


			// Printing data to check validity
			printf("Data read from file and Sent to server: \n %s \n", buffer);
			printf("Bytes read: %d \n", count);

			send(sockfd , buffer , strlen(buffer) , 0 );

		}else if(strcmp(input,"PULL") == 0){

			//the user wants to send a pull command
			char fileName[20];
			//get the fileName from user
			printf("Enter the file Name: \n");
			fflush(stdout);
			scanf("%s", fileName);

			//building the command and sending it
			char command[] = "PULL ";
			strcat(command,fileName);
			printf("COMMAND SENT: %s\n", command);
			send(sockfd , command, strlen(command) , 0 );

			char buff2[20];
			//read message from server should hold the file size
			read( sockfd , buff2, strlen(buff2)); //read message from server
			printf("SERVER SENDING THE SIZE: %s\n",buff2 );

			int size = atoi(buff2);

			char buff3[size];
			read( sockfd , buff3, size); //read message from server
			printf("THIS SHOULD BE THE DATA IN THE FILE:\n  %s \n",buff3 );

			char buff4[2];
			read( sockfd , buff4, strlen(buff4)); //read message from server
			printf("THIS SHOULD BE OK:\n  %s \n",buff4 );

			//clear the buffers
			memset(buff2, 0, strlen(buff2));
			memset(buff3, 0, strlen(buff3));
			memset(buff4, 0, strlen(buff4));

		}else if(strcmp(input,"QUIT") == 0){

			close(sockfd);
			break;
		}
	}
	printf("Successfully exited \n");

	return 0;
}
