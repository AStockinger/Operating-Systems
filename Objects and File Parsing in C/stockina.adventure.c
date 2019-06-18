/*************************************************************************
 * Name: Amy Stockinger
 * CS344 Program 2: adventure.c
 * Date: 4/23/2019
 * Description: program works based off of a directory created by buildrooms
 * to read files in that directory, recreate the array of room structs, and
 * play a game with the user to find the end room.
 * program also displays current time with 'time' command
 * 
 ***** NOTE: room names array in main() must match buildrooms.c file *****
 *************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#define MAX_ROOMS 10     // change array of names in main if this number changes
#define ROOM_COUNT 7     // the number of room files to be made
#define MAX_CON 6        // maximum connections a single room can have
#define MIN_CON 3        // minumum number of connections for a single room

pthread_t thread;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

enum roomType { START_ROOM, MID_ROOM, END_ROOM };

struct room {
    char* name;
    int connections;
    char* connectionNames[MAX_CON];
    enum roomType type;
};

// room array to be rebuilt from files
struct room rooms[ROOM_COUNT];

// get newest directory matching prefix
char* getDirectory(char* dirName){
    DIR* dir;                   // holds directory we're starting in
    struct dirent* fileInDir;   // holds current subdir of the starting dir
    struct stat dirAttributes;  // holds information we've gained about subdir
    int newestDirTime = -1;
    char targetDirPrefix[32] = "stockina.rooms.";                   // prefix to be searched
    dir = opendir(".");                                             // open current directory 
    if(dir > 0){
        while((fileInDir = readdir(dir)) != NULL){                  // read file names in directory
            if(strstr(fileInDir->d_name, targetDirPrefix) != NULL){ // check if names match the prefix
                stat(fileInDir->d_name, &dirAttributes);            // check file stats to see if it's newer than 
                if((int)dirAttributes.st_mtime > newestDirTime){    // previously chosen file
                    newestDirTime = (int)dirAttributes.st_mtime;    
                    memset(dirName, '\0', sizeof(dirName));
                    strcpy(dirName, fileInDir->d_name);
                }
            }
        }
    }
    closedir(dir);
    return dirName;
}

// reads information from files to put back into room struct array
void readRooms(char* names[]){
    char* dirName = malloc(sizeof(char) * 256);
    dirName = getDirectory(dirName);                               // go to directory with room files
    FILE* file;
    DIR* dir = opendir(dirName);
    chdir(dirName);
    free(dirName);                                                 // free malloc'd name
    struct dirent* fileInDir;
    char line[256];
    int roomCounter = 0;
    int i, j;
    char* word;

    // fill in array with info from files
    // check if each room name exists as a file
    while((fileInDir = readdir(dir)) != NULL){
        for(i = 0; i < MAX_ROOMS; i++){
            if(strstr(fileInDir->d_name, names[i]) != NULL){
                if(file = fopen(fileInDir->d_name, "r")){         // if file with a matching name exists, open it      
                    fgets(line, 256, file);                       // read lines from file to get information for room array
                    word = strtok(line, " ");
                    word = strtok(NULL, " ");
                    word = strtok(NULL, "\n");                    // tokenize exact word desired
                    for(j = 0; j < MAX_ROOMS; j++){               // compare to room names
                        if(strcmp(word, names[j]) == 0){
                            rooms[roomCounter].name = names[j];   // if valid, add connection based on name array
                            break;
                        }
                    }
                    while(fgets(line, 256, file) != NULL){
                        word = strtok(line, " ");
                        if(strcmp(word, "CONNECTION") == 0){      // look for 'connection' so we know what the line is
                            word = strtok(NULL, " ");             // tokenize with two spaces and a newline           
                            word = strtok(NULL, "\n");
                            for(j = 0; j < MAX_ROOMS; j++){
                                if(strcmp(word, names[j]) == 0){  // check tokenized input against room names to find a match
                                    rooms[roomCounter].connectionNames[rooms[roomCounter].connections] = names[j];
                                    rooms[roomCounter].connections++;
                                    break;
                                }
                            }                   
                        }
                        else{
                            break;                              // if no more 'connections', exit the loop
                        }
                    }
                    word = strtok(NULL, " ");                   // loop already executed initial tokenization of line before break
                    word = strtok(NULL, "\n");
                    if(strcmp(word, "MID_ROOM") == 0){          // extract and set room type
                        rooms[roomCounter].type = MID_ROOM;
                    }
                    else if(strcmp(word, "START_ROOM") == 0){
                        rooms[roomCounter].type = START_ROOM;
                    }
                    else{
                        rooms[roomCounter].type = END_ROOM;
                    }
                    roomCounter++;                              // increment counter so it fills in the next room for next iteration
                    fclose(file);
                }
            }
        }
    }
    closedir(dir);                                              // close directory and navigate out of it
    chdir("..");
}

// source on time formatting: http://zetcode.com/articles/cdatetime/
void* makeTimeFile(){
    pthread_mutex_lock(&lock);
    char* currTime = "currentTime.txt";                     // file name
    FILE* file;
    file = fopen(currTime, "w");
    char timeDisplay[256];                                  // string to hold time
    memset(timeDisplay, '\0', sizeof(timeDisplay));
    time_t now = time(NULL);                                // get time
    struct tm *ptm = localtime(&now);             
    strftime(timeDisplay, 256, "%I:%M%p, %A, %B %d, %Y\n", ptm);
    fprintf(file, "%s", timeDisplay);                       // write to file
    fclose(file);
    pthread_mutex_unlock(&lock);
    return NULL;
}

// simple function to read a line from file named "currentTime.txt"
void printTime(){
    FILE* file;
    file = fopen("currentTime.txt", "r");
    char timeDisplay[256];
    fgets(timeDisplay, sizeof(timeDisplay), file);
    printf("\n%s", timeDisplay);
    fclose(file);
}

// set up and execute main game loop
void startGame(){
    int inputBufferSize = 100;     
    char input[inputBufferSize];                           // buffer for user input
    int totalSteps = 0;                                    // step counter for victory message
    int record[200];                                       // keeps a record of rooms visited based on index in rooms array
    int i;
    for(i = 0; i < 200; i++){
        record[i] = -1;                                    // set all record entries to -1 as it's not a possible array index
    }
    struct room* current = NULL;                           // points to current room
    int won = 0;                                           // bool for game loop; won = 1 when game is over

    for(i = 0; i < ROOM_COUNT; i++){                       // find the start room to be current at the beginning
        if(rooms[i].type == START_ROOM){
            current = &rooms[i];
            break;
        }
    }
    if(current == NULL){                                    // throw an error if rooms cannot be read
        printf("Something went wrong in reading files.\n");
        return;
    }

    int j, k;                                               // counter vars for nested loops
    i = 0;                                                  // reset i to be counter for record index

    // main game loop
    while(!won){
        printf("\nCURRENT LOCATION: %s\n", current->name);
        printf("POSSIBLE CONNECTIONS: ");
        for(j = 0; j < current->connections - 1; j++){       // have to print last connection with a period after it
            printf("%s, ", current->connectionNames[j]);
        }
        printf("%s.\n", current->connectionNames[current->connections - 1]);
        printf("WHERE TO? >");

        memset(input, '\0', sizeof(input));
        fgets(input, inputBufferSize, stdin);                // user input
        for(j = 0; j < inputBufferSize; j++){                // replace newline with null terminator in input string
            if(input[j] == '\n'){
                input[j] = '\0';
                break;
            }
        }
        int validInput = 0;                                  // flag for valid input

        // if time...
        if(strcmp(input, "time") == 0){
            validInput = 1;
            pthread_mutex_unlock(&lock);                                // start second thread
            pthread_join(thread, NULL);                                 // block main thread
            pthread_mutex_lock(&lock);                                  // relock mutex
            pthread_create(&thread, NULL, &makeTimeFile, NULL);         // re-create second thread
            printTime();
            continue;
        }
        // check choice input
        for(j = 0; j < current->connections; j++){
            if(strcmp(current->connectionNames[j], input) == 0){        // check for valid connection
                validInput = 1;
                for(k = 0; k < ROOM_COUNT; k++){
                    if(strcmp(input, rooms[k].name) == 0){              // change to that room if there's a match
                        record[i] = k;                                  // add this room name index to the record
                        i++;
                        totalSteps++;
                        current = &rooms[k];
                    }
                }
                // check for win
                if(current->type == END_ROOM){
                    printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
                    printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", totalSteps);
                    int counter = 0;
                    while(record[counter] != -1){
                        printf("%s\n", rooms[record[counter]].name);    // print names that correspond to indices in room array
                        counter++;
                    }
                    won = 1;                                            // set bool to game won
                    break;
                }
            }
        }
        // invalid input message if no valid input detected
        if(validInput == 0){
            printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
        }
    }
}

int main(){
    // room names array -- must match buildrooms.c file
    char* roomNames[MAX_ROOMS];
    roomNames[0] = "Daenerys";
    roomNames[1] = "Jon";
    roomNames[2] = "Tyrion";
    roomNames[3] = "Sansa";
    roomNames[4] = "Arya";
    roomNames[5] = "Cersei";
    roomNames[6] = "Jaime";
    roomNames[7] = "Bran";
    roomNames[8] = "Gendry";
    roomNames[9] = "Brienne";

    // make room array and set empty values
    int i;
    for (i = 0; i < ROOM_COUNT; i++){
        rooms[i].connections = 0;
        rooms[i].type = MID_ROOM;
    }

    // get room information from directory/files
    readRooms(roomNames);

    // time thread
    int resultInt;
    pthread_mutex_lock(&lock);
    resultInt = pthread_create(&thread, NULL, &makeTimeFile, NULL);
    if(resultInt != 0){
        printf("Unable to create time thread.\n");
        return 1;
    }
    // game loop
    startGame();
    // clean up
    pthread_mutex_unlock(&lock);                            // unlock so there's no mem leak from thread
    pthread_join(thread, NULL);                             // doing that will change the time file
    pthread_mutex_destroy(&lock);                           // but I seriously hope no one takes that long to play this game
    return 0;
}