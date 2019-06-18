/**********************************************************************************
* Author: Amy Stockinger
* Date: 5/26/2019
* Program: keygen.c
* This program creates a key file of specified length from the 27 allowed
* characters
**********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int main(int argc, char *argv[]){
    if(argc != 2){
        error("ERROR key length too short.");
        exit(0);
    }
    
    srand(time(0));                                         // random seed
    char charArray[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";     // 27 allowed chars
    const int length = atoi(argv[1]);
    char key[length + 1];                                   // char array to hold letters
    int i;
    for(i = 0; i < length; i++){
        key[i] = charArray[rand() % 27];                    // randomly assign letters to key
    }
    key[length] = '\0';
    printf("%s\n", key);                                    // last char printed is newline

    return 0;
}