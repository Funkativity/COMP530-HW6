// Author: Victor Murta
// Homework one for COMP 530: Operating Systems
// UNC Chapel Hill, Fall 2019
///////////////////////////////////////////////////////////////////////////////
// Description : Write a C program on Linux to read in a stream of characters
// from standard input (i.e., the keyboard) and write them as 80 character lines
// to standard output (i.e., the “screen”) with the following changes:
// Every enter/return (newline) character in the input stream is replaced by a space
// Every adjacent pair of asterisks “**” in the input stream is replaced by a “^”.
// now we also do this memmory mapped files and  a custom buffer defined in buffer.h
// several helper functions have been copied from the provided example mmap-example.c

#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>   
#include <sys/wait.h>
#include <fcntl.h>


#define SEM_FULL_READ_NEW   "/sem_full_read_new"
#define SEM_FULL_NEW_AST    "/sem_full_new_ast"
#define SEM_FULL_AST_WRIT   "/sem_full_ast_writ"
#define SEM_EMPTY_READ_NEW  "/sem_empty_read_new"
#define SEM_EMPTY_NEW_AST   "/sem_empty_new_ast"
#define SEM_EMPTY_AST_WRIT  "/sem_empty_ast_writ"

// Helper functions from mmap-example.c
////////////////////////////////////////////////////////////////////////////////
pid_t forkChild(void (*function)(Buffer *), Buffer* state){
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

//"overloadded" version that for functions that take in 2 buffers
pid_t forkFatChild(void (*function)(Buffer *, Buffer *), Buffer* in, Buffer* out){
     //This function takes a pointer to a function as an argument
     //and the functions arguments. It then returns the forked child's pid.

	pid_t childpid;
        switch (childpid = fork()) {
                case ERROR:
                        perror("fork error");
                        exit(EXIT_FAILURE);
                case 0:	
                        (*function)(in, out);
                default:
                       return childpid;
        }
}

//modified for 4 children
void waitForChildren(pid_t* childpids){
	int status;
	while(ERROR < wait(&status)){ //Here the parent waits on any child.
		if(!WIFEXITED(status)){ //If the termination err, kill all children.
			kill(childpids[0], SIGKILL);
			kill(childpids[1], SIGKILL);
			kill(childpids[2], SIGKILL);
	 		kill(childpids[3], SIGKILL);
			break;
	 	}
	}
}


////////////////////////////////////////////////////////////////////////////////




// just puts input onto the buffer until EOF
void processRead(Buffer *state){
    char c;
    do {
        c = getchar();
        deposit(state, c);
    } while(c != EOF);

    exit(EXIT_SUCCESS);
}

//converts newlines to spaces, passes it on to the next buffer
void processReturns(Buffer *in, Buffer *out){
    char curChar;
    do {
        curChar = remoove(in);
        if (curChar == '\n'){
            curChar = ' ';
        }
        deposit(out, curChar);
    } while (curChar != EOF);
    
    exit(EXIT_SUCCESS);
}

//thread that transforms pairs of asterisks into '^'
void processAsterisks(Buffer *in, Buffer *out){
    char curChar, nextChar;
    do {
        curChar = remoove(in);
        // when '*' is encountered, peekahead to see if 
        // translation is needed to '^'
        if (curChar == '*'){
            nextChar = remoove(in);
            if (nextChar == '*'){
                deposit(out, '^');
            }
            else {
                deposit(out, curChar);
                deposit(out, nextChar);
            }
        }
        // non-asterisk case
        else {
            deposit(out, curChar);
        }
    } while (curChar != EOF && nextChar != EOF);

    exit(EXIT_SUCCESS);
}

//fills a buffer with 80 characters then prints them
void processWrite(Buffer *in){

    char line[BUFFER_SIZE]; // buffer for storing characers until 80 is reached
    int index = 0;
    char curChar;
    do {
        // once buffer full, print and reset index
        if (index == BUFFER_SIZE){
            puts(line);
            index = 0;
        }
        curChar = remoove(in);
        line[index] = curChar;
        index++;
    } while (curChar != EOF);
    exit(EXIT_SUCCESS);

}

int main(){
    // communication between the reading proc and the newline conversion proc
    Buffer *read_newline = make_buff(sizeof(Buffer), SEM_EMPTY_READ_NEW, SEM_FULL_READ_NEW);

    // communication between the newline conversion proc and the asterisk conversion proc
    Buffer *newline_asterisk = make_buff(sizeof(Buffer), SEM_EMPTY_NEW_AST, SEM_FULL_NEW_AST);

    // communication between the asterisk conversion proc and the writing  proc
    Buffer *asterisk_write = make_buff(sizeof(Buffer), SEM_EMPTY_AST_WRIT, SEM_FULL_AST_WRIT);

    //Fork children
    pid_t childpids[4];
    childpids[0] = forkChild(processRead, read_newline);
    childpids[1] = forkFatChild(processReturns, read_newline, newline_asterisk);
    childpids[2] = forkFatChild(processAsterisks, newline_asterisk, asterisk_write);
    childpids[3] = forkChild(processWrite, asterisk_write);
    
    //wait for them
    waitForChildren(childpids);


    delete_buff(read_newline);
    delete_buff(newline_asterisk);
    delete_buff(asterisk_write);

    sem_unlink(SEM_FULL_AST_WRIT);
    sem_unlink(SEM_FULL_NEW_AST);
    sem_unlink(SEM_FULL_READ_NEW);
    sem_unlink(SEM_EMPTY_AST_WRIT);
    sem_unlink(SEM_EMPTY_NEW_AST);
    sem_unlink(SEM_EMPTY_READ_NEW);
}