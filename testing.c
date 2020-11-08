#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/wait.h>

#define PAYLOAD_SIZE 34
#define BUFFER_SIZE 100
#define MEMSIZE 64000

extern unsigned int ip_checksum(unsigned char *data, int length);
void *producer_function();
void *consumer_function();
sem_t *full =NULL;
sem_t *empty=NULL;
sem_t *semid=NULL;

int nitems;
int buffer_size;

int flag = 0;
int exitFlag=0;

pthread_mutex_t mutex;
// unsigned char *buffer;

void sig_handler(int sig){
    printf("\nCtrlc found\n");
    /* remove the shared memory object */
    printf("Exiting\n");
    //exit(1);  //get rid of it
    //global variable called done or flag
    flag = 1;
    exit(1);
}

// int in = 0;
// int out = 0;
// producer:  in = (in+1)%nitmes
// consumer: out = (out+1)%ntimes;

typedef struct {
  int item_no;          //number of the item produced
  unsigned short cksum;         //16-bit Internet checksum
  unsigned char payload[PAYLOAD_SIZE];      //random generated data
} item;

// typedef struct{
//   int in;
//   int out;
// } sharedMemObj;

item *itemBuffer;
// sharedMemObj *sharedProd;
// sharedMemObj *sharedCons;

void *producer_function()
{
  //printf("entering producer\n");
    int i;
    int in =0;
    int out = 0;

    item next_produced;
    //sharedProd= (sharedMemObj *)((uint64_t)itemBuffer + sizeof(item)*nitems);
    next_produced.item_no=0;
    //
    //unsigned short cksum, cksum1, cksum2;
    // struct sigaction act;
    // act.sa_handler = sig_handler;
    // sigemptyset(&act.sa_mask);
    // act.sa_flags = 0;
    //int count =0;
    while(!flag) {
      //count++;
      //  itemBuffer = rand(); // Produce an random item
      for(i=0;i<PAYLOAD_SIZE;i++){//3. generate the payload data
        next_produced.payload[i] = (unsigned char) rand() % 256;
      }
        sem_wait(empty);
        pthread_mutex_lock(&mutex);

        next_produced.cksum = (unsigned short) ip_checksum(&next_produced.payload[0],PAYLOAD_SIZE);
        memcpy((void *)&itemBuffer[in],(void *)&next_produced,sizeof(item));
        next_produced.item_no++;

        in = (in+1)%nitems;
        //itemBuffer[in] = next_produced;
        //printf("item num in prod: %d",next_produced.item_no);
        // printf("in %d\n",in);
        pthread_mutex_unlock(&mutex);
        sem_post(full);

        // sigaction(SIGINT, &act, 0);
    }
    pthread_exit(0);
}
void *consumer_function()
{
    int in =0;
    int out = 0;
    // struct sigaction act;
    // act.sa_handler = sig_handler;
    // sigemptyset(&act.sa_mask);
    // act.sa_flags = 1;

    item next_consumed;
    unsigned short cksum1, cksum2;
    int bufferCount = 0;
  //  sharedCons =(sharedMemObj *)((uint64_t)itemBuffer + sizeof(item)*nitems);
    //printf("consumer func\n");

    //starting flag
    while(!flag) {  //while !flag??
        //count++;
        //printf("in while\n");
        sem_wait(full);
        //printf("after wait full");
        pthread_mutex_lock(&mutex);
        //printf("after wait nutex");
        memcpy((void*)&next_consumed, (void*)&itemBuffer[out],sizeof(item)); //<-seg here

        pthread_mutex_unlock(&mutex);
        sem_post(empty);

        if(bufferCount!=(next_consumed.item_no)){
          printf("item no:%d\n", next_consumed.item_no);
          printf("buffercount%d\n", bufferCount);
          printf("Item number not correct");
          exitFlag=1;

      }

        bufferCount++;
        //next_consumed = itemBuffer[out];

        //printf("Consumer %d: Remove Item %d from %d\n",*((int *)cno),item, out);
        out = (out+1)%nitems;
        // printf("out: %d\n",out);
        //Ask about this :)
        cksum2=next_consumed.cksum;
        cksum1 = (unsigned short )ip_checksum (&next_consumed.payload[0],PAYLOAD_SIZE); //<-seg here
        if(cksum1!=cksum2){
          printf("checksum mismatch: received 0x%x, expected 0x%x \n",cksum2,cksum1);
          break;
        }
        //bufferCount++;
        // sigaction(SIGINT, &act, 0);
    }
    pthread_exit(0);
}

int main(int argc, char *argv[])
{

    pthread_t producer,consumer;

    int status;//for error checking
    pthread_attr_t attr;
    // sem_init(&empty,0,BufferSize);
    // sem_init(&full,0,0);

    struct sigaction act;
    act.sa_handler = sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGINT, &act, 0);

    int ID[5] = {0,1,2}; //Just used for numbering the producer and consumer

    if (argc != 2) {
        printf("Usage: %s <nitems> \n", argv[1]);
        return -1;
    }

    nitems = atoi(argv[1]);
    if(nitems*40>MEMSIZE || nitems<=0){
      printf("Too many items");
      return -1;
    }

    //:)
    buffer_size=nitems*40;
    itemBuffer = malloc(BUFFER_SIZE);
    //opening all 3 semaphors
    full = sem_open("full", O_CREAT | O_EXCL, 0644, 0);
    if(full == NULL) {
      perror("Semaphore initialization failed");
      return -1;
    }
    empty = sem_open("empty", O_CREAT | O_EXCL, 0644, 0);
    if(empty == NULL) {
      perror("Semaphore initialization failed");
      return -1;
    }
    // semid = sem_open("semid", O_CREAT | O_EXCL, 0644, 0);
    // if(semid == NULL) {
    //   perror("Semaphore initialization failed");
    //   return -1;
    // }
    //opening Mutex
    pthread_mutex_init(&mutex, NULL);

    //getting default attributes
    pthread_attr_init(&attr);

    //producer funct
    //pthread_create(&producer, &attr, (void *)producer_function, (void *)&ID[0]);
    pthread_create(&producer, &attr, (void *)producer_function, NULL);
    //consumer funct
    //pthread_create(&consumer, &attr, (void *)consumer_function, (void *)&ID[1]);
    pthread_create(&consumer, &attr, (void *)consumer_function, NULL);

    if(flag==1){
      return -1;
    }
    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    printf("finished");
    printf("flag:%d",flag);

   sem_unlink("full");
   sem_unlink("empty");
    // sem_unlink("semid");

    pthread_mutex_destroy(&mutex);

    free(itemBuffer);
    return 0;
}
