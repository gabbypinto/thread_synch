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
sem_t mutex;
sem_t full;
sem_t empty;

typedef struct {
  int item_no;          //number of the item produced
  unsigned short cksum;         //16-bit Internet checksum
  unsigned char payload[PAYLOAD_SIZE];      //random generated data
} item;

//item buffer_item[MEMSIZE];
item* buffer_item;


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

void *thread_producer_function()
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
  //  ftruncate(shared_mem, MMAP_SIZE);

    /* memory map the shared memory object */
  //  ptr = mmap(0, MMAP_SIZE, PROT_WRITE, MAP_SHARED, shared_mem, 0);

    buffer = (unsigned char *)ptr;
    next_produced.item_no = 0;

    while(1){
      //1. increment the buffer count (item_no)
      next_produced.item_no++;

      wait(&full);
      wait(&mutex);

      for(i=0;i<nbytes;i++){
        //3. generate the payload data
        next_produced.payload[i] = (unsigned char) rand() % 256;
      }

      //2. calculate the 16-bit checksum (cksum)
      next_produced.cksum = (unsigned short) ip_checksum(&next_produced.payload[0],PAYLOAD_SIZE);
      //printf("Checksum :0x%x (%s) \n",cksum,argv[1]);

      memcpy((void *)&buffer_item,&next_produced,sizeof(next_produced));

      sem_post(&mutex);
      sem_post(&full);

      sigaction(SIGINT, &act, 0);
    }

    pthread_exit("Thank you for the CPU Time");

}

void *thread_consumer_function()
{
//one signal handler for global...do in main (once)
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
     //decrement empty
     wait(&full);  //sem_wait
     wait(&mutex); //use  mutex lock instead
  //
  //   while(in == out){
  //     sleep(1);             //do nothing but sleep for 1 second
  //     sigaction(SIGINT, &act, 0);
  //   }

    memcpy((void*)&cksum2, (void*)&buffer[PAYLOAD_SIZE],sizeof(unsigned short));
    //unlock mutex
  //  next_consumed = buffer_item[out];
  //  out = (out+1) % MEMSIZE;

    //consumer the item in next_consumed
    //1. check for no skipped buffers (item_no is continguous)
    if(!(bufferCount==next_consumed.item_no)){
      printf("Skipped buffers\n");
      printf("Exiting\n");
      exit(1);
    }

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
    //pthread mutex unlock
    sem_post(&mutex);
    //sem post
    sem_post(&empty);
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
//allocate buffer based on nitem, use new or malloc. ie. if user typed in two item allocate 80bytessss
  buffer_item = malloc(nitems*40); //?
//check if item exceeds cap, exit if too big - 64k is max
  if(nitems*40>MEMSIZE){
    printf("Too many items");
    return -1;
  }

//these are global...
  //initialize the mutex

  sem_init(&mutex,0);
  //P_thread mutex init ^

  //initialize the counting--maybe don't start at one
  //same as full
  sem_init(&full, 0);


  sem_init(&empty,nitems);

  // pthread_attr_init(&attr);  use one parameter
  pthread_create(&producerid,NULL,thread_producer_function(),NULL);
  pthread_create(&consumerid,NULL,thread_consumer_function(),NULL);

}
