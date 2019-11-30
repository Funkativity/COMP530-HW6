#include "buffer.h"

// slightly modified version from mmap-example
Buffer* make_buff(size_t size, char * sem_empty_name, char *sem_full_name){
  //These are the neccessary arguments for mmap. See man mmap.
  void* addr = 0;
  int protections = PROT_READ|PROT_WRITE; //can read and write
  int flags = MAP_SHARED | MAP_ANONYMOUS; //shared b/w procs & not mapped to a file
  int fd = -1; //We could make it map to a file as well but here it is not needed.
  off_t offset = 0;
    
  //Create memory map
  Buffer* state =  mmap(addr, size, protections, flags, fd, offset);
  if (( void *) ERROR == state){//on an error mmap returns void* -1.
    perror("error with mmap");
    exit(EXIT_FAILURE);
  }

  state->next_in = 0;
  state->next_out = 0;
  state->full_slots = sem_open(sem_full_name, O_CREAT, S_IREAD | S_IWRITE, 0);
  state->empty_slots = sem_open(sem_empty_name, O_CREAT, S_IREAD | S_IWRITE, BUFFER_SIZE);
  return state;
}

// returns the the next item in the buffer
// sem locks until there is an unread item in buff
char remoove(Buffer *buff){
    char rt;
    
    
    assert(sem_wait((buff->full_slots)) == 0);
    //make sure that we have at least one buffer that is not full
    int num_full;
    sem_getvalue((buff->full_slots), &num_full);
    assert(num_full < BUFFER_SIZE);

    rt = buff->buff[buff->next_out];
    buff->next_out = (buff->next_out + 1) % BUFFER_SIZE;

    //make sure that we have at least one buffer that is empty
    int num_empty;
    sem_getvalue((buff->empty_slots), &num_empty);
    assert(num_empty >= 0);
    assert(sem_post((buff->empty_slots)) == 0);
    return rt;
}

// add a new element to the buffer
// sem locks until there is a free tem in the slot
void deposit(Buffer *buff, char new_char){
    assert(sem_wait((buff->empty_slots))==0);
    //make sure that we have at least one buffer that is empty
    int num_empty;
    sem_getvalue((buff->empty_slots), &num_empty);
    assert(num_empty >= 0);

    buff->buff[buff->next_in] = new_char;
    buff->next_in = (buff->next_in + 1) % BUFFER_SIZE;

    //make sure that we have at least one buffer that is not full
    int num_full;
    sem_getvalue((buff->full_slots), &num_full);
    assert(num_full < BUFFER_SIZE);

    assert(sem_post((buff->full_slots))==0);
    return;
}

void delete_buff(Buffer *buffer){
    // not sure if these are dynamically allocated / if this is necessary
    sem_close((buffer->empty_slots));
    sem_close((buffer->empty_slots));

    if (ERROR == munmap(buffer, sizeof(Buffer))){
		perror("error deleting mmap");
		exit(EXIT_FAILURE);
	}
    return;
}