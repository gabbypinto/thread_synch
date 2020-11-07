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
char  *message;
int buffer_size;
int ID[2];
int run = 1;

// sem_t mutex;
sem_t *full =NULL;
sem_t *empty=NULL;

// pthread_mutex_t full;
// pthread_mutex_t empty;
pthread_mutex_t mutex;
int j = 0;
int k= 0;


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
void runHandler(int running){
  run =0;
}

void *thread_producer_function(){
    int   i,shm_fd;
    int   nbytes;
    void  *ptr;

    item next_produced; //item defined above

    unsigned short cksum;

    item next_consumed; //item defined above
    unsigned short cksum1,cksum2;
    unsigned char *buffer;   //change item buffer

    struct sigaction act;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    buffer = (unsigned char *)ptr;
    next_produced.item_no = 0;

    printf("prod\n");
    int item_count=0;
    while(k<buffer_size){
      //1. increment the buffer count (item_no)
      next_produced.item_no++;

      sem_wait(full);
      pthread_mutex_lock(&mutex);

      for(i=0;i<nbytes;i++){
        //3. generate the payload data
        next_produced.payload[i] = (unsigned char) rand() % 256;
      }

      //2. calculate the 16-bit checksum (cksum)
      next_produced.cksum = (unsigned short) ip_checksum(&next_produced.payload[0],PAYLOAD_SIZE);
      //printf("Checksum :0x%x (%s) \n",cksum,argv[1]);
      printf("next: %hu", next_produced.cksum);

      memcpy((void *)&buffer_item[k],&next_produced,sizeof(next_produced));
      printf("Buffer item:%hu", buffer_item->cksum);
      //memcpy((void*)&cksum2, &buffer_item[PAYLOAD_SIZE],sizeof(unsigned short));

      pthread_mutex_unlock(&mutex);
      sem_post((sem_t *)&full);
      item_count++;
      k++;

      //sigaction(SIGINT, &act, 0);
      printf("in while\n");
    }

    // pthread_exit("Thank you for the CPU Time");
    printf("end of while!!!!");
    pthread_exit(0);

}

void *thread_consumer_function(){
  printf("HI\n");
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

  while(j<buffer_size){
    //decrement empty
    sem_wait((sem_t *)&full);  //sem_wait
    pthread_mutex_lock(&mutex); //use  mutex lock instead
    printf("%hu\n",buffer_item->cksum);
    memcpy((void*)&cksum2, (void *)&buffer_item,sizeof(unsigned short));

    //consumer the item in next_consumed
    //1. check for no skipped buffers (item_no is continguous)

    if(!(bufferCount==next_consumed.item_no)){
      printf("buffer count:%d\n", bufferCount);
      printf("next confusmed %d\n", next_consumed.item_no);
      printf("Skipped buffers\n");
      printf("Exiting\n");
      exit(1);
    }

    /* read the message to shared memory */
    cksum1 = (unsigned short )ip_checksum ((unsigned char  *)buffer_item,PAYLOAD_SIZE);
    printf("chksum1: %hu \n",cksum1);
    printf("chksum2: %hu \n",cksum2);
    //2. verify the calculated checksum matches what is stored in next_consumed
    if(cksum1!=cksum2){
      printf("checksum mismatch: received 0x%x, expected 0x%x \n",cksum2,cksum1);
      exit(0);
    }
    else{
      printf("good");
    }

    // sigaction(SIGINT, &act, 0);
    pthread_mutex_unlock(&mutex);
    sem_post(empty);
    next_consumed.item_no++;
    bufferCount++;
    j++;
  }
    // printf("thread_function CONSUMER is running. Message is %s\n", message);
    // strcpy(message, "Good-Bye");
    // pthread_exit("Thank you for the CPU Time");
    return NULL;
}


int main (int argc, char *argv[]){
  int nitems; //number of items in shared buffer
  pthread_t consumerid; //thread identifier for consummer
  pthread_t producerid; //for producer
  ID[0]=0;
  ID[1]=1;

  if (argc != 2) {
      printf("Usage: %s <nitems> \n", argv[1]);
      return -1;
  }
  nitems = atoi(argv[1]);
  // //check if item exceeds cap, exit if too big - 64k is max
  if(nitems*40>MEMSIZE || nitems<=0){
    printf("Too many items");
    return -1;
  }
//allocate buffer based on nitem, use new or malloc. ie. if user typed in two item allocate 80bytessss
  buffer_size=nitems*40;
  buffer_item = malloc(buffer_size); //?
  int status;
  status = pthread_mutex_init(&mutex,0);
  if (status!=0){
    perror("mutex initialization failed");
    return -1;
  }
  // //sem_init(&mutex,0)
  //
  // //initialize the counting--maybe don't start at one
  // //sem_init(&full, 0);
  // fullNum = sem_open((pthread_mutex_t*)&full,0);
  // //
  // // // sem_init(&empty,nitems);
  // emptyNum = sem_open((pthread_mutex_t*)&empty,nitems);
  full = sem_open("full", O_CREAT | O_EXCL, 0644, 1);
  empty = sem_open("empty", O_CREAT | O_EXCL, 0644, nitems);
  pthread_create(&producerid,NULL,thread_producer_function,(void *) &ID[0]);
  pthread_create(&consumerid,NULL,(void *)thread_consumer_function,(void *) &ID[1]);
  pthread_exit(0);
}
