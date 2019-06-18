/**********************************************************************************
* Author: Amy Stockinger
* Date: 5/26/2019
* Program: otp_enc_d.c
* Description: encrypting daemon that communicates with otp_enc.c
**********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 80000

void error(const char *msg, int exitValue) { perror(msg); exit(exitValue); } // Error function used for reporting issues

void encrypt(char* plaintext, char* key, char* cipher){
    char charArray[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";     // 27 allowed chars (0-26)
	int textSize = strlen(plaintext);
	int i, t, k, d;

	for(i = 0; i < textSize; i++){
		// convert cipher chars to ints that correspond to charArray
		if(plaintext[i] == ' '){
			t = 26;
		}
		else{
			t = plaintext[i] - 65;
		}
		// convert key chars to ints that correspond to charArray
		if(key[i] == ' '){
			k = 26;
		}
		else{
			k = key[i] - 65;
		}

		// encryption formula
		d = (t + k) % 27;

		// fill in as plaintext
		cipher[i] = charArray[d];
	}
}

int main(int argc, char *argv[]){
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[BUFFER_SIZE];
	struct sockaddr_in serverAddress, clientAddress;
    int status, yes = 1;
	int plaintextSize;
	pid_t pid;

    // Check usage & args
	if (argc < 2){                 
        fprintf(stderr,"USAGE: %s port\n", argv[0]); 
        exit(1); 
    }

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); 								 // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; 					     // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); 				 // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; 				 // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); 			 // Create the socket
	if (listenSocketFD < 0){
		error("otp_enc_d error: opening socket", 1);
	}
	// allow reuse
    setsockopt(listenSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){ // Connect socket to port
		error("otp_enc_d error: on binding", 1);
	}
	listen(listenSocketFD, 5); 									// Flip the socket on - it can now receive up to 5 connections

	while(1){
		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); 				// Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		if (establishedConnectionFD < 0){
			error("otp_enc_d error: on accept", 1);
		}

		pid = fork();
			if(pid == -1){ 																// fork failed
				perror("otp_enc_d: Hull Breach! ERROR on fork");
				close(establishedConnectionFD);
				exit(1);
			}
			else if(pid == 0){
				memset(buffer, '\0', BUFFER_SIZE);
				charsRead = recv(establishedConnectionFD, buffer, BUFFER_SIZE - 1, 0); 	// Read the client's message from the socket
				if (charsRead < 0){
					perror("otp_enc_d error: reading initial message from socket: ");
				}
				else if(strcmp(buffer, "encrypt") != 0){
					send(establishedConnectionFD, "invalid", 8, 0);						// if not encrypting, then not correct connection
					fprintf(stderr, "otp_enc_d error: invalid encryption socket\n");
					exit(2);															// exit 2 if not correct connection
				}

				// Send a Success message back to the client
				charsRead = send(establishedConnectionFD, "encrypt", 8, 0);             // verify proper connection for decryption by sending a message back
				if (charsRead < 0){
					perror("otp_enc_d error: writing to socket: ");
				}
				memset(buffer, '\0', BUFFER_SIZE);										// reset buffer

                charsRead = recv(establishedConnectionFD, &plaintextSize, sizeof(plaintextSize), 0); // get plaintext size
                if(charsRead < 0){
					perror("otp_enc_d error: reading from socket: ");
                }

				// make arrays to hold characters of plaintext and key
				char plaintext[plaintextSize];
				memset(plaintext, '\0', sizeof(plaintext));
				char key[plaintextSize];															// we only need plaintext size of the key
				memset(key, '\0', sizeof(key));														// which otp_enc.c is set up to send that way

				charsRead = recv(establishedConnectionFD, plaintext, sizeof(plaintext), MSG_WAITALL); // get plaintext
                if(charsRead < 0){
					perror("otp_enc_d error: reading from socket: ");
                }

				charsRead = recv(establishedConnectionFD, key, sizeof(key), MSG_WAITALL); 	 // get key
                if(charsRead < 0){
					perror("otp_enc_d error: reading from socket: ");
                }

				char cipher[plaintextSize];													// cipher to be same size as plaintext
				memset(cipher, '\0', sizeof(cipher));
				encrypt(plaintext, key, cipher);											// encrypt plaintext
				charsRead = send(establishedConnectionFD, cipher, sizeof(cipher), 0);		// send back ciphertext
				if (charsRead < 0){						
					perror("otp_enc_d error: writing to socket: ");
				}
				close(establishedConnectionFD); // Close the existing socket which is connected to the client
				exit(0);
			}
			else{								// default
				close(establishedConnectionFD); // Close the existing socket which is connected to the client
                establishedConnectionFD = -1;
                wait(NULL);
			}
		}
	close(listenSocketFD); 						// Close the listening socket
	return 0; 
}