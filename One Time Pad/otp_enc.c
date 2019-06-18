/**********************************************************************************
* Author: Amy Stockinger
* Date: 5/26/2019
* Program: otp_enc.c
* Description: This program connects to otp_enc_d, and asks it to perform a one-time 
* pad style encryption
**********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#define BUFFER_SIZE 80000

void error(const char *msg, int exitValue){ 
	perror(msg); exit(exitValue); 								// Error function used for reporting issues
}

int main(int argc, char *argv[]){
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	int plaintextSize, keySize;
	int i, yes = 1;
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', sizeof(buffer));

	if (argc != 4){
		fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); 	// check args
	}

	/* Check file arguments */
	// open files, get size information and make char arrays for each
	// https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
	FILE* ptextf = fopen(argv[1], "r");				// get file arg and open plaintext
	if(ptextf < 0){									// check that file can be opened
		error("otp_enc error: opening plaintext file", 1);
	}
	fgets(buffer, sizeof(buffer), ptextf);			// copy file contents to buffer
	fclose(ptextf);									// close file
	buffer[strcspn(buffer, "\n")] = 0;				// get only the chars until the newline
	plaintextSize = strlen(buffer) + 1;				// calculate size of our plaintext
	char plaintext[plaintextSize];
	strncpy(plaintext, buffer, plaintextSize);		// copy buffer into plaintext array
	plaintext[plaintextSize - 1] = '\0';			// null terminate plaintext
	memset(buffer, '\0', sizeof(buffer));			// reset buffer to get key

	FILE* keyf = fopen(argv[2], "r");				// get file arg and open key
	if(keyf < 0){									// check that file can be opened
		error("otp_enc error: opening key file", 1);
	}
	fgets(buffer, sizeof(buffer), keyf);			// copy file contents to buffer
	fclose(keyf);
	buffer[strcspn(buffer, "\n")] = 0;				// get chars only until newline
	keySize = strlen(buffer) + 1;					// calculate 
	char key[plaintextSize];
	strncpy(key, buffer, plaintextSize);
	key[plaintextSize - 1] = '\0';
	memset(buffer, '\0', sizeof(buffer));

	if(keySize < plaintextSize || keySize == 0){				 // key must be large enough to account for plain text
		fprintf(stderr, "otp_enc error: key '%s' is too short\n", argv[2]);
		exit(1);
	}

	for(i = 0; i < strlen(plaintext); i++){						// loop through plaintext to check that only valid chars are used
		if(plaintext[i] > 'Z' || (plaintext[i] < 'A' && plaintext[i] != ' ')){
			fprintf(stderr, "otp_enc error: '%s' contains invalid characters\n", argv[1]);
			exit(1);
		}
	}


	/* Set up Server*/
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); 								// Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; 						// Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); 				// Store the port number
	serverHostInfo = gethostbyname("localhost"); 				// Convert the machine name into a special form of address
	if (serverHostInfo == NULL){ 
		fprintf(stderr, "otp_enc error: no such host\n"); 
		exit(2); 
	}
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); 				// Create the socket
	if (socketFD < 0){
		error("otp_enc error: opening socket", 2);
	}
	setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); 

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to address
		error("otp_enc error: connecting", 2);
	}

	// verify proper connection
	memset(buffer, '\0', sizeof(buffer)); 						// Clear out the buffer array
	charsWritten = send(socketFD, "encrypt", 8, 0);				// send 'encrypt'
	if(charsWritten < 0){
		perror("otp_enc error: writing to socket: ");
	}
	charsRead = recv(socketFD, buffer, sizeof(buffer), 0);  	// Read data from the socket, leaving \0 at end
	if (charsRead < 0){
		perror("otp_enc error: reading first response from socket: ");
	}
	if(strcmp(buffer, "encrypt") != 0){							// expect to receive back 'encrypt'
		fprintf(stderr, "otp_enc error: invalid encryption connection\n");
		exit(2);
	}


	/* Communicate with Server */
	// send plaintext size
	charsWritten = send(socketFD, &plaintextSize, sizeof(plaintextSize), 0); // Write text size to the server
	if (charsWritten < 0){
		perror("otp_enc error: writing to socket: ");
	}

	// Send plaintext and key server for encryption
	charsWritten = send(socketFD, plaintext, sizeof(plaintext), 0); 	// Write text to the server
	if (charsWritten < 0){
		perror("otp_enc error: writing to socket: ");
	}
	if (charsWritten < strlen(plaintext)){
		fprintf(stderr, "otp_enc WARNING: Not all data written to socket!\n");
	}

	charsWritten = send(socketFD, key, sizeof(plaintext), 0); 			// Write key to the server
	if (charsWritten < 0){
		perror("otp_enc error: writing to socket: ");
	}
	if (charsWritten < strlen(key)){
		fprintf(stderr, "otp_enc WARNING: Not all data written to socket!\n");
	}

	// Get return message from server
	char cipher[plaintextSize];											// expecting to get back the same amount of chars sent
	memset(cipher, '\0', sizeof(cipher)); 							 
	charsRead = recv(socketFD, cipher, sizeof(cipher), MSG_WAITALL); 	// get back cipher text
	if (charsRead < 0){
		perror("otp_enc error: reading cipher from socket: ");
	}


	/* Print final text and we're done */
	fprintf(stdout, "%s\n", cipher);									// print cipher
	close(socketFD); 													// Close the socket
	return 0;
}