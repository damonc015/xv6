#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#define NUM_CHILDREN 8
#define NUM_INCREMENTS 5

void read_counter(int fd, int *value) {
    close(fd);
    fd = open("counter", O_RDWR);
    read(fd, value, sizeof(*value));
}

void write_counter(int fd, int value) {
    close(fd);
    fd = open("counter", O_RDWR);
    write(fd, &value, sizeof(value));
}

int
main(void)
{
  int fd = open("counter", O_CREATE | O_RDWR);
  int counter = 0;
  write_counter(fd, counter);
  
  printf(1, "Starting counter: %d\n", counter);

  int i,j;
  for(i = 0; i < NUM_CHILDREN; i++) {
    if(fork() == 0) {  // Child process
      for(j = 0; j < NUM_INCREMENTS; j++) {
        read_counter(fd, &counter);
        counter++;
        write_counter(fd, counter);
      }
      exit();
    }
  }

  for(i = 0; i < NUM_CHILDREN; i++) {
    wait();
  }

  read_counter(fd, &counter);
  printf(1, "Final counter: %d\n", counter);
  printf(1, "Expected: %d\n", NUM_CHILDREN * NUM_INCREMENTS);
  
  unlink("counter");
  exit();
  return 0;
} 