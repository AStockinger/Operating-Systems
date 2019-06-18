/**************************************************************************
 * Name: Amy Stockinger
 * CS344 Program 2: adventure.c
 * Date: 4/23/2019
 * Description: This helper program builds the room files to be
 * used in adventure.c in a directory named with my ONID and
 * the program's processID
 *************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define MAX_ROOMS 10     // change array of names in main if this number changes
#define ROOM_COUNT 7     // the number of room files to be made
#define MAX_CON 6        // maximum connections a single room can have
#define MIN_CON 3        // minumum number of connections for a single room

enum roomType { START_ROOM, MID_ROOM, END_ROOM };

struct room {
    char* name;                         // room's name, array of possible names in main()
    int connections;                    // number of room's current connected rooms
    char* connectionNames[MAX_CON];     // name of room's current connected rooms
    enum roomType type;                 // assigned room type (start, mid, end)
};

// rooms must have connections that go both ways per specs
// i.e. an initial room connected to another room means that the second one is also connected to the first
void connect(struct room rooms[], int one, int two){
    int i;
    // check indexes are in bounds
    if(one > ROOM_COUNT || one < 0 || two > ROOM_COUNT || two < 0){
        return;
    }
    // if same room, return
    if(strcmp(rooms[one].name, rooms[two].name) == 0){
        return;
    }
    // if one room has max connections already, return
    if(rooms[one].connections == MAX_CON || rooms[two].connections == MAX_CON){
        return;
    }
    // if already connected, return
    for(i = 0; i < rooms[one].connections; i++){
        if(strcmp(rooms[one].connectionNames[i], rooms[two].name) == 0){
            return;
        }
    }
    // else, connect both rooms and increcrement connection counts
    rooms[one].connectionNames[rooms[one].connections] = rooms[two].name;
    rooms[two].connectionNames[rooms[two].connections] = rooms[one].name;
    rooms[one].connections++;
    rooms[two].connections++;
}

// function shuffles/assigns room names and randomly picks a room to connect to 3 others
// this means each room will have at least 3 connections initially, but can add more if
// randomly chosen to be one of the 3 connected to another room.
// end rooms tend to only have 3 connections--which I think makes the game a little 
// more interesting
void assignRooms(struct room rooms[], char* names[]){
    // shuffle names
    int i;
    for(i = MAX_ROOMS - 1; i > 0; i--){
        int j = rand() % (i + 1);
        char* temp = names[j];
        names[j] = names[i];
        names[i] = temp;
    }
    // assign names
    for(i = 0; i < ROOM_COUNT; i++){
        rooms[i].name = names[i];
    }
    // connect rooms with at least 3 connections each
    for(i = 0; i < ROOM_COUNT; i++){
        while(rooms[i].connections < MIN_CON){
            int index = rand() % MAX_CON;
            connect(rooms, i, index);
        }
    }
}

void makeFiles(struct room rooms[]){
    // make and open directory for files
    char folderName[100];                                       // char array to hold folder name
    memset(folderName, '\0', sizeof(folderName));
    sprintf(folderName, "stockina.rooms.%d", getpid());         // name folder with PID
    mkdir(folderName, 0770);
    chdir(folderName);                                          // navigate into folder to write files
    FILE* file;
    int i, j;

    // make and write to room files per specs
    for(i = 0; i < ROOM_COUNT; i++){
        char fileName[100];
        memset(fileName, '\0', sizeof(fileName));
        sprintf(fileName, "%s_room", rooms[i].name);            // roomName_room as file name
        file = fopen(fileName, "w");                            // open file for writing
        fprintf(file, "ROOM NAME: %s\n", rooms[i].name);        // print room name
        for(j = 0; j < rooms[i].connections; j++){              // cycle through and add connection names
            fprintf(file, "CONNECTION %d: %s\n", j + 1, rooms[i].connectionNames[j]);
        }
        if(rooms[i].type == START_ROOM){                        // conditional to print room type
            fprintf(file, "ROOM TYPE: START_ROOM\n");          
        }
        else if(rooms[i].type == END_ROOM){
            fprintf(file, "ROOM TYPE: END_ROOM\n");
        }
        else{
            fprintf(file, "ROOM TYPE: MID_ROOM\n");
        }
        fclose(file);
    }
    chdir("..");                                                // return back to parent folder
}

int main(){
    // random seed
    srand(time(NULL));

    // room names array
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
    struct room rooms[ROOM_COUNT];
    for (i = 0; i < ROOM_COUNT; i++){
        rooms[i].connections = 0;
        rooms[i].type = MID_ROOM;
    }
    // set start and end
    rooms[0].type = START_ROOM;
    rooms[ROOM_COUNT - 1].type = END_ROOM;

    // assign random names and make room connections
    assignRooms(rooms, roomNames);

    // make directory and add room files
    makeFiles(rooms);
    return 0;
}