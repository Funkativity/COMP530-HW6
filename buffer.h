#ifndef C_BUFFER
#define C_BUFFER

#include <assert.h>
#include <semaphore.h>
#include <sys/mman.h>  // mmap, munmap
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> // kill
#include <sys/stat.h> // kill
#include <signal.h>    // kill
#include <fcntl.h>

#define BUFFER_SIZE 80
#define ERROR -1


typedef struct {
    sem_t *full_slots;
    sem_t *empty_slots;
    char buff[BUFFER_SIZE];
    int next_in;
    int next_out;
} Buffer;

Buffer* make_buff(size_t buffer_size, char *sem_empty_name, char *sem_full_name);
char remoove(Buffer *buff);
void deposit(Buffer *buff, char a);
void delete_buff(Buffer *buff);

#endif // C_BUFFER