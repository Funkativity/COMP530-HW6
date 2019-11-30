
/*
 * This is a sample program for using memory mapping.
 * It is done by the function mmap (see man 2 mmap).
 * In this example there are two processes that use a memory mapped loction
 * to communicate. 
 * The first process simply reads from stdin and puts chars in an array full.
 * The second process reads from the array and prints out each char until it
 * has read the whole array.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>  // mmap, munmap
#include <unistd.h>    // sleep
#include <sys/types.h> // kill
#include <signal.h>    // kill

#define MESSAGE_LENGTH 80
#define ERROR -1
#define SLEEP_TIME 1

/* This is the object that is mapped to memory. It has an array
 * and a count of objects in the array.
 */
typedef struct {
    char MSG[MESSAGE_LENGTH];
    int count;
} messageObject;

// Prototypes for functions
void writer(messageObject *state);
void reader(messageObject *state);
void waitForChildren(pid_t*);
pid_t forkChild(void (*func)(messageObject *), messageObject* state);
messageObject* createMMAP(size_t size);
void deleteMMAP(void*);

int main () {
    
    //get address of memory mapped location.
    messageObject* state = createMMAP(sizeof(messageObject));
    state->count = 0; // initialize count
    
    //Fork children
    pid_t childpids[2];
    childpids[0] = forkChild(writer, state);
    childpids[1] = forkChild(reader, state);
 
    //wait for them
    waitForChildren(childpids);

    //cleanup
    deleteMMAP(state);
    exit(EXIT_SUCCESS);
}

messageObject* createMMAP(size_t size){
  //These are the neccessary arguments for mmap. See man mmap.
  void* addr = 0;
  int protections = PROT_READ|PROT_WRITE; //can read and write
  int flags = MAP_SHARED|MAP_ANONYMOUS; //shared b/w procs & not mapped to a file
  int fd = -1; //We could make it map to a file as well but here it is not needed.
  off_t offset = 0;
    
  //Create memory map
  messageObject* state =  mmap(addr, size, protections, flags, fd, offset);
    
  if (( void *) ERROR == state){//on an error mmap returns void* -1.
	perror("error with mmap");
	exit(EXIT_FAILURE);
  }
  return state;
}

void deleteMMAP(void* addr){
	//This deletes the memory map at given address. see man mmap
	if (ERROR == munmap(addr, sizeof(messageObject))){
		perror("error deleting mmap");
		exit(EXIT_FAILURE);
	}
}

pid_t forkChild(void (*function)(messageObject *), messageObject* state){
     //This function takes a pointer to a function as an argument
     //and the functions argument. It then returns the forked child's pid.

	pid_t childpid;
        switch (childpid = fork()) {
                case ERROR:
                        perror("fork error");
                        exit(EXIT_FAILURE);
                case 0:	
                        (*function)(state);
                default:
                       return childpid;
        }
}

void waitForChildren(pid_t* childpids){
	int status;
	while(ERROR < wait(&status)){ //Here the parent waits on any child.
		if(!WIFEXITED(status)){ //If the termination err, kill all children.
			kill(childpids[0], SIGKILL);
	 		kill(childpids[1], SIGKILL);
			break;
	 	}
	}
}

// The execution path for writer
void writer(messageObject* state) {
    //This process reads a char & writes it until the array is full.
    while(1){
	while (MESSAGE_LENGTH == state->count){ //busy wait if condition is true
		sleep(SLEEP_TIME);
		// This is a naive/bad approach to synchronization, wouldn't a POSIX semaphore be great!
		
	}
        char c = fgetc(stdin); //read in a char
        state->MSG[state->count] = c;
	state->count++;
        if (EOF == c){ //if EOF break out of loop and exit
            state->count = MESSAGE_LENGTH; //Set this condition so that the other process can read
	    exit(EXIT_SUCCESS);
        }
    }
    
    return;
}

// The execution path for reader
void reader(messageObject* state) {
    //simply prints a char until the array is empty.
    int localCount = 0;//used to keep track of what is next to read.
    while(1){
	while (MESSAGE_LENGTH > state->count){
		sleep(SLEEP_TIME);
		//Once again a bad way to do synchronization.
	}
       	//Read in char
	char c = state->MSG[localCount];
       
	if (EOF == c){//break if EOF
            exit(EXIT_SUCCESS);
        }
        //print char
	putchar(c);
	//update count
	localCount++;
	//check if array is full and reset counts
	if (MESSAGE_LENGTH == localCount){
	    localCount = 0;
	    state->count = 0;
	}
}

    return;
}


