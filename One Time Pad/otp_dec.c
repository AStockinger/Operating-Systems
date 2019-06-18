/**********************************************************************************
* Author: Amy Stockinger
* Date: 5/26/2019
* Program: otp_dec.c
* Description: This program connects to otp_dec_d and will ask it to decrypt 
* ciphertext using a passed-in ciphertext and key
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
	int cipherSize, keySize;
	int i, yes = 1;
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', sizeof(buffer));

	if (argc != 4){
		fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); 	// check args
	}

	/* Check file arguments */
	// open files, get size information and make char arrays for each
	// NOTE: see otp_enc.c for more detailed comments in this section
	FILE* cipherf = fopen(argv[1], "r");
	if(cipherf < 0){
		error("otp_dec error: opening cipher file", 1);
	}
	fgets(buffer, sizeof(buffer), cipherf);
	fclose(cipherf);
	buffer[strcspn(buffer, "\n")] = 0;
	cipherSize = strlen(buffer) + 1;
	char cipher[cipherSize];
	strncpy(cipher, buffer, cipherSize);
	cipher[cipherSize - 1] = '\0';
	memset(buffer, '\0', sizeof(buffer));

	FILE* keyf = fopen(argv[2], "r");
	if(keyf < 0){
		error("otp_dec error: opening key file", 1);
	}
	fgets(buffer, sizeof(buffer), keyf);
	fclose(keyf);
	buffer[strcspn(buffer, "\n")] = 0;
	keySize = strlen(buffer) + 1;
	char key[cipherSize];
	strncpy(key, buffer, cipherSize);
	key[cipherSize - 1] = '\0';
	memset(buffer, '\0', sizeof(buffer));

	if(keySize < cipherSize || keySize == 0){					// verify key size is long enough to cover the cipher
		fprintf(stderr, "otp_dec error: key '%s' is too short\n", argv[2]);
		exit(1);	
	}

	for(i = 0; i < strlen(cipher); i++){						// make sure all chars are valid
		if(cipher[i] > 'Z' || (cipher[i] < 'A' && cipher[i] != ' ')){
			fprintf(stderr, "otp_dec error: '%s' contains invalid characters\n", argv[1]);
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
		fprintf(stderr, "otp_dec error: no such host\n"); 
		exit(2); 
	}
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); 				// Create the socket
	if (socketFD < 0){
		error("otp_dec error: opening socket", 2);
	}
	setsockopt(socketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)); 

	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to address
		error("otp_dec error: connecting", 2);
	}

	// verify proper connection
	memset(buffer, '\0', sizeof(buffer)); 						// Clear out the buffer array
	charsWritten = send(socketFD, "decrypt", 8, 0);
	if(charsWritten < 0){
		perror("otp_dec error: writing to socket: ");
	}
	charsRead = recv(socketFD, buffer, sizeof(buffer), 0);  	// Read data from the socket, leaving \0 at end
	if (charsRead < 0){
		perror("otp_dec error: reading first response from socket: ");
	}
	if(strcmp(buffer, "decrypt") != 0){
		fprintf(stderr, "otp_dec error: invalid decryption connection\n");
		exit(2);
	}


	/* Communicate with Server */
	// send cipher size -- no need to send key size too since we already made sure it's large enough
	charsWritten = send(socketFD, &cipherSize, sizeof(cipherSize), 0); 	// Write text to the server
	if (charsWritten < 0){
		perror("otp_dec error: writing to socket: ");
	}

	// Send cipher and key to server for encryption
	charsWritten = send(socketFD, cipher, sizeof(cipher), 0); 	// Write cipher to the server
	if (charsWritten < 0){
		perror("otp_dec error: writing to socket: ");
	}
	if (charsWritten < strlen(cipher)){
		fprintf(stderr, "otp_dec WARNING: Not all data written to socket!\n");
	}

	charsWritten = send(socketFD, key, sizeof(cipher), 0); 		// Write key to the server
	if (charsWritten < 0){
		perror("otp_dec error: writing to socket: ");
	}
	if (charsWritten < strlen(key)){
		fprintf(stderr, "otp_dec WARNING: Not all data written to socket!\n");
	}

	// Get return message from server
	char plaintext[cipherSize];											   // expecting to get back the same amount of chars sent
	memset(plaintext, '\0', sizeof(plaintext));
	charsRead = recv(socketFD, plaintext, sizeof(plaintext), MSG_WAITALL); // read back plaintext message
	if (charsRead < 0){
		perror("otp_dec error: reading plaintext from socket: ");
	}


	/* Print final text and we're done */
	fprintf(stdout, "%s\n", plaintext);									// print plaintext
	close(socketFD); 													// Close the socket
	return 0;
}