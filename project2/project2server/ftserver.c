/* Amanda Lawrence
* November 27, 2019
* CS 372 - Intro to Computer Networks
* Project 2
* ftserver.c
*
* Implement 2-connection client-server network application
* Practice using the sockets API
* 
* 
* Design and implement a simple file transfer system, i.e., create a file transfer server and a file transfer client
* Write the ftserver and ftclient programs.
* The final version of your programs must accomplish the following tasks:
*  1. ftserver starts on Host A and validates command-line parameters (<SERVER_PORT>)
*  2. ftserver waits on <PORTNUM> for a client request
*  3. ftclient starts on Host B, and validates any pertinent command-line parameters.
*  4. ftserver and ftclient establish a TCP control connection on <SERVER_PORT>. 
*      (For the remainder of this description, call this connection P)
*  5. ftserver waits on connection P for ftclient to send a command.
*  6. ftclient sends a command (-l (list) of -g <FILENAME> (get)) on connection P.
*  7. ftserver receives command on connection P.
*      If ftclient sent an invalid command:
*	       - ftserver sends an error message to ftclient on connection P, and ftclient displays the message on-screen. 
*      Otherwise:
*		   - ftserver initiates a TCP data connection with ftclient on <DATA_PORT> (called connection Q)
*		   - If ftclient has sent the -l command, ftserver sends its directory to ftclient on connection Q,
*				and ftclient displays the directory on-screen.
*		   - If ftclient has sent -g <FILENAME>, ftserver validates FILENAME, and either:
*					- sends the contents of FILENAME on connection Q. ftclient saves the file in the current
*						default directory (handling "duplicate file name" error if necessary), and displays a
*						"transfer complete" message on-screen.
*					- Or, sends an appropriate error message ("File not found", etc.) to ftclient on connection
*						P, and ftclient displays the message on-screen.
*		   - ftserver closes connection Q (don't leave open sockets!)
* 8. ftclient closes connection P (don't leave open sockets!) and terminates.
* 9. ftserver repeats from 2 (above) until terminated by a supervisor (SIGINT).
*/


/*
*	This program is based on Beej's Guide to Programming and Project 1's chatclient.py.
*/



#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <dirent.h>
//#include <signal.h>
//#include <fcntl.h>



#define BUFFER_SIZE 500
#define FILENAME_LENGTH 500

int newFD;


//void handle_sigint(int sig){
//	printf("Closing down server.\n");
//	fflush(stdout);
//	close(newFD);
//	kill(getppid(), SIGINT);
//}

static void exit_handler(void){
	//printf("Closing down server.\n");
	//fflush(stdout);
	close(newFD);
	kill(getppid(), SIGINT);
}


// From Beej's Guide to Network Programming
// creates address info for server
struct addrinfo* createServerAddress(char* port){	
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo; 			// will point to the results
	memset(&hints, 0, sizeof hints); 	// make sure the struct is empty
	hints.ai_family = AF_UNSPEC; 		// don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; 	// TCP stream sockets
	hints.ai_flags = AI_PASSIVE; 		// fill in my IP for me
	if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s. Please enter correct port.\n", gai_strerror(status));
		exit(1);
	}
	return servinfo;
}


// From Beej's Guide to Network Programming
// creates address info for client, including destination IP and port.
struct addrinfo* createClientAddress(char *ipAddress, char *port){	
	int status;
	struct addrinfo hints;
	struct addrinfo *res, *p; 			// will point to the results
	memset(&hints, 0, sizeof hints); 	// make sure the struct is empty
	hints.ai_family = AF_UNSPEC; 		// don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM; 	// TCP stream sockets
	hints.ai_flags = AI_PASSIVE; 		// fill in my IP for me
	if ((status = getaddrinfo(ipAddress, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s. Please enter correct port.\n", gai_strerror(status));
		exit(1);
	}

	char ipstr[INET6_ADDRSTRLEN];
	//printf("IP addresses for %s:\n\n", ipAddress);
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
		//inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		//printf(" %s: %s\n", ipver, ipstr);
	}


	return res;
}


// From Beej's Guide to Network Programming
// Creates the socket
int createSocket(struct addrinfo* res){
	int socketFD;

	if ((socketFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
		fprintf(stderr, "Error: creating socket. Exiting.\n");
		exit(1);
	}
	return socketFD;
}




// From Beej's Guide to Network Programming
// For connecting socket to a remote host. 
void connectSocket(int socketFD, struct addrinfo *res){
	int status;
	if ((status = connect(socketFD, res->ai_addr, res->ai_addrlen)) == -1){
		fprintf(stderr, "Error: connecting socket. Exiting.\n");
		exit(1);
	}
}




// From Beej's Guide to Network Programming
// For binding socket to a remote host. 
void bindSocket(int socketFD, struct addrinfo* res){
	int status;
	if ((status = bind(socketFD, res->ai_addr, res->ai_addrlen)) == -1){
		fprintf(stderr, "Error: binding socket. Exiting.\n");
		close(socketFD);
		exit(1);
	}
}




// From Beej's Guide to Network Programming
// Once the socket has binded, we need to listen for incoming data. Allows up to 10 connetions
void listenSocket(int socketFD){
	int status;
	if ((status = listen(socketFD, 10)) == -1){
		fprintf(stderr, "Error: listening on socket. Exiting.\n");
		close(socketFD);
		exit(1);
	}
}



// Gets a list of the files in the same directory as the ftserver.c file, and sends a list of text files back to client, per the Example Execution.
int listDirectoryHandler(char *buffer_port, char *buffer_ipaddress, int newFD, char *msg){
	// The following code block for searching through the directory is adapted from my CS 344 - Project 2: lawreama.adventure.c file
	//char** buffer_filenames = malloc(FILENAME_LENGTH*sizeof(char));	// buffer for storing each filename in the directory
	char **buffer_filenames = NULL; //malloc(FILENAME_LENGTH*sizeof(char));	// buffer for storing each filename in the directory
	DIR *dirToCheck; 			/* Holds the directory we're starting in */
	struct dirent *fileInDir; 	/* Holds the current subdir of the starting dir */
	char targetFileType[32] = ".txt"; /* We're only looking for the files ending in .txt */

	dirToCheck = opendir("."); 											/* Open up the directory this program was run in */

	int j = 0;
	int numAnyFiles = 0;
	//int numTextFiles = 0;

	// Checks the directory, and finds the total amount of files that are contained in directory.
	if (dirToCheck > 0){
		while ((fileInDir = readdir(dirToCheck)) != NULL) {				/* Check each entry in dir */
  			numAnyFiles++;
  		}
  		//printf("Number of files in this directory is: %d\n", numAnyFiles);
  		//fflush(stdout);
  	}
  	rewinddir(dirToCheck);

  	// Allocate pointers for individual filenames:
  	buffer_filenames = malloc(numAnyFiles * sizeof(*buffer_filenames));
  	if ( buffer_filenames == NULL ) {
  		fprintf(stderr, "Failed to allocate buffer_filenames!\n");
  		exit(EXIT_FAILURE);
  	}

  	int i = 0;
	for(i=0; i<numAnyFiles; i++){
		buffer_filenames[i] = malloc(FILENAME_LENGTH * sizeof(*(buffer_filenames[i])));
		memset(buffer_filenames[i], '\0', FILENAME_LENGTH);
	}

	if (dirToCheck > 0){ 												/* Make sure the current directory could be opened */
		//printf("Checking the contents of this directory...\n");
		//fflush(stdout);
		while ((fileInDir = readdir(dirToCheck)) != NULL) {				/* Check each entry in dir */
  			//printf("what is this filename? %s\n", fileInDir->d_name);
  			//fflush(stdout);
  			if (strstr(fileInDir->d_name, targetFileType)) {		/* If file has file type of .txt */
      			strcpy(buffer_filenames[j], fileInDir->d_name);			/* copy the file name into of buffer_filename */
      			//printf("Current filename is:  %s\n", buffer_filenames[j]);
      			//fflush(stdout);
      			//numTextFiles++;
      			j++;
  			}
		}
	}
	closedir(dirToCheck); 	/* Close the directory we opened */

	printf("List directory requested on port %s\n", buffer_port);
	fflush(stdout);
	send(newFD, msg, strlen(msg), 0);
	printf("Sending directory contents to %s:%s\n", buffer_ipaddress, buffer_port);
	fflush(stdout);
	//printf("Delay 3 seconds...");
	//fflush(stdout);
	sleep(2); //used to fight race conditions
	//printf("Finished \n");
	//fflush(stdout);

	// Send the directory we just searched thru to the client
	struct addrinfo *res = createClientAddress(buffer_ipaddress, buffer_port);
	//printf("About to create a socket, called fileListSocket\n");
	//fflush(stdout);
	int fileListSocket = createSocket(res);
	//printf("the fileListSocket int is: %d\n", fileListSocket);
	//fflush(stdout);

	connectSocket(fileListSocket, res);
	int k;
	//char buffer_dummy[BUFFER_SIZE] = { '\0' };
	// Sends the filenames, one at a time, followed by a newline character. This will only send text files, per the Example Execution's output.
	for(k = 0; k < j; k++){
		//printf("Sending item %d of %d: %s \n", k+1, j, buffer_filenames[k]);
		//fflush(stdout);
		send(fileListSocket, buffer_filenames[k], strlen(buffer_filenames[k]), 0);
		send(fileListSocket, "\n", 1, 0);
		//printf("Send complete, waiting for reply...\n");
		//fflush(stdout);
		//recv(newFD, buffer_dummy, sizeof(buffer_dummy)-1, 0);
	}
	//printf("Sending 'complete' ....\n");
	//fflush(stdout);
	send(fileListSocket, "complete", strlen("complete"), 0); // Used to denote the end of the list of files to send to client.
	close(fileListSocket);
	freeaddrinfo(res);
	//printf("DONE\n");
	//fflush(stdout);

	// Clean up allocated memory:
	for(i=0; i<numAnyFiles; i++){
		if ( buffer_filenames[i] ) { 
			free(buffer_filenames[i]);
			buffer_filenames[i] = NULL;
		}
	}
	if ( buffer_filenames ) {
		free(buffer_filenames);
		buffer_filenames = NULL;
	}

	return 0;
}


// Opens the file requested by user on client, and writes its data to buffer, then sends that buffer to client for the client to then write the data to a file. 
#define BUFFSIZE 2046
int fileOpener(char *buffer_port, char *buffer_ipaddress, char *filename){
	//fgets
	FILE *fp;
	fp = fopen(filename, "r");
	struct addrinfo *res = createClientAddress(buffer_ipaddress, buffer_port);
	int fileDataSocket = createSocket(res);
	connectSocket(fileDataSocket, res);

	if (fp == NULL){
		fprintf(stderr, "Failed to open %s\n", filename);
		return 1;
	}

	char buffer[BUFFSIZE+2] = { '\0' };

	while ( fgets(buffer, BUFFSIZE, fp) != NULL ) {
		//printf("%s", buffer);
		//fflush(stdout);
		send(fileDataSocket, buffer, strlen(buffer), 0);
	}
	close(fileDataSocket);
	freeaddrinfo(res);
	fclose(fp);
	return 0;
}




int fileTransferHandler(char *buffer_port, char *buffer_ipaddress, int newFD, char *msg, char *filename){
	printf("File %s requested on port %s.\n", filename, buffer_port);
	fflush(stdout);

	// Code block structure from Stack Overflow: https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
	if (access(filename, F_OK)!= -1){
		// the file exists on server.
		printf("Sending '%s' to %s:%s\n", filename, buffer_ipaddress, buffer_port);
		fflush(stdout);
		//printf("msg: %s\n", msg);
		//fflush(stdout);
		send(newFD, msg, strlen(msg), 0);
		fileOpener(buffer_port, buffer_ipaddress, filename);

	} else {
		// the file does not exist on server.
		printf("File not found. Sending error message to %s: %s\n", buffer_ipaddress, buffer_port);
		fflush(stdout);
		send(newFD, "FILE NOT FOUND", strlen("FILE NOT FOUND"), 0);
		return 1;
	}

	return 0;
}





int badCommandHandler(char *buffer_port, char *buffer_ipaddress, int newFD, char *msg){

	return 0;
}





// From Beej's Guide to Network Programming
// This will accept new connections from the client. 
// It will handle their request for: a list of directory contents, or, a single file based 
// 		on whether command -l or -g is present.
void requestHandler(int newFD){
	char *msg = "ok";
	char buffer_port[BUFFER_SIZE];		// buffer for port number, received from user on client.
	char buffer_command[BUFFER_SIZE];	// buffer for command, received from user on client.
	char buffer_ipaddress[BUFFER_SIZE];	// buffer for IP address, received from user on client.
	char buffer_filename[BUFFER_SIZE];	// buffer for filename, received from user on client.
	char buffer_racecond[BUFFER_SIZE];
	//char** buffer_filenames = malloc(FILENAME_LENGTH*sizeof(char));	// buffer for storing each filename in the directory
	int i;



	memset(buffer_command, '\0', sizeof(buffer_command));
	recv(newFD, buffer_command, sizeof(buffer_command)-1, 0);
	//fprintf(stdout, "Received buffer_command of: '%s'\n", buffer_command);
	//fflush(stdout);
	send(newFD, msg, strlen(msg), 0);

	memset(buffer_port, '\0', sizeof(buffer_port));
	recv(newFD, buffer_port, sizeof(buffer_port)-1, 0);
	//fprintf(stdout, "Received buffer_port of: '%s'\n", buffer_port);
	//fflush(stdout);
	send(newFD, msg, strlen(msg), 0);


	memset(buffer_ipaddress, '\0', sizeof(buffer_ipaddress));
	recv(newFD, buffer_ipaddress, sizeof(buffer_ipaddress)-1, 0);
	//fprintf(stdout, "Received buffer_ipaddress of: '%s'\n", buffer_ipaddress);
	//fflush(stdout);
	send(newFD, msg, strlen(msg), 0);
	

	if (strncmp(buffer_command, "-g", 2) == 0){
		memset(buffer_filename, '\0', sizeof(buffer_filename));
		recv(newFD, buffer_filename, sizeof(buffer_filename)-1, 0);
		//fprintf(stdout, "Received buffer_filename of: '%s'\n", buffer_filename);
		//fflush(stdout);
		send(newFD, msg, strlen(msg), 0);
	}


	fprintf(stdout, "Connection from '%s'\n", buffer_ipaddress);
	fflush(stdout);

	// used in order to avoid race conditions
	recv(newFD, buffer_racecond, sizeof(buffer_racecond)-1, 0);
	sleep(3);
	//fprintf(stdout, "Proceed received: '%s'.\n", buffer_racecond);
	//fflush(stdout);


	if(strcmp(buffer_command, "-l") == 0){			// "l" == retrieve list of directory contents and send to client. 
		listDirectoryHandler(buffer_port, buffer_ipaddress, newFD, msg);

	} else if (strncmp(buffer_command, "-g", 2) == 0){	// "g" == retrieve a file and send to client.
		//printf("File %s requested on port %s\n", buffer_filename, buffer_port);
		fileTransferHandler(buffer_port, buffer_ipaddress, newFD, msg, buffer_filename);

	} else {	// Else, we have received a bad command.
		send(newFD, "Error: Invalid command received by server.", strlen("Error: Invalid command received by server."), 0); // Error message to send client
		fprintf(stdout, "Error: Invalid command received by server.\n");
		fflush(stdout);
	}
}





void handshake(int socketFD, char *clientname, char *servername){
	// send the client username to server, and receive the servername from the server.
	int send_username = send(socketFD, clientname, strlen(clientname), 0);
	int rec_servername = recv(socketFD, servername, 10, 0);
	//printf("Servername is:  %s\n", servername);
	//fflush(stdout);
}








int main (int argc, char *argv[]){

	//int socketFD, newFD, portNumber, charsWritten, charsRead;
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	//struct hostent* serverHostInfo = gethostbyname("localhost");

	if (argc != 2){
		printf("Please include a server port number. \n");
		fflush(stdout);
		exit(0);
	} else {
		portNumber = atoi(argv[1]);
		printf("Server open on %d\n", portNumber);
		fflush(stdout);
	}


	struct addrinfo* res = createServerAddress(argv[1]);
	socketFD = createSocket(res);

	if (setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0){
    	perror("setsockopt(SO_REUSEADDR) failed");
    	exit(1);
    }

	bindSocket(socketFD, res);
	listenSocket(socketFD);
	atexit(exit_handler);

	// From Beej's Guide to Network Programming
	// For waiting for connections on our socket
	while(1){
		//signal(SIGINT, handle_sigint);
		
		addr_size = sizeof(their_addr);
		newFD = accept(socketFD, (struct sockaddr *)&their_addr, &addr_size);
		if (newFD == -1){
			perror("accept");
			continue;
		}


		// ACCEPT NEW CONNECTIONS HERE
		//printf("Starting request handler\n");
		//fflush(stdout);
		requestHandler(newFD);
		//close(newFD);
		//printf("Ending request handler\n");
		//fflush(stdout);
	}


	freeaddrinfo(res);

}
