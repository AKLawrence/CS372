/* Amanda Lawrence
* Last Modified: November 3, 2019 - 5:30 PM HST
* CS 372 - Intro to Computer Networks
* Project 1
* chatclient.c
*
* Implement a client-server network application
* Learn to use the sockets API
* Use the TCP protocol
* 
* chatserve starts on host A, and waits on a port (specified at the command-line) for a client request.
* chatclient starts on host B, specifying host A's hostname and port number on the command line. 
* chatclient on host B gets the user's "handle" by initial query (a one-word name, up to 10 characters). 
* the chatclient will display this handle as a prompt on host B and will prepend it to all messages sent to host A. ("SteveO> Hi")
* chatclient on host B sends an initial message to chatserve on host A: PORTNUM. This causes a connection
* to be established between host A and host B. Host A and host B are now peers, and may alternate sending and receiving messages. 
* Responses from host A should have host A's "handle" prepended.
* Host A responds to host B, or closes the connection with the command "\quit"
* Host B responds to host A, or closes the connection with command "\quit"
* If the connection is not closed at this point, host A and host B should still be able to respond to the other.
* However, if the connection is closed, chatserve repeats from 2 (until a SIGINT is received)
*
* Programs must be able to send messages up to 500 characters
* Don't hard-code any directories
*
* This program was written using Beej's Guide to Network Programming, and my own code from this same assignment when I
* attempted this class in Spring 2019. 
*/




#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
//#include <signal.h>
//#include <fcntl.h>



#define BUFFER_SIZE 502
#define NAME_LENGTH  11


void handshake(int socketFD, char *clientname, char *servername){
	// send the client username to server, and receive the servername from the server.
	int send_username = send(socketFD, clientname, strlen(clientname), 0);
	int rec_servername = recv(socketFD, servername, 10, 0);
	//printf("Servername is:  %s\n", servername);
	//fflush(stdout);
}




int main (int argc, char *argv[]){

	char clientname[NAME_LENGTH] = { '\0' };
	char servername[NAME_LENGTH] = { '\0' };
	int socketFD, portNumber_int, charsWritten, charsRead;
	char portNumber[10] = { '\0' };
	struct sockaddr_in serverAddress;
	//struct hostent* serverHostInfo = gethostbyname("localhost");
	char buffer_input[BUFFER_SIZE];	// buffer for data received from user.
	char buffer_rec[BUFFER_SIZE];	// buffer for data received from server.
	char buffer_output[BUFFER_SIZE+10];	// buffer for the client's name + message to send to server


	// Setting up variables for getting the server hostname's IP address, for when this client is run on a different machine than 
	// the server. This section was written based on Beej's Guide to Network Programming.
	int status;
	struct addrinfo hints, *res, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; 			// Use IPv4
	hints.ai_socktype = SOCK_STREAM;	// use TCP
	char ipstr[INET6_ADDRSTRLEN];


	if (argc != 3){
		printf("Please include a server host name and port number. \n");
		fflush(stdout);
		exit(0);
	}


	// Set up the server address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	memset((char*)&portNumber, '\0', sizeof(portNumber));
	strcpy(portNumber, argv[2]); // Get the client port number
	portNumber_int = atoi(portNumber);
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber_int); // Store the port number

	//portNumber = atoi(argv[2]); // Convert port number to an integer from a string
	if ((status = getaddrinfo(argv[1], portNumber, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return 2;
	}

	printf("IP addresses for %s:\n\n", argv[1]);
	for (p = res;p != NULL; p = p->ai_next) {
		void *addr;
		char *ipver;
		// get the pointer to the address itself,
		// different fields in IPv4 and IPv6:
		if (p->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		// convert the IP to a string and print it:
		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		printf(" %s: %s\n", ipver, ipstr);
	}





	// Get the client user's username
	printf("What is your username? ");
	fgets(buffer_input, 501, stdin);
	snprintf(clientname, NAME_LENGTH, "%s", buffer_input);
	clientname[strcspn(clientname, "\r\n")] = 0;


	//memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)ipstr, strlen(ipstr)); // Copy in the address
	fflush(stdout);


	// Create the TCP socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); 
	if (socketFD < 0) {
		fprintf(stderr, "ERROR opening socket.\n");
		exit(2);
	}


	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) { // Connect socket to address
		fprintf(stderr, "ERROR connecting to server.\n");
		exit(2);
	}



	handshake(socketFD, clientname, servername);
	//fprintf(stderr, "Handshake complete. I think:\n");
	//fprintf(stderr, "--> clientname: '%s'\n", clientname);
	//fprintf(stderr, "--> servername: '%s'\n", servername);
	char msg[1024] = {'\0'};

    //fprintf(stderr, "\n\nStarting chat loop ...\n\n");
	while(1){
		// Get the user's message to send to the server.
		printf("%s> ", clientname);
		fflush(stdout);
		fgets(buffer_input, 501, stdin);

		//printf("Again... the servername is:  %s.\n", servername);

		// Check if the user entered \quit
		if(strcmp(buffer_input, "\\quit\n") == 0){
			printf("Closing connection.\n");
			fflush(stdout);
			// close(socketFD);
			break;
		} 


		// Otherwise, we'll send the message to the server.
		// Send text in message to server
		strtok(buffer_input, "\n");				// Remove trailing newline characters 
		sprintf(msg, "%s> %s", clientname, buffer_input);
		//fprintf(stderr, "msg: '%s\n'", msg);
		//charsWritten = send(socketFD, buffer_input, strlen(buffer_input), 0); // Write the text buffer to the server
		charsWritten = send(socketFD, msg, strlen(msg), 0); // Write the text buffer to the server
		if (charsWritten < 0) {
			fprintf(stderr, "ERROR writing text buffer to socket.\n");
			exit(2);
		}
		if (charsWritten < strlen(buffer_input)) {
			fprintf(stderr, "WARNING: Not all text buffer data written to socket.\n");
			exit(2);
		}



		// Get message from server
		//fprintf(stderr, "Getting from socket ...");
		memset(buffer_rec, '\0', sizeof(buffer_rec)); // Clear out the buffer again for reuse
		charsRead = recv(socketFD, buffer_rec, sizeof(buffer_rec) - 1, 0); // Read data from the socket, leaving \0 at end
		//fprintf(stderr, "done.\n");
		if (charsRead < 0) {
			fprintf(stderr, "ERROR reading from socket.\n");
			exit(2);
		} else if (charsRead == 0) {
			printf("Server closed connection.\n");
			break;
		} else {		// print out the message from the server
			if (strcmp("quit", buffer_rec) == 0){
				printf("Server has closed the connection.\n");
				exit(2);
			}
			printf("%s> %s\n", servername, buffer_rec);
			fflush(stdout);
		}

	}



	freeaddrinfo(res); // free the linked list
	return(0);

}


