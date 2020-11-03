//Gabriela Pinto and Katherine Hansen
//2318655 and 2326665

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/wait.h>



#define MEMSIZE 64000
extern unsigned int ip_checksum(unsigned char *data, int length);
#define MMAP_SIZE 4096
#define PAYLOAD_SIZE 34
const char *name = "OS-IPC";

typedef struct {
  int item_no;          //number of the item produced
  unsigned short cksum;         //16-bit Internet checksum
  unsigned char payload[PAYLOAD_SIZE];      //random generated data
} item;

item buffer_item[MEMSIZE];
int in = 0;
int out = 0;


void sig_handler(int sig){
    printf("\nCtrlc found\n");
    /* remove the shared memory object */
    shm_unlink(name);
    printf("Exiting\n");
    exit(1);
}

void sighandler(int);

void sighandler(int signum) {
   printf("Caught signal %d, coming out...\n", signum);
  // exit(1);
}

void *thread_producer_function(int shared_mem, sem_t mutex, sem_t counting, sem_t empty)
{

  //  printf("thread_function PRODUCER is running. Message is %s\n", message);
//    sleep(3);
    // strcpy(message, "Good-Bye");

    int   i,shm_fd;
    int   nbytes;
    void  *ptr;
    char  *message;

    item next_produced; //item defined above

    unsigned short cksum;

    item next_consumed; //item defined above
    unsigned short cksum1,cksum2;
    unsigned char *buffer;   //change item buffer

    struct sigaction act;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;


    /* configure the size of the shared memory object */
    ftruncate(shared_mem, MMAP_SIZE);

    /* memory map the shared memory object */
    ptr = mmap(0, MMAP_SIZE, PROT_WRITE, MAP_SHARED, shared_mem, 0);

    buffer = (unsigned char *)ptr;
    next_produced.item_no = 0;

    while(1){
      //1. increment the buffer count (item_no)
      next_produced.item_no++;

      wait(&counting);
      wait(&mutex);

      for(i=0;i<nbytes;i++){
        //3. generate the payload data
        next_produced.payload[i] = (unsigned char) rand() % 256;
      }

      //2. calculate the 16-bit checksum (cksum)
      next_produced.cksum = (unsigned short) ip_checksum(&next_produced.payload[0],PAYLOAD_SIZE);
      //printf("Checksum :0x%x (%s) \n",cksum,argv[1]);

      // while (((in + 1) % BUFFER_SIZE) == out){
      //     sleep(1);     //do nothing but sleep for 1 second
      // }

      memcpy((void *)&buffer_item[in],&next_produced,sizeof(next_produced));

      signal(&mutex, sighandler);
      signal(&counting, sighandler);

      buffer_item[in] = next_produced;       //store next_produced into share buffer size
      in = (in+1) % MEMSIZE;

      sigaction(SIGINT, &act, 0);
    }

    pthread_exit("Thank you for the CPU Time");

}

void *thread_consumer_function(int shared_mem, sem_t mutex, sem_t counting, sem_t empty)
{

  int   shm_fd;
  void  *ptr;
  int   nbytes;

  item next_consumed; //item defined above
  unsigned short cksum1,cksum2;
  unsigned char *buffer;   //change item buffer

  struct sigaction act;

  act.sa_handler = sig_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;

  int bufferCount=0;
  int count=0;

   while(true){
     wait(&counting);
     wait(&mutex);
  //
  //   while(in == out){
  //     sleep(1);             //do nothing but sleep for 1 second
  //     sigaction(SIGINT, &act, 0);
  //   }

    memcpy((void*)&cksum2, (void*)&buffer[PAYLOAD_SIZE],sizeof(unsigned short));
    next_consumed = buffer_item[out];
    out = (out+1) % MEMSIZE;

    //consumer the item in next_consumed
    //1. check for no skipped buffers (item_no is continguous)
    if(!(bufferCount==next_consumed.item_no)){
      printf("Skipped buffers\n");
      printf("Exiting\n");
      exit(1);
    }

    /* configure the size of the shared memory object */
    ftruncate(shm_fd, MMAP_SIZE);

    /* memory map the shared memory object */
    ptr = mmap(0, MMAP_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);

    /* read the message to shared memory */
    //printf("%s\n", (char *)ptr);

    buffer = (unsigned char *)ptr;
    cksum1 = (unsigned short )ip_checksum (&buffer[0],PAYLOAD_SIZE);


    //2. verify the calculated checksum matches what is stored in next_consumed
    if(cksum1!=cksum2){
      printf("checksum mismatch: received 0x%x, expected 0x%x \n",cksum2,cksum1);
      break;
    }
    sigaction(SIGINT, &act, 0);
    bufferCount++;
    signal(&mutex,sighandler);
    signal(&empty,sighandler);
    next_consumed.item_no++;
    }

    // printf("thread_function CONSUMER is running. Message is %s\n", message);
  //  sleep(3);
    // strcpy(message, "Good-Bye");
    pthread_exit("Thank you for the CPU Time");

}


int main (int argc, char *argv[]){
  int nitems; //number of items in shared buffer
  pthread_t consumerid; //thread identifier for consummer
  pthread_t producerid; //for producer

  if (argc != 2) {
      printf("Usage: %s <nitems> \n", argv[1]);
      return -1;
  }

  nitems = (int) argv[1];
  int shared_mem = shm_open(nitems,O_RDONLY, 0666);

  //error checking, shared memory wasn't created
  if (shared_mem == -1) {
      perror("Error creating shared memory");
      return -1;
  }

  //initialize the mutex
  sem_t mutex;
  sem_init(&mutex, 1, 1);
  //initialize the counting
  sem_t counting;
  sem_init(&counting, 1, 0);

  sem_t empty;
  sem_init(&empty,1,nitems);

  // pthread_attr_init(&attr);
  pthread_create(&producerid,NULL,thread_producer_function(shared_mem, mutex, counting, empty),NULL);
  pthread_create(&consumerid,NULL,thread_consumer_function(shared_mem, mutex, counting, empty),NULL);

}