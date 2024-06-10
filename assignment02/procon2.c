#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

#define N_COUNTER 4 // the size of a shared buffer
#define MILLI 1000  // time scale

void mywrite(int n);
int myread();

sem_t semWrite, semRead; // POSIX semaphore
int queue[N_COUNTER]; // shared buffer
int wptr = 0; // write pointer for queue[]
int rptr = 0; // read pointer for queue[]

int flag[2] = {0, 0}; // Peterson algorithm flags
int turn = 0; // Peterson algorithm turn

void peterson_lock(int id) {
  int other = 1 - id;
  flag[id] = 1;
  turn = other;
  asm("mfence"); // Memory fence
  while (flag[other] && turn == other) {
    // busy wait
  }
}

void peterson_unlock(int id) {
  flag[id] = 0;
  asm("mfence"); // Memory fence
}

// producer thread function
void* producer(void* arg) { 
  for(int i = 0; i < 10; i++) {
    peterson_lock(0);
    mywrite(i); // write i into the shared memory
    printf("producer : wrote %d\n", i);
    peterson_unlock(0);

    // sleep m milliseconds
    int m = rand() % 10;
    usleep(MILLI * m * 10); // m * 10 milliseconds
  }
  return NULL;
}

// consumer thread function
void* consumer(void* arg) { 
  for(int i = 0; i < 10; i++) {
    peterson_lock(1);
    int n = myread(); // read a value from the shared memory
    printf("\tconsumer : read %d\n", n);
    peterson_unlock(1);

    // sleep m milliseconds
    int m = rand() % 10;
    usleep(MILLI * m * 10); // m * 10 milliseconds
  }
  return NULL;
}

// write n into the shared memory
void mywrite(int n) { 
  // wait for empty space
  sem_wait(&semWrite);
  
  queue[wptr] = n;
  wptr = (wptr + 1) % N_COUNTER;

  // signal that there is a new item
  sem_post(&semRead);
}

// read a value from the shared memory
int myread() { 
  // wait for an available item
  sem_wait(&semRead);

  int n = queue[rptr];
  rptr = (rptr + 1) % N_COUNTER;

  // signal that there is empty space
  sem_post(&semWrite);

  return n;
}

int main() {
  pthread_t t[2]; // thread structure
  srand(time(NULL)); 

  // init semaphore
  sem_init(&semWrite, 0, N_COUNTER); // initial value is the size of the buffer
  sem_init(&semRead, 0, 0); // initial value is 0

  // create the threads for the producer and consumer
  pthread_create(&t[0], NULL, producer, NULL); 
  pthread_create(&t[1], NULL, consumer, NULL); 

  for(int i = 0; i < 2; i++)
    pthread_join(t[i], NULL); // wait for the threads

  //destroy the semaphores
  sem_destroy(&semWrite);
  sem_destroy(&semRead);

  return 0;
}
